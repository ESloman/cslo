/**
 * @file rules.c
 * This file contains the rules for parsing tokens in the CSLO language.
 */

#include "parser/parser.h"
#include "compiler/scanner.h"
#include "parser/expressions.h"
#include "parser/rules.h"

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
    [TOKEN_LEFT_PAREN]      = { parseGrouping,       parseCall,      PREC_CALL       },
    [TOKEN_RIGHT_PAREN]     = { NULL,                NULL,           PREC_NONE       },
    [TOKEN_LEFT_BRACE]      = { parseDictLiteral,    NULL,           PREC_NONE       },
    [TOKEN_RIGHT_BRACE]     = { NULL,                NULL,           PREC_NONE       },
    [TOKEN_COMMA]           = { NULL,                NULL,           PREC_NONE       },
    [TOKEN_DOT]             = { NULL,                dot,            PREC_CALL       },
    [TOKEN_MINUS]           = { unary,               binary,         PREC_TERM       },
    [TOKEN_PLUS]            = { NULL,                binary,         PREC_TERM       },
    [TOKEN_SEMICOLON]       = { NULL,                NULL,           PREC_NONE       },
    [TOKEN_SLASH]           = { NULL,                binary,         PREC_FACTOR     },
    [TOKEN_STAR]            = { NULL,                binary,         PREC_FACTOR     },
    [TOKEN_MODULO]          = { NULL,                binary,         PREC_FACTOR     },
    [TOKEN_LEFT_BRACKET]    = { list,                index,          PREC_CALL       },
    [TOKEN_RIGHT_BRACKET]   = { NULL,                NULL,           PREC_NONE       },
    [TOKEN_COLON]           = { NULL,                NULL,           PREC_NONE       },
    [TOKEN_BANG]            = { unary,               NULL,           PREC_NONE       },
    [TOKEN_BANG_EQUAL]      = { NULL,                binary,         PREC_EQUALITY   },
    [TOKEN_EQUAL]           = { NULL,                NULL,           PREC_NONE       },
    [TOKEN_EQUAL_EQUAL]     = { NULL,                binary,         PREC_EQUALITY   },
    [TOKEN_GREATER]         = { NULL,                binary,         PREC_EQUALITY   },
    [TOKEN_GREATER_EQUAL]   = { NULL,                binary,         PREC_EQUALITY   },
    [TOKEN_LESS]            = { NULL,                binary,         PREC_EQUALITY   },
    [TOKEN_LESS_EQUAL]      = { NULL,                binary,         PREC_EQUALITY   },
    [TOKEN_EXPO]            = { NULL,                binary,         PREC_FACTOR     },
    [TOKEN_PLUS_PLUS]       = { prefixIncDec,        postfixIncDec,  PREC_POSTFIX    },
    [TOKEN_MINUS_MINUS]     = { prefixIncDec,        postfixIncDec,  PREC_POSTFIX    },
    [TOKEN_PLUS_EQUAL]      = { NULL,                compoundAssign, PREC_ASSIGNMENT },
    [TOKEN_MINUS_EQUAL]     = { NULL,                compoundAssign, PREC_ASSIGNMENT },
    [TOKEN_STAR_EQUAL]      = { NULL,                compoundAssign, PREC_ASSIGNMENT },
    [TOKEN_SLASH_EQUAL]     = { NULL,                compoundAssign, PREC_ASSIGNMENT },
    [TOKEN_IDENTIFIER]      = { variable,            NULL,           PREC_NONE       },
    [TOKEN_STRING]          = { parseStringLiteral,  NULL,           PREC_NONE       },
    [TOKEN_NUMBER]          = { parseNumberLiteral,  NULL,           PREC_NONE       },
    [TOKEN_AND]             = { NULL,                and_,           PREC_AND        },
    [TOKEN_CLASS]           = { NULL,                NULL,           PREC_NONE       },
    [TOKEN_ELIF]            = { NULL,                NULL,           PREC_NONE       },
    [TOKEN_ELSE]            = { NULL,                NULL,           PREC_NONE       },
    [TOKEN_FALSE]           = { literal,             NULL,           PREC_NONE       },
    [TOKEN_FOR]             = { NULL,                NULL,           PREC_NONE       },
    [TOKEN_FUN]             = { NULL,                NULL,           PREC_NONE       },
    [TOKEN_EXTENDS]         = { NULL,                NULL,           PREC_NONE       },
    [TOKEN_IF]              = { NULL,                NULL,           PREC_NONE       },
    [TOKEN_NIL]             = { literal,             NULL,           PREC_NONE       },
    [TOKEN_OR]              = { NULL,                or_,            PREC_OR         },
    [TOKEN_RETURN]          = { NULL,                NULL,           PREC_NONE       },
    [TOKEN_SUPER]           = { super_,              NULL,           PREC_NONE       },
    [TOKEN_SELF]            = { self,                NULL,           PREC_NONE       },
    [TOKEN_TRUE]            = { literal,             NULL,           PREC_NONE       },
    [TOKEN_VAR]             = { NULL,                NULL,           PREC_NONE       },
    [TOKEN_WHILE]           = { NULL,                NULL,           PREC_NONE       },
    [TOKEN_HAS]             = { NULL,                has,            PREC_EQUALITY   },
    [TOKEN_HAS_NOT]         = { NULL,                hasNot,         PREC_EQUALITY   },
    [TOKEN_IN]              = { NULL,                NULL,           PREC_NONE       },
    [TOKEN_ENUM]            = { NULL,                NULL,           PREC_NONE       },
    [TOKEN_AS]              = { NULL,                NULL,           PREC_NONE       },
    [TOKEN_IMPORT]          = { NULL,                NULL,           PREC_NONE       },
    [TOKEN_ERROR]           = { NULL,                NULL,           PREC_NONE       },
    [TOKEN_EOF]             = { NULL,                NULL,           PREC_NONE       },
};
