/**
 * @file compiler.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/common.h"
#include "core/errors.h"
#include "compiler/compiler.h"
#include "core/debug.h"
#include "compiler/codegen.h"
#include "parser/parser.h"
#include "compiler/scanner.h"
#include "parser/statements.h"

Compiler* current;
ClassCompiler* currentClass = NULL;
Token lastVariableToken;

static Token globalFinals[UINT8_MAX];
static int globalFinalCount = 0;
Token lastVariableToken;

/**
 * Method for reporting an error.
 */
static void errorAt(Token* token, const char* message) {
    if (parser.panicMode) {
        return;
    }

    Exception exc = {
        .type = ERROR_SYNTAX,
        .message = message,
        .line = token->line,
        .column = token->column,
        .file = current->function->file->chars,
        .stacktrace = NULL
    };
    reportError(&exc);
    parser.panicMode = true;
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
 * Method for patching a previous jump instruction to somewhere specific.
 */
void patchJumpTo(int offset, int target) {
    // -2 to adjust for the bytecode for the jump offset itself
    int jump = target - offset - 2;

    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }

    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
}

/**
 * Method for initialising our compiler.
 */
static void initCompiler(Compiler* compiler, FunctionType type, const char* file) {
    compiler->enclosing = current;
    compiler->function = NULL;
    compiler->type = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->innermostLoopStart = -1;
    compiler->innermostLoopScopeDepth = 0;
    compiler->continueCount = 0;
    compiler->breakCount = 0;
    compiler->function = newFunction();
    if (file != NULL) {
        compiler->function->file = copyString(file, (int)strlen(file));
    } else {
        compiler->function->file = copyString("<repl>", 6);
    }
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
    #ifdef DEBUG_LOGGING
    printf("beginScope: scopeDepth now %d\n", current->scopeDepth);
    #endif
}

/**
 * Method for ending a block's scope.
 */
void endScope() {
    current->scopeDepth--;
    #ifdef DEBUG_LOGGING
    printf("endScope: scopeDepth now %d\n", current->scopeDepth);
    #endif
    #ifdef DEBUG_LOGGING
    printf("== Locals at line %d (scopeDepth=%d, localCount=%d) ==\n", parser.previous.line, current->scopeDepth, current->localCount);
    for (int i = 0; i < current->localCount; i++) {
        printf("  [%d] %.*s (depth=%d)\n", i, current->locals[i].name.length, current->locals[i].name.start, current->locals[i].depth);
    }
    #endif
    while (current->localCount > 0 && current->locals[current->localCount - 1].depth > current->scopeDepth) {
        #ifdef DEBUG_LOGGING
        printf("Popping local: %.*s", current->locals[current->localCount - 1].name.length, current->locals[current->localCount - 1].name.start);
        printf("  (depth=%d, scopeDepth now=%d)\n", current->locals[current->localCount - 1].depth, current->scopeDepth);
        #endif
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
 * Method for adding an upvalue to the current compiler
*/
int addUpvalue(Compiler* compiler, uint8_t index, bool isLocal, bool isFinal) {
    int upvalueCount = compiler->function->upvalueCount;

    for (int i = 0; i < upvalueCount; i++) {
        Upvalue* upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            // If any upvalue is final, always treat as final
            if (isFinal) upvalue->isFinal = true;
            return i;
        }
    }

    if (upvalueCount == UINT8_COUNT) {
        error("Too many closure variables in function.");
        return 0;
    }

    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    compiler->upvalues[upvalueCount].isFinal = isFinal;
    return compiler->function->upvalueCount++;
}

/**
 * Method for resolving an upvalue variable.
 * */
int resolveUpvalue(Compiler* compiler, Token* name) {
    if (compiler->enclosing == NULL) {
        return -1;
    }

    int local = resolveLocal(compiler->enclosing, name);
    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;
        bool isFinal = compiler->enclosing->locals[local].isFinal;
        return addUpvalue(compiler, (uint8_t)local, true, isFinal);
    }

    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        // Propagate isFinal from enclosing upvalue
        bool isFinal = compiler->enclosing->upvalues[upvalue].isFinal;
        return addUpvalue(compiler, (uint8_t)upvalue, false, isFinal);
    }

    return -1;
}

/**
 * Method for adding a local.
 */
void addLocal(Token name, bool isFinal) {
    #ifdef DEBUG_LOGGING
    printf("addLocal: %.*s (localCount=%d, scopeDepth=%d)\n", name.length, name.start, current->localCount, current->scopeDepth);
    #endif
    if (current->localCount == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }

    Local* local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
    local->isCaptured = false;
    local->isFinal = isFinal;
}

/**
 * Method for declaring a variable.
 */
void declareVariable(bool isFinal) {
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

    for (int i = 0; i < globalFinalCount; i++) {
        if (identifiersEqual(name, &globalFinals[i])) {
            error("Cannot shadow a final global variable.");
        }
    }

    addLocal(*name, isFinal);
}

