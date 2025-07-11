/**
 * @file keywords.h
 * @brief Header file for keyword handling in the scanner.
 */

#ifndef cslo_keywords_h
#define cslo_keywords_h

#include "compiler/tokens.h"

/**
 * @struct Keyword
 * @brief Represents a keyword in the language.
 */
typedef struct {
    const char* keyword;
    TokenType type;
} Keyword;

/**
 * @array keywords
 *
 * @brief Array of keywords used in the language.
 *
 * This array maps keywords to their corresponding token types.
 * It is used to identify keywords during the scanning process.
 * Each keyword is represented as a string and associated with a TokenType.
 */
static const Keyword keywords[] = {
    {"and",     TOKEN_AND},
    {"as",      TOKEN_AS},
    {"assert",  TOKEN_ASSERT},
    {"break",   TOKEN_BREAK},
    {"class",   TOKEN_CLASS},
    {"continue",TOKEN_CONTINUE},
    {"else",    TOKEN_ELSE},
    {"elif",    TOKEN_ELIF},
    {"enum",    TOKEN_ENUM},
    {"extends", TOKEN_EXTENDS},
    {"false",   TOKEN_FALSE},
    {"final",   TOKEN_FINAL},
    {"for",     TOKEN_FOR},
    {"func",    TOKEN_FUN},
    {"has",     TOKEN_HAS},
    {"if",      TOKEN_IF},
    {"import",  TOKEN_IMPORT},
    {"in",      TOKEN_IN},
    {"nil",     TOKEN_NIL},
    {"or",      TOKEN_OR},
    {"return",  TOKEN_RETURN},
    {"self",    TOKEN_SELF},
    {"super",   TOKEN_SUPER},
    {"true",    TOKEN_TRUE},
    {"var",     TOKEN_VAR},
    {"while",   TOKEN_WHILE},
};

#define KEYWORD_COUNT (sizeof(keywords)/sizeof(keywords[0]))

#endif
