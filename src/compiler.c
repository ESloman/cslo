/**
 * @file compiler.c
 */

#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "scanner.h"

/**
 * @struct Parser
 * 
 * Parser struct keeps track of current and
 * previous tokens.
 */
typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

/**
 * @enum Precedence
 */
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

/**
 * 
 */
typedef void (*ParseFn)();

/**
 * @struct ParseRule
 */
typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

// forward defining these here so they can be reference earlier than
// they are implemented
static void expression();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);
static void grouping();
static void number();
static void unary();
static void binary();

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
  [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE}, 
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_BANG]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_GREATER]       = {NULL,     NULL,   PREC_NONE},
  [TOKEN_GREATER_EQUAL] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LESS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LESS_EQUAL]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IDENTIFIER]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_STRING]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

Parser parser;
Chunk* compilingChunk;

/**
 * Method for returning the current compiling chunk.
 */
static Chunk* currentChunk() {
    return compilingChunk;
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
 * 
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
 * Method for 'emitting' a byte.
 * 
 * This writes a given bit of bytecode to the current chunk.
 */
static void emitByte(uint8_t byte, int line) {
    writeChunk(currentChunk(), byte, line);
}

/**
 * 
 */
static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1, parser.previous.line);
    emitByte(byte2, parser.previous.line);
}

/**
 * Emits an OP_RETURN to the chunk.
 */
static void emitReturn() {
    emitByte(OP_RETURN, parser.previous.line);
}

/**
 * 
 */
static void emitConstant(Value value) {
    writeConstant(currentChunk(), value, parser.previous.line);
}

/**
 * Method for ending the compiler.
 */
static void endCompiler() {
    emitReturn();
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), "code");
    }
#endif
}

/**
 * 
 */
static void parsePrecedence(Precedence precendence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }

    prefixRule();

    while (precendence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule();
    }
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
static void binary() {
    TokenType operatorType = parser.previous.type;

    int tLine = parser.previous.line;
    ParseRule* rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_PLUS:
            emitByte(OP_ADD, tLine);
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
 * Method for compiling an expression.
 */
static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

/**
 * 
 */
static void grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

/**
 * 
 */
static void number() {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(value);
}

/**
 * 
 */

static void unary() {
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
bool compile(const char* source, Chunk* chunk) {
    initScanner(source);

    compilingChunk = chunk;

    parser.hadError = false;
    parser.panicMode = false;

    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression.");
    endCompiler();
    return !parser.hadError;
}
