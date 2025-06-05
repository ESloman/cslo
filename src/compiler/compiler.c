/**
 * @file compiler.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/common.h"
#include "compiler/compiler.h"
#include "core/debug.h"
#include "compiler/codegen.h"
#include "compiler/parser.h"
#include "compiler/scanner.h"

#define MAX_IF_BRANCHES 56

Compiler* current;
ClassCompiler* currentClass = NULL;
Token lastVariableToken;

/**
 * Method for reporting an error.
 */
static void errorAt(Token* token, const char* message) {
    if (parser.panicMode) {
        return;
    }

    parser.panicMode = true;

    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // ?
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}


/**
 * Method for reporting an error.
 */
void error(const char* message) {
    errorAt(&parser.current, message);
}

/**
 * Method for reporting an error at the current token.
 */
void errorAtCurrent(const char* message) {
    errorAt(&parser.current, message);
}

/**
 * Method for patching the previous jump instruction.
 */
void patchJump(int offset) {
    // -2 to adjust for the bytecode for the jump offset itself
    int jump = currentChunk()->count - offset - 2;

    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }

    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
}

/**
 * Method for initialising our compiler.
 */
static void initCompiler(Compiler* compiler, FunctionType type) {
    compiler->enclosing = current;
    compiler->function = NULL;
    compiler->type = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function = newFunction();
    current = compiler;

    if (type != TYPE_SCRIPT) {
        current->function->name = copyString(parser.previous.start, parser.previous.length);
    }

    Local* local = &current->locals[current->localCount++];
    local->depth = 0;
    local->isCaptured = false;
    if (type != TYPE_FUNCTION) {
        local->name.start = "self";
        local->name.length = 4;
    } else {
        local->name.start = "";
        local->name.length = 0;
    }
}

/**
 * Method for ending the compiler.
 */
static ObjFunction* endCompiler() {
    emitReturn();
    ObjFunction* function = current->function;
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), function->name != NULL ? function->name->chars : "<script>");
    }
#endif

    current = current->enclosing;
    return function;
}

/**
 * Method for beginning a block's scope.
 */
void beginScope() {
    current->scopeDepth++;
}

/**
 * Method for ending a block's scope.
 */
void endScope() {
    current->scopeDepth--;

    while (current->localCount > 0 && current->locals[current->localCount - 1].depth > current->scopeDepth) {
        if (current->locals[current->localCount - 1].isCaptured) {
            emitByte(OP_CLOSE_UPVALUE, parser.previous.line);
        } else {
            emitByte(OP_POP, parser.previous.line);
        }
        current->localCount--;
    }
}

/**
 * Method for identifying a constant.
 */
uint8_t identifierConstant(Token* name) {
    return makeConstant(
        OBJ_VAL(copyString(name->start, name->length))
    );
}

/**
 * Method for comparing two identifiers.
 */
bool identifiersEqual(Token* a, Token* b) {
    if (a->length != b->length) {
        return false;
    }
    return memcmp(a->start, b->start, a->length) == 0;
}

/**
 * Method for resolving a local variable.
 */
int resolveLocal(Compiler* compiler, Token* name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                error("Can't read local var in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}

/**
 * Method for adding an upvalue to the current compiler.
 */
int addUpvalue(Compiler* compiler, uint8_t index, bool isLocal) {
    int upvalueCount = compiler->function->upvalueCount;

    for (int i = 0; i < upvalueCount; i++) {
        Upvalue* upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }

    if (upvalueCount == UINT8_COUNT) {
        error("Too many closure variables in function.");
        return 0;
    }

    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
}

/**
 * Method for resolving an upvalue variable.
 */
int resolveUpvalue(Compiler* compiler, Token* name) {
    if (compiler->enclosing == NULL) {
        return -1;
    }

    int local = resolveLocal(compiler->enclosing, name);
    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue(compiler, (uint8_t)local, true);
    }

    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(compiler, (uint8_t)upvalue, false);
    }

    return -1;
}

/**
 * Method for adding a local.
 */
void addLocal(Token name) {
    if (current->localCount == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }

    Local* local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
    local->isCaptured = false;
}

/**
 * Method for declaring a variable.
 */
void declareVariable() {
    if (current->scopeDepth == 0) {
        return;
    }

    Token* name = &parser.previous;

    for (int i = current->localCount - 1; i >= 0; i--) {
        Local* local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, &local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }
    addLocal(*name);
}

/**
 * Method for parsing a variable.
 */
