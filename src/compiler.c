/**
 * @file compiler.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "scanner.h"

#define MAX_IF_BRANCHES 56

// forward defining these here so they can be reference earlier than
// they are implemented
static void expression();
static void statement();
static void declaration();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);
static void grouping(bool canAssign);
static void number(bool canAssign);
static void string(bool canAssign);
static void variable(bool canAssign);
static Token syntheticToken(const char* text);
static void unary(bool canAssign);
static void binary(bool canAssign);
static void literal(bool canAssign);
static void and_(bool canAssign);
static void or_(bool canAssign);
static void super_(bool canAssign);
static void call(bool canAssign);
static void dot(bool canAssign);
static void namedVariable(Token name, bool canAssign);
static void self(bool canAssign);
static void prefixIncDec(bool canAssign);
static void postfixIncDec(bool canAssign);
static void compoundAssign(bool canAssign);
static void list(bool canAssign);
static void index(bool canAssign);
static void expressionStatement();
static void has(bool canAssign);
static void hasNot(bool canAssign);

/**
 * Table for defining parse rules and precedence.
 *
 * The keys to the table are the token types.
 *
 * The columns are:
 *   - the function for prefix expressions for that token type
 *   - the function for infix expressions for that token type
 *   - the precedence of an infix expression for that token type
 *
 * AKA:
 *   - an expression starting with a '(' will call the 'grouping' expression function
 *   - a '-' token will call 'unary' if it's a prefix \
 *      and 'binary' if it's an infix expression with it's precendence set to TERM
 *
 * The precendence tells us how much of remaining code to consume before returning.
 * AKA, how much code belongs to this expression we're compiling.
 */
ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]      = { grouping,     call,         PREC_CALL      },
    [TOKEN_RIGHT_PAREN]     = { NULL,         NULL,         PREC_NONE      },
    [TOKEN_LEFT_BRACE]      = { NULL,         NULL,         PREC_NONE      },
    [TOKEN_RIGHT_BRACE]     = { NULL,         NULL,         PREC_NONE      },
    [TOKEN_COMMA]           = { NULL,         NULL,         PREC_NONE      },
    [TOKEN_DOT]             = { NULL,         dot,          PREC_CALL      },
    [TOKEN_MINUS]           = { unary,        binary,       PREC_TERM      },
    [TOKEN_PLUS]            = { NULL,         binary,       PREC_TERM      },
    [TOKEN_SEMICOLON]       = { NULL,         NULL,         PREC_NONE      },
    [TOKEN_SLASH]           = { NULL,         binary,       PREC_FACTOR    },
    [TOKEN_STAR]            = { NULL,         binary,       PREC_FACTOR    },
    [TOKEN_MODULO]          = { NULL,         binary,       PREC_FACTOR    },
    [TOKEN_EXPO]            = { NULL,         binary,       PREC_FACTOR    },
    [TOKEN_BANG]            = { unary,        NULL,         PREC_NONE      },
    [TOKEN_BANG_EQUAL]      = { NULL,         binary,       PREC_EQUALITY  },
    [TOKEN_EQUAL]           = { NULL,         NULL,         PREC_NONE      },
    [TOKEN_EQUAL_EQUAL]     = { NULL,         binary,       PREC_EQUALITY  },
    [TOKEN_GREATER]         = { NULL,         binary,       PREC_EQUALITY  },
    [TOKEN_GREATER_EQUAL]   = { NULL,         binary,       PREC_EQUALITY  },
    [TOKEN_LESS]            = { NULL,         binary,       PREC_EQUALITY  },
    [TOKEN_LESS_EQUAL]      = { NULL,         binary,       PREC_EQUALITY  },
    [TOKEN_IDENTIFIER]      = { variable,     NULL,         PREC_NONE      },
    [TOKEN_STRING]          = { string,       NULL,         PREC_NONE      },
    [TOKEN_NUMBER]          = { number,       NULL,         PREC_NONE      },
    [TOKEN_AND]             = { NULL,         and_,         PREC_AND       },
    [TOKEN_CLASS]           = { NULL,         NULL,         PREC_NONE      },
    [TOKEN_ELSE]            = { NULL,         NULL,         PREC_NONE      },
    [TOKEN_FOR]             = { NULL,         NULL,         PREC_NONE      },
    [TOKEN_FUN]             = { NULL,         NULL,         PREC_NONE      },
    [TOKEN_ELIF]            = { NULL,         NULL,         PREC_NONE      },
    [TOKEN_IF]              = { NULL,         NULL,         PREC_NONE      },
    [TOKEN_NIL]             = { literal,      NULL,         PREC_NONE      },
    [TOKEN_OR]              = { NULL,         or_,          PREC_OR        },
    [TOKEN_RETURN]          = { NULL,         NULL,         PREC_NONE      },
    [TOKEN_SUPER]           = { super_,       NULL,         PREC_NONE      },
    [TOKEN_SELF]            = { self,         NULL,         PREC_NONE      },
    [TOKEN_VAR]             = { NULL,         NULL,         PREC_NONE      },
    [TOKEN_WHILE]           = { NULL,         NULL,         PREC_NONE      },
    [TOKEN_ERROR]           = { NULL,         NULL,         PREC_NONE      },
    [TOKEN_EOF]             = { NULL,         NULL,         PREC_NONE      },
    [TOKEN_PLUS_PLUS]       = { prefixIncDec, postfixIncDec,PREC_POSTFIX   },
    [TOKEN_MINUS_MINUS]     = { prefixIncDec, postfixIncDec,PREC_POSTFIX   },
    [TOKEN_PLUS_EQUAL]      = { NULL, compoundAssign,         PREC_ASSIGNMENT },
    [TOKEN_MINUS_EQUAL]     = { NULL, compoundAssign,         PREC_ASSIGNMENT },
    [TOKEN_STAR_EQUAL]      = { NULL, compoundAssign,         PREC_ASSIGNMENT },
    [TOKEN_SLASH_EQUAL]     = { NULL, compoundAssign,         PREC_ASSIGNMENT },
    [TOKEN_LEFT_BRACKET]    = { list, index,          PREC_CALL},
    [TOKEN_HAS]              = { NULL,          has,         PREC_EQUALITY      },
    [TOKEN_HAS_NOT]              = { NULL,          hasNot,         PREC_EQUALITY      },
    // [TOKEN_RIGHT_BRACKET]   = { NULL,         NULL,         PREC_NONE      },
};

