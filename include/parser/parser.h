/**
 * @file parser.h
 * This file contains the parser for the CSLO language.
 */

#ifndef cslo_parser_h
#define cslo_parser_h

#include <stdbool.h>

#include "compiler/compiler.h"
#include "parser/rules.h"
#include "compiler/scanner.h"

#define MAX_LOOKAHEAD 4

/**
 * @struct Parser
 *
 * Parser struct keeps track of current and
 * previous tokens.
 */
typedef struct Parser {
    Token current;
    Token previous;
    Token lookahead[MAX_LOOKAHEAD];
    int lookaheadCount;
    bool interpolating;
    bool hadError;
    bool panicMode;
} Parser;

/**
 * Global parser instance.
 */
extern Parser parser;

// define all the functions here

/**
 * Method for initialising the parser.
 */
void initParser();

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
 * Method for peeking at the nth token without consuming it.
 */
Token peekToken(int n);

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
void parseExpression();

/**
 * Method for parsing an embedded expression.
 */
void parseEmbeddedExpression(const char* expressionStart, int expressionLength);

/**
 * Method for parsing a declaration.
 */
void parseDeclaration();

/**
 * Method for compiling blocks.
 */
void parseBlock();

/**
 * Method for compiling a grouping.
 */
void parseGrouping(bool canAssign);

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
void listIndex(bool canAssign);
void has(bool canAssign);
void hasNot(bool canAssign);

#endif
