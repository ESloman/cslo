/**
 * @file rules.h
 * This file contains the rules for parsing tokens in the CSLO language.
 */

#ifndef cslo_rules_h
#define cslo_rules_h

#include "stdbool.h"

/**
 * @enum Precedence rules
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