uint8_t parseVariable(const char* errorMessage) {
    consumeToken(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();
    if (current->scopeDepth > 0) {
        return 0;
    }
    return identifierConstant(&parser.previous);
}

/**
 * Method for marking a local variable initialised.
 */
void markInitialized() {
    if (current->scopeDepth == 0) {
        return;
    }
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

/**
 * Method for defining a variale.
 */
void defineVariable(uint8_t global) {
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }
    emitBytes(OP_DEFINE_GLOBAL, global);
}

/**
 * Method for compiling an argument list.
 */
uint8_t argumentList() {
    uint8_t argCount = 0;
    if (!checkToken(TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if (argCount == 255) {
                error("Can't have more than 255 arguments.");
            }
            argCount++;
        } while(matchToken(TOKEN_COMMA));
    }
    consumeToken(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return argCount;
}

/**
 * Method for actually compiling a function.
 */
void function(FunctionType type) {
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope();

    consumeToken(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!checkToken(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                errorAtCurrent("Can't have more than 255 parameters.");
            }
            uint8_t constant = parseVariable("Expected parameter name.");
            defineVariable(constant);
        } while (matchToken(TOKEN_COMMA));
    }
    consumeToken(TOKEN_RIGHT_PAREN, "Expected ')' after parameters.");
    consumeToken(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block();

    ObjFunction* function = endCompiler();
    emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

    for (int i = 0; i < function->upvalueCount; i++) {
        emitByte(compiler.upvalues[i].isLocal ? 1 : 0, parser.previous.line);
        emitByte(compiler.upvalues[i].index, parser.previous.line);
    }
}

/**
 * Method for compiling class methods.
 */
void method() {
    consumeToken(TOKEN_FUN, "Expected 'func' to define method.");
    consumeToken(TOKEN_IDENTIFIER, "Expected method name.");
    uint8_t constant = identifierConstant(&parser.previous);

    FunctionType type = TYPE_METHOD;
    if (parser.previous.length == 8 && memcmp(parser.previous.start, "__init__", 8) == 0) {
        type = TYPE_INITIALISER;
    }
    function(type);
    emitBytes(OP_METHOD, constant);
}

/**
 * Method for compiling classes.
 */
void classDeclaration() {
    consumeToken(TOKEN_IDENTIFIER, "Expected class name.");
    Token className = parser.previous;
    uint8_t nameConstant = identifierConstant(&parser.previous);
    declareVariable();

    emitBytes(OP_CLASS, nameConstant);
    defineVariable(nameConstant);

    ClassCompiler classCompiler;
    classCompiler.hasSuperClass = false;
    classCompiler.enclosing = currentClass;
    currentClass = &classCompiler;

    if (matchToken(TOKEN_EXTENDS)) {
        consumeToken(TOKEN_IDENTIFIER, "Expected superclass name.");
        variable(false);

        if (identifiersEqual(&className, &parser.previous)) {
            error("A class can't inherit from itself.");
        }

        beginScope();
        addLocal(syntheticToken("super"));
        defineVariable(0);
        namedVariable(className, false);
        emitByte(OP_INHERIT, parser.previous.line);
        classCompiler.hasSuperClass = true;
    }

    namedVariable(className, false);
    consumeToken(TOKEN_LEFT_BRACE, "Expected '{' before class body.");
    while (!checkToken(TOKEN_RIGHT_BRACE) && !checkToken(TOKEN_EOF)) {
        method();
    }

    consumeToken(TOKEN_RIGHT_BRACE, "Expected '}' after class body.");
    emitByte(OP_POP, parser.previous.line);

    if (classCompiler.hasSuperClass) {
        endScope();
    }

    currentClass = currentClass->enclosing;
}

/**
 * Method for compiling functions.
 */
void funDeclaration() {
    uint8_t global = parseVariable("Expected function name.");
    markInitialized();
    function(TYPE_FUNCTION);
    defineVariable(global);
}

/**
 *
 */
void varDeclaration() {
    uint8_t global = parseVariable("Expected a variable name.");

    if (matchToken(TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OP_NIL, parser.previous.line);
    }

    consumeToken(TOKEN_SEMICOLON, "Expected ';' after variable declaration.");
    defineVariable(global);
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
            expression();
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

            statement();

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
        expressionStatement();
    }

    int loopStart = currentChunk()->count;
    int exitJump = -1;
    if(!matchToken(TOKEN_SEMICOLON)) {
        expression();
        consumeToken(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        exitJump = emitJump(OP_JUMP_IF_FALSE);
        // needed to pop the condition
        emitByte(OP_POP, parser.previous.line);
    }

    if (!matchToken(TOKEN_RIGHT_PAREN)) {
        int bodyJump = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;
        expression();
        emitByte(OP_POP, parser.previous.line);
        consumeToken(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(loopStart);
        loopStart = incrementStart;
        patchJump(bodyJump);
    }

    statement();
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
    expression();
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
    statement();

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
        expression();
        consumeToken(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

        // create patch jump position for next branch
        // this signals where to jump to if our expression evaluates to false
        int thenJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP, parser.previous.line);

        // the code in the elif block
        statement();

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
        statement();
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
    expression();
    consumeToken(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP, parser.previous.line);
    statement();
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
        expression();
        consumeToken(TOKEN_SEMICOLON, "Expected ';' after return value.");
        emitByte(OP_RETURN, parser.previous.line);
    }
}

/**
 * Method for synchronizing after an error.
 */
void synchronize() {
    parser.panicMode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) {
            return;
        }

        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_ELIF:
            case TOKEN_WHILE:
            case TOKEN_RETURN:
                return;
            default:
                ;
        }

        parserAdvance();
    }
}

/**
 * Method for compiling a named variable.
 */
void namedVariable(Token name, bool canAssign) {
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);

    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else if ((arg = resolveUpvalue(current, &name)) != -1) {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    } else {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    if (canAssign && matchToken(TOKEN_EQUAL)) {
        expression();
        emitBytes(setOp, (uint8_t)arg);
    } else {
        emitBytes(getOp, (uint8_t)arg);
    }
}

/**
 * @brief Creates a synthetic token from a string.
 *
 * This is used to create a token for identifiers that do not come directly from source code,
 * such as "self" or "super" in class contexts.
 *
 * @param text The string to use as the token's text.
 * @return The synthetic token.
 */
Token syntheticToken(const char* text) {
    Token token;
    token.start = text;
    token.length = (int)strlen(text);
    return token;
}


/**
 * Method for compiling given slo code into bytecode.
 *
 * Will fill up the given chunk with the bytecode.
 */
ObjFunction* compile(const char* source) {
    initScanner(source);

    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);

    parser.hadError = false;
    parser.panicMode = false;

    parserAdvance();

    while (!matchToken(TOKEN_EOF)) {
        declaration();
    }

    ObjFunction* function = endCompiler();
    return parser.hadError ? NULL : function;
}
