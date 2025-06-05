/**
 * @file parser.h
 * This file contains the parser for the CSLO language.
 */

#ifndef cslo_parser_h
#define cslo_parser_h

#include <stdbool.h>

#include "compiler/compiler.h"
#include "compiler/scanner.h"

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
    PREC_FACTOR,      // * / % **
    PREC_UNARY,       // ! -
    PREC_POSTFIX,     // ++ --
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

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
 * @typedef ParseFn
 * Function pointer type for parsing functions.
 */
typedef void (*ParseFn)(bool canAssign);

/**
 * @struct ParseRule
 * Struct that defines the parsing rules for each token type.
 */
typedef struct ParseRule {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

/**
 * Global parser instance.
 */
extern Parser parser;

// define all the functions here

/**
 * Method for advancing the parser's token.
 *
 * Sets the previous token to the current and retrieves the next
 * token. Will report any errors if it encounters an error token.
 */
void parserAdvance();

/**
 * Method for parsing an expression with a given precedence.
 *
 * Will parse the expression according to the precedence level
 * and advance the parser's token.
 */
void parsePrecedence(Precedence precedence);

/**
 * Method for returning the rules for a given token.
 *
 * Will return the ParseRule for the given token type.
 */
ParseRule* getRule(TokenType type);

/**
 * Method for consuming a given token type.
 *
 * Will advance over the current token if it matches,
 * otherwise will throw an error with the given message.
 */
void consumeToken(TokenType type, const char* message);

/**
 * Method for checking if the current token type type.
 */
bool checkToken(TokenType type);

/**
 * Method for matching and consuming the given token type.
 */
bool matchToken(TokenType type);

/**
 * Method for parsing an expression.
 */
void expression();

/**
 * Method for parsing a statement.
 */
void statement();

/**
 * Method for compiling expression statements.
 */
void expressionStatement();

/**
 * Method for parsing a declaration.
 */
void declaration();

/**
 * Method for compiling blocks.
 */
void block();

/**
 * Method for compiling a grouping.
 */
void grouping(bool canAssign);

/**
 * Method for compiling a number.
 */
void number(bool canAssign);

/**
 * Method for compiling a string.
 */
void string(bool canAssign);

/**
 * Method for compiling a variable.
 *
 * Will compile the variable and emit the appropriate bytecode.
 */
void variable(bool canAssign);
void unary(bool canAssign);
void binary(bool canAssign);
void literal(bool canAssign);
void and_(bool canAssign);
void or_(bool canAssign);
void super_(bool canAssign);
void parseCall(bool canAssign);
void dot(bool canAssign);
void self(bool canAssign);
void prefixIncDec(bool canAssign);
void postfixIncDec(bool canAssign);
void compoundAssign(bool canAssign);
void list(bool canAssign);
void index(bool canAssign);
void has(bool canAssign);
void hasNot(bool canAssign);

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
extern ParseRule rules[];

#endif
