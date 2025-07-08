/**
 * @file statements.h
 * This file contains the declarations for the statements in the CSLO language.
 */

#include <stdio.h>

#include "compiler/codegen.h"
#include "compiler/scanner.h"
#include "parser/parser.h"
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
    } else if (matchToken(TOKEN_BREAK)) {
        breakStatement();
    } else if (matchToken(TOKEN_CONTINUE)) {
        continueStatement();
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

void breakStatement() {
    if (current->innermostLoopStart == -1) {
        error("Can't use 'break' outside of a loop.");
    }

    consumeToken(TOKEN_SEMICOLON, "Expect ';' after 'break'.");
}

void continueStatement() {
    if (current->innermostLoopStart == -1) {
        error("Can't use 'continue' outside of a loop.");
    }

    consumeToken(TOKEN_SEMICOLON, "Expect ';' after 'continue'.");

    // Discard any locals created inside the loop.
    for (
        int i = current->localCount - 1;
        i >= 0 && current->locals[i].depth > current->innermostLoopScopeDepth;
        i--
    ) {
        emitByte(OP_POP, parser.previous.line);
    }

    // Jump to top of current innermost loop.
    emitLoop(current->innermostLoopStart);
}

/**
 * Method for compiling for statements.
 */
void forStatement() {
    #ifdef DEBUG_LOGGING
    printf("== Locals at line %d (scopeDepth=%d, localCount=%d) ==\n", parser.previous.line, current->scopeDepth, current->localCount);
    for (int i = 0; i < current->localCount; i++) {
        printf("  [%d] %.*s (depth=%d)\n", i, current->locals[i].name.length, current->locals[i].name.start, current->locals[i].depth);
    }
    #endif
    #ifdef DEBUG_LOGGING
    printf("Scope depth before loop %d\n", current->scopeDepth);
    #endif
    beginScope();
    #ifdef DEBUG_LOGGING
    printf("Scope depth increased to %d\n", current->scopeDepth);
    #endif
    consumeToken(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

    #ifdef DEBUG_LOGGING
    printf("== Locals at line %d (scopeDepth=%d, localCount=%d) ==\n", parser.previous.line, current->scopeDepth, current->localCount);
    for (int i = 0; i < current->localCount; i++) {
        printf("  [%d] %.*s (depth=%d)\n", i, current->locals[i].name.length, current->locals[i].name.start, current->locals[i].depth);
    }
    #endif
    // check and handle 'for (var x in list)' syntax
    if (checkToken(TOKEN_VAR) && peekToken(1).type == TOKEN_IDENTIFIER && peekToken(2).type == TOKEN_IN) {
        parserAdvance();  // consume 'var'
        Token varName = parser.current;
        parserAdvance();  // consume identifier
        parserAdvance();  // consume 'in'
        parseExpression();
        consumeToken(TOKEN_RIGHT_PAREN, "Expect ')' after 'for' clauses.");

        // store iterable locally
        addLocal(syntheticToken("__iterable"));
        markInitialized();
        uint8_t iterableSlot = current->localCount - 1;
        emitBytes(OP_SET_LOCAL, iterableSlot);
        // this is the initial value of the iterable variable
        // do not pop it!

        // create index variable
        addLocal(syntheticToken("__idx"));
        markInitialized();
        uint8_t indexSlot = current->localCount - 1;
        emitConstant(NUMBER_VAL(0));
        emitBytes(OP_SET_LOCAL, indexSlot);
        // this is the initial value of the index variable
        // do not pop it!

        addLocal(varName);
        markInitialized();
        uint8_t varSlot = current->localCount - 1;
        // set the loop variable to a random initial value on the stack for now
        // this value will be overwritten on each loop
        emitConstant(NUMBER_VAL(-1));
        emitBytes(OP_SET_LOCAL, varSlot);
        // this is the initial value of the loop variable
        // do not pop it!

        int surroundingLoopStart = current->innermostLoopStart;
        int surroundingLoopScopeDepth = current->innermostLoopScopeDepth;
        current->innermostLoopStart = currentChunk()->count;
        current->innermostLoopScopeDepth = current->scopeDepth;

        // check we're still in range
        emitBytes(OP_GET_LOCAL, indexSlot);  // push index
        emitBytes(OP_GET_LOCAL, iterableSlot);     // push iterable
        emitByte(OP_LEN, parser.previous.line); // pop iterable, push length
        emitByte(OP_GREATER_EQUAL, parser.previous.line); // compare index >= length

        int exitJump = emitJump(OP_JUMP_IF_TRUE);
        emitByte(OP_POP, parser.previous.line); // pop the condition only

        // get the current index and iterable
        // the iterable first, and then the index second
        // OP_GET_INDEX will then pop both and push the value at that index
        // OP_SET_LOCAL will then update varSlot with the correct value - note that this won't pop the value
        // we can then pop the value safely as the original stack slot has been updated
        emitBytes(OP_GET_LOCAL, iterableSlot);
        emitBytes(OP_GET_LOCAL, indexSlot);

        emitBytes(OP_INVOKE, makeConstant(OBJ_VAL(copyString("__index__", 9))));
        emitByte(1, parser.previous.line);

        emitBytes(OP_SET_LOCAL, varSlot);
        emitByte(OP_POP, parser.previous.line);

        parseStatement();

        // increment index
        int incrementStart = currentChunk()->count;
        emitBytes(OP_GET_LOCAL, indexSlot);
        emitConstant(NUMBER_VAL(1));
        emitByte(OP_ADD, parser.previous.line);
        emitBytes(OP_SET_LOCAL, indexSlot);
        emitByte(OP_POP, parser.previous.line); // Pop the value after assignment

        emitLoop(current->innermostLoopStart);
        current->innermostLoopStart = incrementStart;
        patchJump(exitJump);
        emitByte(OP_POP, parser.previous.line); // pop the condition only
        // No OP_POP here! endScope() will clean up all locals and stack values for the for-in loop.
        #ifdef DEBUG_LOGGING
        printf("== Locals at line %d (scopeDepth=%d, localCount=%d) ==\n", parser.previous.line, current->scopeDepth, current->localCount);
        for (int i = 0; i < current->localCount; i++) {
            printf("  [%d] %.*s (depth=%d)\n", i, current->locals[i].name.length, current->locals[i].name.start, current->locals[i].depth);
        }
        #endif

        current->innermostLoopStart = surroundingLoopStart;
        current->innermostLoopScopeDepth = surroundingLoopScopeDepth;

        endScope();
        #ifdef DEBUG_LOGGING
        printf("== Locals after endscope at line %d (scopeDepth=%d, localCount=%d) ==\n", parser.previous.line, current->scopeDepth, current->localCount);
        for (int i = 0; i < current->localCount; i++) {
            printf("  [%d] %.*s (depth=%d)\n", i, current->locals[i].name.length, current->locals[i].name.start, current->locals[i].depth);
        }
        #endif
        return;
    }

    // handle traditional style for loop syntax
    if (matchToken(TOKEN_SEMICOLON)) {
        // nothing
    } else if (matchToken(TOKEN_VAR)) {
        #ifdef DEBUG_LOGGING
        printf("For loop with variable declaration.\n");
        #endif
        varDeclaration();
    } else {
        parseExpressionStatement();
    }

    int surroundingLoopStart = current->innermostLoopStart;
    int surroundingLoopScopeDepth = current->innermostLoopScopeDepth;
    current->innermostLoopStart = currentChunk()->count;
    current->innermostLoopScopeDepth = current->scopeDepth;
    int exitJump = -1;
    if(!matchToken(TOKEN_SEMICOLON)) {
        #ifdef DEBUG_LOGGING
        printf("Parsing for loop condition.\n");
        #endif
        parseExpression();
        consumeToken(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP, parser.previous.line); // pop the condition only
    } else {
        #ifdef DEBUG_LOGGING
        printf("No condition in for loop.\n");
        #endif
    }

    if (!matchToken(TOKEN_RIGHT_PAREN)) {
        #ifdef DEBUG_LOGGING
        printf("Parsing for loop increment.\n");
        #endif
        int bodyJump = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;
        parseExpression();
        emitByte(OP_POP, parser.previous.line);
        consumeToken(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(current->innermostLoopStart);
        current->innermostLoopStart = incrementStart;
        patchJump(bodyJump);
    }

    parseStatement();
    emitLoop(current->innermostLoopStart);

    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OP_POP, parser.previous.line); // pop the condition only
    }

    current->innermostLoopStart = surroundingLoopStart;
    current->innermostLoopScopeDepth = surroundingLoopScopeDepth;

    endScope();
    #ifdef DEBUG_LOGGING
    printf("== Locals at line %d (scopeDepth=%d, localCount=%d) ==\n", parser.previous.line, current->scopeDepth, current->localCount);
    for (int i = 0; i < current->localCount; i++) {
        printf("  [%d] %.*s (depth=%d)\n", i, current->locals[i].name.length, current->locals[i].name.start, current->locals[i].depth);
    }
    #endif
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

    beginScope();

    int surroundingLoopStart = current->innermostLoopStart;
    int surroundingLoopScopeDepth = current->innermostLoopScopeDepth;
    current->innermostLoopStart = currentChunk()->count;
    current->innermostLoopScopeDepth = current->scopeDepth;

    consumeToken(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    parseExpression();
    consumeToken(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP, parser.previous.line);
    parseStatement();
    emitLoop(current->innermostLoopStart);
    patchJump(exitJump);
    emitByte(OP_POP, parser.previous.line);

    current->innermostLoopStart = surroundingLoopStart;
    current->innermostLoopScopeDepth = surroundingLoopScopeDepth;

    endScope();
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


/**
 * Method for compiling an import statement.
 */
void parseImportStatement() {
    consumeToken(TOKEN_IDENTIFIER, "Expected module name after 'import'.");
    ObjString* moduleName = copyString(parser.previous.start, parser.previous.length);


    if (matchToken(TOKEN_AS)) {
        consumeToken(TOKEN_IDENTIFIER, "Expected name after 'as'.");
        ObjString* moduleNickName = copyString(parser.previous.start, parser.previous.length);
        emitByte(OP_IMPORT_AS, parser.previous.line);
        emitByte(makeConstant(OBJ_VAL(moduleName)), parser.previous.line);
        emitByte(makeConstant(OBJ_VAL(moduleNickName)), parser.previous.line);
    } else {
        emitBytes(OP_IMPORT, makeConstant(OBJ_VAL(moduleName)));
    }
    consumeToken(TOKEN_SEMICOLON, "Expected ';' after import statement.");
}