Parser parser;
Compiler* current = NULL;
ClassCompiler* currentClass = NULL;
Token lastVariableToken;

/**
 * Method for returning the current compiling chunk.
 */
static Chunk* currentChunk() {
    return &current->function->chunk;
}

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
static void error(const char* message) {
    errorAt(&parser.current, message);
}

/**
 * Method for reporting an error at the current token.
 */
static void errorAtCurrent(const char* message) {
    errorAt(&parser.current, message);
}

/**
 * Method for advancing the parser's token.
 *
 * Sets the previous token to the current and retrieves the next
 * token. Will report any errors if it encoutners an error token.
 */
static void advance() {
    parser.previous = parser.current;

    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) {
            break;
        }

        errorAtCurrent(parser.current.start);
    }
}

/**
 * Method for consuming a given token type.
 *
 * Will advance over the current token if it matches,
 * otherwise will throw an error with the given message.
 */
static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

/**
 * Method for checking if the current token type type.
 */
static bool check(TokenType type) {
    return parser.current.type == type;
}

/**
 * Method for matching and consuming the given token type.
 */
static bool match(TokenType type) {
    if (!check(type)) {
        return false;
    }
    advance();
    return true;
}

/**
 * Method for 'emitting' a byte.
 *
 * This writes a given bit of bytecode to the current chunk.
 *
 * This takes a line number as a parameter because it's plausible
 * that some bytes being emitted aren't necessarily always going to be
 * from 'parser.previous.line'.
 */
static void emitByte(uint8_t byte, int line) {
    writeChunk(currentChunk(), byte, line);
}

/**
 * Method for emitting two bytes in sequence for convenience.
 */
static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1, parser.previous.line);
    emitByte(byte2, parser.previous.line);
}

/**
 * Method for emitting a loop instruction.
 */
static void emitLoop(int loopStart) {
    emitByte(OP_LOOP, parser.previous.line);

    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX) {
        error("Loop body too large.");
    }

    emitByte((offset >> 8) & 0xff, parser.previous.line);
    emitByte(offset & 0xff, parser.previous.line);
}