/**
 * Method for parsing a variable.
 */
uint8_t parseVariable(bool isFinal, const char* errorMessage) {
    consumeToken(TOKEN_IDENTIFIER, errorMessage);

    lastVariableToken = parser.previous;

    declareVariable(isFinal);
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
    #ifdef DEBUG_LOGGING
    printf("markInitialized: %.*s (localCount=%d, setting depth=%d)\n",
        current->locals[current->localCount - 1].name.length,
        current->locals[current->localCount - 1].name.start,
        current->localCount,
        current->scopeDepth);
    #endif
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

/**
 * Method for defining a variale.
 */
void defineVariable(uint8_t global, bool isFinal) {
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }
    if (isFinal) {
        if (globalFinalCount == UINT8_MAX) {
            error("Can't have more than 255 final globals.");
        }
        globalFinals[globalFinalCount++] = lastVariableToken;
    }
    OpCode op = isFinal ? OP_DEFINE_FINAL_GLOBAL : OP_DEFINE_GLOBAL;
    emitBytes((uint8_t)op, global);
}

/**
 * Method for compiling an argument list.
 */
uint8_t argumentList() {
    uint8_t argCount = 0;
    if (!checkToken(TOKEN_RIGHT_PAREN)) {
        do {
            parseExpression();
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
    initCompiler(&compiler, type, current->function->file->chars);
    beginScope();

    consumeToken(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!checkToken(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                errorAtCurrent("Can't have more than 255 parameters.");
            }
            uint8_t constant = parseVariable(false, "Expected parameter name.");
            defineVariable(constant, false);
        } while (matchToken(TOKEN_COMMA));
    }
    consumeToken(TOKEN_RIGHT_PAREN, "Expected ')' after parameters.");
    consumeToken(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    parseBlock();

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
    declareVariable(false);

    emitBytes(OP_CLASS, nameConstant);
    defineVariable(nameConstant, false);

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
        addLocal(syntheticToken("super"), false);
        defineVariable(0, false);
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
    uint8_t global = parseVariable(false, "Expected function name.");
    markInitialized();
    function(TYPE_FUNCTION);
    defineVariable(global, false);
}

/**
 *
 */
void varDeclaration(bool isFinal) {
    if (isFinal) {
        // consume the var
        consumeToken(TOKEN_VAR, "Expected 'var' after 'final'.");
    }
    uint8_t global = parseVariable(isFinal, "Expected a variable name.");

    if (isFinal) {
        // we have to define the variable value here for final
        consumeToken(TOKEN_EQUAL, "Expect '=' after variable name for final variables.");
        parseExpression();
    } else {
        if (matchToken(TOKEN_EQUAL)) {
            parseExpression();
        } else {
            emitByte(OP_NIL, parser.previous.line);
        }
    }

    consumeToken(TOKEN_SEMICOLON, "Expected ';' after variable declaration.");
    defineVariable(global, isFinal);
}

/**
 * Method for compiling an enum declaration.
 */
void enumDeclaration() {
    uint8_t global = parseVariable(false, "Expected enum name.");
    markInitialized();

    uint8_t count = 0;
    consumeToken(TOKEN_LEFT_BRACE, "Expected '{' before enum body.");

    if (!checkToken(TOKEN_RIGHT_BRACE)) {
        do {
            consumeToken(TOKEN_IDENTIFIER, "Expected enum member name.");
            emitConstant(OBJ_VAL(copyString(parser.previous.start, parser.previous.length)));
            emitConstant(NUMBER_VAL(count));
            count++;
        } while (matchToken(TOKEN_COMMA));
    }

    consumeToken(TOKEN_RIGHT_BRACE, "Expected '}' after enum body.");

    emitByte(OP_ENUM, parser.previous.line);
    emitByte(count, parser.previous.line);
    // emitByte((count >> 8) & 0xff, parser.previous.line);
    // emitByte(count & 0xff, parser.previous.line);

    emitByte(global, parser.previous.line);

    defineVariable(global, false);
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
    uint8_t getOp;
    uint8_t setOp;

    int arg = resolveLocal(current, &name);

    bool isFinal = false;
    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
        isFinal = current->locals[arg].isFinal;
    } else if ((arg = resolveUpvalue(current, &name)) != -1) {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
        isFinal = current->upvalues[arg].isFinal;
    } else {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    if (canAssign && matchToken(TOKEN_EQUAL)) {
        if (isFinal) {
            errorAt(&name, "Cannot assign to final variable.");
        }
        parseExpression();
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
ObjFunction* compile(const char* source, const char* file) {
    initScanner(source);
    initParser();

    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT, file);

    parser.hadError = false;
    parser.panicMode = false;

    parserAdvance();

    while (!matchToken(TOKEN_EOF)) {
        parseDeclaration();
    }

    ObjFunction* function = endCompiler();
    if (file != NULL) {
        function->file = copyString(file, (int)strlen(file));
    }
    return parser.hadError ? NULL : function;
}
