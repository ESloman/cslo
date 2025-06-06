/**
 * @file statements.h
 * This file contains the declarations for the statements in the CSLO language.
 */

#include "compiler/codegen.h"
#include "compiler/scanner.h"
#include "compiler/parser.h"
#include "core/chunk.h"
#include "core/common.h"
#include "parser/statements.h"

/**
 * Method for parsing a statement.
 */
void parseStatement() {
    if (matchToken(TOKEN_FOR)) {
        forStatement();
    } else if (matchToken(TOKEN_IF)) {
        ifStatement();
    } else if (matchToken(TOKEN_RETURN)) {
        returnStatement();
    } else if (matchToken(TOKEN_WHILE)) {
        whileStatement();
    } else if (matchToken(TOKEN_LEFT_BRACE)) {
        beginScope();
        parseBlock();
        endScope();
    } else {
        parseExpressionStatement();
    }
}

/**
 * Method for compiling expression statements.
 */
void parseExpressionStatement() {
    parseExpression();
    consumeToken(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP, parser.previous.line);
}

/**
 * Method for compiling for statements.
 */
void forStatement() {
    beginScope();
    consumeToken(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

    // check and handle 'for (x in list)' syntax
    if (checkToken(TOKEN_IDENTIFIER)) {
        Token varName = parser.current;
        parserAdvance();  // consume identifier
        if (checkToken(TOKEN_IN)) {
            parserAdvance();
            parseExpression();
            consumeToken(TOKEN_RIGHT_PAREN, "Expect ')' after 'for' clauses.");

            // store iterable locally
            addLocal(syntheticToken("__iterable"));
            uint8_t iterableSlot = current->localCount - 1;
            emitBytes(OP_SET_LOCAL, iterableSlot);

            // create index variable
            addLocal(syntheticToken("__idx"));
            uint8_t indexSlot = current->localCount - 1;
            emitConstant(NUMBER_VAL(0));
            emitBytes(OP_SET_LOCAL, indexSlot);

            int loopStart = currentChunk()->count;

            // check we're still in range
            emitBytes(OP_GET_LOCAL, indexSlot);
            emitBytes(OP_GET_LOCAL, iterableSlot);

            emitByte(OP_LEN, parser.previous.line);
            emitByte(OP_GREATER_EQUAL, parser.previous.line);

            int exitJump = emitJump(OP_JUMP_IF_TRUE);

            // get the current index and iterable
            emitBytes(OP_GET_LOCAL, iterableSlot);
            emitBytes(OP_GET_LOCAL, indexSlot);
            emitByte(OP_GET_INDEX, parser.previous.line);
            addLocal(varName);
            markInitialized();
            uint8_t varSlot = current->localCount - 1;
            emitBytes(OP_SET_LOCAL, varSlot);

            parseStatement();

            // increment index
            emitBytes(OP_GET_LOCAL, indexSlot);
            emitConstant(NUMBER_VAL(1));
            emitByte(OP_ADD, parser.previous.line);
            emitBytes(OP_SET_LOCAL, indexSlot);

            emitLoop(loopStart);
            patchJump(exitJump);
            emitByte(OP_POP, parser.previous.line);
            endScope();
            return;
        }
    }

    // handle traditional style for loop syntax
    if (matchToken(TOKEN_VAR)) {
        varDeclaration();
    } else if (matchToken(TOKEN_SEMICOLON)) {
        // nothing
    } else {
        parseExpressionStatement();
    }

    int loopStart = currentChunk()->count;
    int exitJump = -1;
    if(!matchToken(TOKEN_SEMICOLON)) {
        parseExpression();
        consumeToken(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        exitJump = emitJump(OP_JUMP_IF_FALSE);
        // needed to pop the condition
        emitByte(OP_POP, parser.previous.line);
    }

    if (!matchToken(TOKEN_RIGHT_PAREN)) {
        int bodyJump = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;
        parseExpression();
        emitByte(OP_POP, parser.previous.line);
        consumeToken(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(loopStart);
        loopStart = incrementStart;
        patchJump(bodyJump);
    }

    parseStatement();
    emitLoop(loopStart);

    if (exitJump != -1) {
        patchJump(exitJump);
        // needed to pop the condition
        emitByte(OP_POP, parser.previous.line);
    }
    endScope();
}

/**
 * Method for compiling if statements.
 *
 * This includes compiling the accompanying elif and else statements.
 * We use jumping and patching here for flow so our VM knows where to jump to and from.
 */
void ifStatement() {
    // compile the bit within the '()'
    // should evaluate to true/false
    consumeToken(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    parseExpression();
    consumeToken(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    // create an array of jumps
    // this is the jumps that'll be emitted at the end of each if/elif's block of code
    // we'll patch this later to tell the VM where the code continues after all the if/elif/else blocks
    // they'll all be patched to the same value
    int elseJumps[MAX_IF_BRANCHES];
    // initialise array elements to -1
    // MAX_IF_BRANCHES is max elif branches we'll support
    for (int x = 0; x < MAX_IF_BRANCHES; x++) {
        elseJumps[x] = -1;
    }

    // emit the OP_JUMP_IF_FALSE which we'll patch later
    // this signals where to jump to if our expression evaluates to false
    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP, parser.previous.line);

    // compile the actual code within the if block
    parseStatement();

    // create our jump point to patch later
    elseJumps[0] = emitJump(OP_JUMP);

    // patch the "if false" jump so we know where to skip to
    patchJump(thenJump);
    emitByte(OP_POP, parser.previous.line);

    int jumps = 1;
    // continuously loop if we keep finding ELIF tokens
    // each one is compiled similarly to an if
    // if false; we patch jump to the next elif, else, or end of branching
    // at the end of our block, we emit a jump to jump to the end of branching
    while(matchToken(TOKEN_ELIF)) {
        if (jumps == MAX_IF_BRANCHES) {
            error("Too many elif branches!");
        }
        // compile the bit within the '()'
        // should evaluate to true/false
        consumeToken(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
        parseExpression();
        consumeToken(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

        // create patch jump position for next branch
        // this signals where to jump to if our expression evaluates to false
        int thenJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP, parser.previous.line);

        // the code in the elif block
        parseStatement();

        // create our jump point to patch later
        elseJumps[jumps] = emitJump(OP_JUMP);

        // patch the "if false" jump so we know where to skip to
        patchJump(thenJump);
        emitByte(OP_POP, parser.previous.line);

        jumps++;
    }

    // else is optional
    // no need for any more patching in here, we just fall out of the 'else' block naturally
    if (matchToken(TOKEN_ELSE)) {
        parseStatement();
    }

    for (int j = 0; j < MAX_IF_BRANCHES; j++) {
        if (elseJumps[j] == -1) {
            // patched all the jumps
            break;
        }
        patchJump(elseJumps[j]);
    }
}

/**
 * Method for compiling while statements.
 */
void whileStatement() {
    int loopStart = currentChunk()->count;
    consumeToken(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    parseExpression();
    consumeToken(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP, parser.previous.line);
    parseStatement();
    emitLoop(loopStart);

    patchJump(exitJump);
    emitByte(OP_POP, parser.previous.line);
}

/**
 * Method for compiling a return statement.
 */
void returnStatement() {
    if (current->type == TYPE_SCRIPT) {
        error("Can't return from top-level code.");
    }

    if (matchToken(TOKEN_SEMICOLON)) {
        emitReturn();
    } else {
        if (current->type == TYPE_INITIALISER) {
            error("Can't return a value from an initialiser.");
        }
        parseExpression();
        consumeToken(TOKEN_SEMICOLON, "Expected ';' after return value.");
        emitByte(OP_RETURN, parser.previous.line);
    }
}