/**
 * Method for emitting a jump instruction.
 */
static int emitJump(uint8_t instruction) {
    int line = parser.previous.line;
    emitByte(instruction, line);
    emitByte(0xff, line);
    emitByte(0xff, line);
    return currentChunk()->count - 2;
}

/**
 * Emits an OP_RETURN to the chunk.
 */
static void emitReturn() {
    if (current->type == TYPE_INITIALISER) {
        emitBytes(OP_GET_LOCAL, 0);
    } else {
        emitByte(OP_NIL, parser.previous.line);
    }
    emitByte(OP_RETURN, parser.previous.line);
}

/**
 * makeConstant
 */
static uint8_t makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

/**
 * Method for writing a constant to the chunk.
 */
static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

/**
 * Method for patching the previous jump instruction.
 */
static void patchJump(int offset) {
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
static void beginScope() {
    current->scopeDepth++;
}

/**
 * Method for ending a block's scope.
 */
static void endScope() {
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
 *
 */
static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

/**
 * Method for identifying a constant.
 */
static uint8_t identifierConstant(Token* name) {
    return makeConstant(
        OBJ_VAL(copyString(name->start, name->length))
    );
}

/**
 * Method for comparing two identifiers.
 */
static bool identifiersEqual(Token* a, Token* b) {
    if (a->length != b->length) {
        return false;
    }
    return memcmp(a->start, b->start, a->length) == 0;
}

/**
 * Method for resolving a local variable.
 */
static int resolveLocal(Compiler* compiler, Token* name) {
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
static int addUpvalue(Compiler* compiler, uint8_t index, bool isLocal) {
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
static int resolveUpvalue(Compiler* compiler, Token* name) {
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
static void addLocal(Token name) {
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
static void declareVariable() {
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
static uint8_t parseVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();
    if (current->scopeDepth > 0) {
        return 0;
    }
    return identifierConstant(&parser.previous);
}

/**
 * Method for marking a local variable initialised.
 */
static void markInitialized() {
    if (current->scopeDepth == 0) {
        return;
    }
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

/**
 * Method for defining a variale.
 */
static void defineVariable(uint8_t global) {
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }
    emitBytes(OP_DEFINE_GLOBAL, global);
}

/**
 * @brief Compiles a postfix increment or decrement expression (x++ or x--).
 *
 * This function assumes the variable's value is already on the stack.
 * It duplicates the value (so the original is left as the result of the expression),
 * increments or decrements the value, stores the new value back into the variable,
 * and then pops the new value, leaving the original value on the stack.
 *
 * @param canAssign Indicates if assignment is allowed (unused for postfix).
 */
static void postfixIncDec(bool canAssign) {
    TokenType op = parser.previous.type;
    // The variable is already on the stack (from the prefix rule for identifiers)
    // Duplicate the value for the result
    emitByte(OP_DUP, parser.previous.line);
    emitConstant(NUMBER_VAL(1));
    emitByte(op == TOKEN_PLUS_PLUS ? OP_ADD : OP_SUBTRACT, parser.previous.line);


    uint8_t setOp;
    int arg = resolveLocal(current, &lastVariableToken);

    if (arg != -1) {
        setOp = OP_SET_LOCAL;
    } else if ((arg = resolveUpvalue(current, &lastVariableToken)) != -1) {
        setOp = OP_SET_UPVALUE;
    } else {
        arg = identifierConstant(&lastVariableToken);
        setOp = OP_SET_GLOBAL;
    }

    emitBytes(setOp, (uint8_t)arg);
    emitByte(OP_POP, parser.previous.line);
}

/**
 * @brief Compiles a prefix increment or decrement expression (++x or --x).
 *
 * Advances to the variable token, loads its value, increments or decrements it,
 * stores the new value back into the variable, and leaves the new value on the stack
 * as the result of the expression.
 *
 * @param canAssign Indicates if assignment is allowed (unused for prefix).
 */
static void prefixIncDec(bool canAssign) {
    TokenType op = parser.previous.type;
    advance();
    variable(false);
    emitConstant(NUMBER_VAL(1));
    emitByte(op == TOKEN_PLUS_PLUS ? OP_ADD : OP_SUBTRACT, parser.previous.line);
    uint8_t setOp;
    int arg = resolveLocal(current, &lastVariableToken);

    if (arg != -1) {
        setOp = OP_SET_LOCAL;
    } else if ((arg = resolveUpvalue(current, &lastVariableToken)) != -1) {
        setOp = OP_SET_UPVALUE;
    } else {
        arg = identifierConstant(&lastVariableToken);
        setOp = OP_SET_GLOBAL;
    }
    emitBytes(setOp, (uint8_t)arg);
}

/**
 * @brief Compiles a compound assignment expression (e.g., x += y, x -= y, x *= y, x /= y).
 *
 * This function assumes the left-hand side is a variable and is already on the stack.
 * It parses the right-hand side expression, emits the appropriate bytecode for the operation,
 * and stores the result back into the variable.
 *
 * @param canAssign Indicates if assignment is allowed (unused).
 */
static void compoundAssign(bool canAssign) {
    if (lastVariableToken.start == NULL || lastVariableToken.length == 0) {
        error("Invalid assignment target for compound assignment.");
        return;
    }

    TokenType op = parser.previous.type;
    // The left-hand side (variable) is already parsed and on the stack
    // Save the variable token for assignment
    Token varToken = lastVariableToken;

    // Parse the right-hand side expression
    expression();

    // Perform the operation
    if (op == TOKEN_PLUS_EQUAL) {
        emitByte(OP_ADD, parser.previous.line);
    } else if (op == TOKEN_MINUS_EQUAL) {
        emitByte(OP_SUBTRACT, parser.previous.line);
    } else if (op == TOKEN_STAR_EQUAL) {
        emitByte(OP_MULTIPLY, parser.previous.line);
    } else if (op == TOKEN_SLASH_EQUAL) {
        emitByte(OP_DIVIDE, parser.previous.line);
    }

    // Assign the result back to the variable
    uint8_t setOp;
    int arg = resolveLocal(current, &varToken);
    if (arg != -1) {
        setOp = OP_SET_LOCAL;
    } else if ((arg = resolveUpvalue(current, &varToken)) != -1) {
        setOp = OP_SET_UPVALUE;
    } else {
        arg = identifierConstant(&varToken);
        setOp = OP_SET_GLOBAL;
    }
    emitBytes(setOp, (uint8_t)arg);
}

/**
 * Method for compiling a list.
 */
static void list(bool canAssign) {
    int32_t argCount = 0;

    if (!check(TOKEN_RIGHT_BRACKET)) {
        do {
            expression();
            if (argCount >= INT32_MAX) {
                error("Can't have more elements in a list that 65k.");
            }
            argCount++;
        } while(match(TOKEN_COMMA));
    }

    consume(TOKEN_RIGHT_BRACKET, "Expect ']' after list literal.");
    emitByte(OP_LIST, parser.previous.line);
    emitByte((argCount >> 8) & 0xff, parser.previous.line);
    emitByte(argCount & 0xff, parser.previous.line);
}

/**
 * Method for compiling list indexing.
 */
static void index(bool canAssign) {
    if (check(TOKEN_COLON)) {
        emitByte(OP_NIL, parser.previous.line); // default to nil if no start index
    } else {
        expression(); // parse the index value
    }
    if (match(TOKEN_COLON)) {
        // handle slicers
        if (!check(TOKEN_RIGHT_BRACKET)) {
            expression(); // parse the end index
        } else {
            emitByte(OP_NIL, parser.previous.line); // default to nil if no end index
        }
        consume(TOKEN_RIGHT_BRACKET, "Expect ']' after index.");
        emitByte(OP_SLICE, parser.previous.line);
        return;
    }

    // handle standard indexing
    consume(TOKEN_RIGHT_BRACKET, "Expect ']' after index.");
    if (match(TOKEN_PLUS_EQUAL) || match(TOKEN_MINUS_EQUAL) ||
        match(TOKEN_STAR_EQUAL) || match(TOKEN_SLASH_EQUAL)) {

            TokenType op = parser.previous.type;

            emitByte(OP_DUP2, parser.previous.line);
            emitByte(OP_GET_INDEX, parser.previous.line);

            // If we have a compound assignment, we need to compile the right-hand side
            expression();
            switch (op) {
                case TOKEN_PLUS_EQUAL:    emitByte(OP_ADD, parser.previous.line); break;
                case TOKEN_MINUS_EQUAL:   emitByte(OP_SUBTRACT, parser.previous.line); break;
                case TOKEN_STAR_EQUAL:    emitByte(OP_MULTIPLY, parser.previous.line); break;
                case TOKEN_SLASH_EQUAL:   emitByte(OP_DIVIDE, parser.previous.line); break;
                default: break;
            }
            emitByte(OP_SET_INDEX, parser.previous.line);
    } else if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitByte(OP_SET_INDEX, parser.previous.line);
    } else {
        emitByte(OP_GET_INDEX, parser.previous.line);
    }
}

/**
 * Method for compiling an argument list.
 */
static uint8_t argumentList() {
    uint8_t argCount = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if (argCount == 255) {
                error("Can't have more than 255 arguments.");
            }
            argCount++;
        } while(match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return argCount;
}

/**
 * Method for compiling AND statements.
 * Effectively, works like control flow.
 * By this point, the left-hand side has been compiled
 * already so if it evaluates to 'False', we want to skip
 * the right-hand statement.
 */
static void and_(bool canAssign) {
    int endJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP, parser.previous.line);
    parsePrecedence(PREC_AND);

    patchJump(endJump);
}

/**
 * Method for compiling OR statements.
 * Effectively, works like control flow.
 * By this point, the left-hand side has been compiled
 * already so if it evaluates to 'True', we want to skip
 * the right-hand statement.
 */
static void or_(bool canAssign) {
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP, parser.previous.line);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

/**
 * Method for compiling has statements.
 */
static void has(bool canAssign) {
    expression();
    emitByte(OP_HAS, parser.previous.line);
}

/**
 * Method for compiling has statements.
 */
static void hasNot(bool canAssign) {
    expression();
    emitByte(OP_HAS_NOT, parser.previous.line);
}

/**
 * Method for returning the rules for a given token.
 */
static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

/**
 *
 */
static void binary(bool canAssign) {
    TokenType operatorType = parser.previous.type;

    int tLine = parser.previous.line;
    ParseRule* rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_BANG_EQUAL:
            emitByte(OP_NOT_EQUAL, tLine);
            break;
        case TOKEN_EQUAL_EQUAL:
            emitByte(OP_EQUAL, tLine);
            break;
        case TOKEN_GREATER:
            emitByte(OP_GREATER, tLine);
            break;
        case TOKEN_GREATER_EQUAL:
            emitByte(OP_GREATER_EQUAL, tLine);
            break;
        case TOKEN_LESS:
            emitByte(OP_LESS, tLine);
            break;
        case TOKEN_LESS_EQUAL:
            emitByte(OP_LESS_EQUAL, tLine);
            break;
        case TOKEN_PLUS:
            emitByte(OP_ADD, tLine);
            break;
        case TOKEN_MODULO:
            emitByte(OP_MODULO, tLine);
            break;
        case TOKEN_EXPO:
            emitByte(OP_POW, tLine);
            break;
        case TOKEN_MINUS:
            emitByte(OP_SUBTRACT, tLine);
            break;
        case TOKEN_STAR:
            emitByte(OP_MULTIPLY, tLine);
            break;
        case TOKEN_SLASH:
            emitByte(OP_DIVIDE, tLine);
            break;
        default:
            return;
        }
}

/**
 * Method for compiling function calls.
 */
static void call(bool canAssign) {
    uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);
}

/**
 * Method for compiling . expressions.
*/
static void dot(bool canAssign) {
    consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
    uint8_t name = identifierConstant(&parser.previous);

    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(OP_SET_PROPERTY, name);
    } else if (match(TOKEN_LEFT_PAREN)) {
        uint8_t argCount = argumentList();
        emitBytes(OP_INVOKE, name);
        emitByte(argCount, parser.previous.line);
    } else {
        emitBytes(OP_GET_PROPERTY, name);
    }
}

/**
 * @brief Compiles a literal value (nil, true, false).
 *
 * Emits bytecode to load the literal onto the stack.
 *
 * @param canAssign Indicates if assignment is allowed (unused).
 */
static void literal(bool canAssign) {

    int tLine = parser.previous.line;
    switch (parser.previous.type) {
        case TOKEN_FALSE:
            emitByte(OP_FALSE, tLine);
            break;
        case TOKEN_NIL:
            emitByte(OP_NIL, tLine);
            break;
        case TOKEN_TRUE:
            emitByte(OP_TRUE, tLine);
            break;
        default:
            return;
    }
}


/**
 * Method for compiling an expression.
 */
static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

/**
 * Method for compiling blocks.
 */
static void block() {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

/**
 * Method for actually compiling a function.
 */
static void function(FunctionType type) {
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope();

    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                errorAtCurrent("Can't have more than 255 parameters.");
            }
            uint8_t constant = parseVariable("Expected parameter name.");
            defineVariable(constant);
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after parameters.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
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
static void method() {
    consume(TOKEN_FUN, "Expected 'func' to define method.");
    consume(TOKEN_IDENTIFIER, "Expected method name.");
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
static void classDeclaration() {
    consume(TOKEN_IDENTIFIER, "Expected class name.");
    Token className = parser.previous;
    uint8_t nameConstant = identifierConstant(&parser.previous);
    declareVariable();

    emitBytes(OP_CLASS, nameConstant);
    defineVariable(nameConstant);

    ClassCompiler classCompiler;
    classCompiler.hasSuperClass = false;
    classCompiler.enclosing = currentClass;
    currentClass = &classCompiler;

    if (match(TOKEN_EXTENDS)) {
        consume(TOKEN_IDENTIFIER, "Expected superclass name.");
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
    consume(TOKEN_LEFT_BRACE, "Expected '{' before class body.");
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        method();
    }

    consume(TOKEN_RIGHT_BRACE, "Expected '}' after class body.");
    emitByte(OP_POP, parser.previous.line);

    if (classCompiler.hasSuperClass) {
        endScope();
    }

    currentClass = currentClass->enclosing;
}

/**
 * Method for compiling functions.
 */
static void funDeclaration() {
    uint8_t global = parseVariable("Expected function name.");
    markInitialized();
    function(TYPE_FUNCTION);
    defineVariable(global);
}

/**
 *
 */
static void varDeclaration() {
    uint8_t global = parseVariable("Expected a variable name.");

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OP_NIL, parser.previous.line);
    }

    consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration.");
    defineVariable(global);
}

/**
 * Method for compiling expression statements.
 */
static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP, parser.previous.line);
}

