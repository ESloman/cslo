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
typedef struct Parser {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

/**
 * @enum Precedence
 */
typedef enum Precedence {
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
typedef void (*ParseFn)(bool canAssign);

/**
 * @struct ParseRule
 */
typedef struct ParseRule {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

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
static void unary(bool canAssign);
static void binary(bool canAssign);
static void literal(bool canAssign);

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
  [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_LESS]          = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
  [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
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
 * Emits an OP_RETURN to the chunk.
 */
static void emitReturn() {
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
 * Method for parsing a variable.
 */
static uint8_t parseVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);
    return identifierConstant(&parser.previous);
}

/**
 * Method for defining a variale.
 */
static void defineVariable(uint8_t global) {
    emitBytes(OP_DEFINE_GLOBAL, global);
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
 * 
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
 * Method for compiling a print.
 */
static void printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT, parser.previous.line);
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
            case TOKEN_WHILE:
            case TOKEN_PRINT:
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
    if (match(TOKEN_VAR)) {
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
    if (match(TOKEN_PRINT)) {
        printStatement();
    } else {
        expressionStatement();
    }
}

/**
 * 
 */
static void grouping(bool canAssign) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

/**
 * 
 */
static void number(bool canAssign) {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

/**
 * 
 */
static void string(bool canAssign) {
    emitConstant(OBJ_VAL(copyString(parser.previous.start +1, parser.previous.length - 2)));
}

/**
 * Method for compiling a named variable.
 */
static void namedVariable(Token name, bool canAssign) {
    uint8_t arg = identifierConstant(&name);
    
    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(OP_SET_GLOBAL, arg);
    } else {
        emitBytes(OP_GET_GLOBAL, arg);
    }
}

/**
 * Method for compiling a variable.
 */
static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

/**
 * 
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
bool compile(const char* source, Chunk* chunk) {
    initScanner(source);

    compilingChunk = chunk;

    parser.hadError = false;
    parser.panicMode = false;

    advance();
    
    while (!match(TOKEN_EOF)) {
        declaration();
    }

    endCompiler();
    return !parser.hadError;
}