/**
 * Method for compiling for statements.
 */
static void forStatement() {
    beginScope();
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

    // check and handle 'for (var x in list)' syntax
    if (match(TOKEN_VAR)) {
        consume(TOKEN_IDENTIFIER, "Expect variable name after 'var'.");
        Token varName = parser.previous;
        consume(TOKEN_IN, "Expect 'in' after variable name.");
        expression();
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after 'for' clauses.");

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

    if (match(TOKEN_SEMICOLON)) {
        // NOTHING
    } else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        expressionStatement();
    }

    int loopStart = currentChunk()->count;
    int exitJump = -1;
    if(!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        exitJump = emitJump(OP_JUMP_IF_FALSE);
        // needed to pop the condition
        emitByte(OP_POP, parser.previous.line);
    }

    if (!match(TOKEN_RIGHT_PAREN)) {
        int bodyJump = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;
        expression();
        emitByte(OP_POP, parser.previous.line);
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

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
static void ifStatement() {
    // compile the bit within the '()'
    // should evaluate to true/false
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

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
    while(match(TOKEN_ELIF)) {
        if (jumps == MAX_IF_BRANCHES) {
            error("Too many elif branches!");
        }
        // compile the bit within the '()'
        // should evaluate to true/false
        consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
        expression();
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

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
    if (match(TOKEN_ELSE)) {
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
static void whileStatement() {
    int loopStart = currentChunk()->count;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

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
static void returnStatement() {
    if (current->type == TYPE_SCRIPT) {
        error("Can't return from top-level code.");
    }

    if (match(TOKEN_SEMICOLON)) {
        emitReturn();
    } else {
        if (current->type == TYPE_INITIALISER) {
            error("Can't return a value from an initialiser.");
        }
        expression();
        consume(TOKEN_SEMICOLON, "Expected ';' after return value.");
        emitByte(OP_RETURN, parser.previous.line);
    }
}

/**
 * Method for synchronizing after an error.
 */
static void synchronize() {
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

        advance();
    }
}

/**
 * Method for compiling a declaration.
 */
static void declaration() {
    if (match(TOKEN_CLASS)) {
        classDeclaration();
    } else if (match(TOKEN_FUN)) {
        funDeclaration();
    } else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }

    if (parser.panicMode) {
        synchronize();
    }
}

/**
 *
 */
static void statement() {
    if (match(TOKEN_FOR)) {
        forStatement();
    } else if (match(TOKEN_IF)) {
        ifStatement();
    } else if (match(TOKEN_RETURN)) {
        returnStatement();
    } else if (match(TOKEN_WHILE)) {
        whileStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
}

/**
 * @brief Compiles a grouping expression (parentheses).
 *
 * Parses the inner expression and expects a closing parenthesis.
 *
 * @param canAssign Indicates if assignment is allowed (unused).
 */
static void grouping(bool canAssign) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

/**
 * @brief Compiles a numeric literal.
 *
 * Emits bytecode to load the number onto the stack.
 *
 * @param canAssign Indicates if assignment is allowed (unused).
 */
static void number(bool canAssign) {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

/**
 * @brief Compiles a string literal.
 *
 * Emits bytecode to load the string onto the stack.
 *
 * @param canAssign Indicates if assignment is allowed (unused).
 */
static void string(bool canAssign) {
    emitConstant(OBJ_VAL(copyString(parser.previous.start +1, parser.previous.length - 2)));
}

/**
 * Method for compiling a named variable.
 */
static void namedVariable(Token name, bool canAssign) {
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

    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(setOp, (uint8_t)arg);
    } else {
        emitBytes(getOp, (uint8_t)arg);
    }
}

/**
 * Method for compiling a variable.
 */
static void variable(bool canAssign) {
    lastVariableToken = parser.previous;
    namedVariable(parser.previous, canAssign);
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
static Token syntheticToken(const char* text) {
    Token token;
    token.start = text;
    token.length = (int)strlen(text);
    return token;
}

/**
 * @brief Handles the 'super' keyword for superclass method access.
 *
 * Emits bytecode to access or invoke a method from the superclass.
 *
 * @param canAssign Indicates if assignment is allowed (unused).
 */
static void super_(bool canAssign) {
    if (currentClass == NULL) {
        error("Can't use 'super' outside of a class.");
    } else if (!currentClass->hasSuperClass) {
        error("Can't use 'super' in a class with no superclass.");
    }

    consume(TOKEN_DOT, "Expected '.' after 'super'.");
    consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
    uint8_t name = identifierConstant(&parser.previous);

    namedVariable(syntheticToken("self"), false);
    if (match(TOKEN_LEFT_PAREN)) {
        uint8_t argCount = argumentList();
        namedVariable(syntheticToken("super"), false);
        emitBytes(OP_SUPER_INVOKE, name);
        emitByte(argCount, parser.previous.line);
    } else {
        namedVariable(syntheticToken("super"), false);
        emitBytes(OP_GET_SUPER, name);
    }
}

/**
 * @brief Handles the 'self' keyword for method calls within a class.
 *
 * Emits bytecode to push the current instance onto the stack.
 *
 * @param canAssign Indicates if assignment is allowed (unused).
 */
static void self(bool canAssign) {
    if (currentClass == NULL) {
        error("Can't use 'self' outside of a class.");
        return;
    }
    variable(false);
}

/**
 * @brief Compiles a unary expression (e.g., -x or !x).
 *
 * Parses the operand and emits the appropriate bytecode for the unary operator.
 *
 * @param canAssign Indicates if assignment is allowed (unused).
 */
static void unary(bool canAssign) {
    TokenType operatorType = parser.previous.type;

    // store the line this unary operator is one for 'emitByte'
    // it's plausible that expression() for compiling the operand
    // will take us onto a different line so our error reporting
    // could be wrong
    int tLine = parser.previous.line;

    parsePrecedence(PREC_UNARY);

    switch (operatorType) {
        case TOKEN_MINUS:
            emitByte(OP_NEGATE, tLine);
            break;
        case TOKEN_BANG:
            emitByte(OP_NOT, tLine);
            break;
        default:
            return;
    }
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

    advance();

    while (!match(TOKEN_EOF)) {
        declaration();
    }

    ObjFunction* function = endCompiler();
    return parser.hadError ? NULL : function;
}
