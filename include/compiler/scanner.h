/**
 * @file scanner.h
 */

#ifndef cslo_scanner_h
#define cslo_scanner_h

/**
 * @enum TokenType
 *
 * Defines all of the available token types.
 */
typedef enum TokenType {
    // Single-character tokens.
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,
    TOKEN_MODULO,

    TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET, TOKEN_COLON,

    // One or two character tokens.
    TOKEN_BANG, TOKEN_BANG_EQUAL,
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,
    TOKEN_EXPO,

    TOKEN_PLUS_PLUS,
    TOKEN_MINUS_MINUS,
    TOKEN_PLUS_EQUAL,
    TOKEN_MINUS_EQUAL,
    TOKEN_STAR_EQUAL,
    TOKEN_SLASH_EQUAL,

    // Literals.
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

    // Keywords.
    TOKEN_AND, TOKEN_CLASS, TOKEN_ELIF, TOKEN_ELSE, TOKEN_FALSE,
    TOKEN_FOR, TOKEN_FUN, TOKEN_EXTENDS, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
    TOKEN_RETURN, TOKEN_SUPER, TOKEN_SELF,
    TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,
    TOKEN_HAS, TOKEN_HAS_NOT, TOKEN_IN,
    TOKEN_ENUM,

    TOKEN_IMPORT,

    // special
    TOKEN_ERROR, TOKEN_EOF
} TokenType;

/**
 * @struct Token
 *
 * Defines a token and stores information related to that token:
 *  - the type
 *  - pointer to the start of the token
 *  - the length of the token
 *  - the line the token appears on
 */
typedef struct Token {
    TokenType type;
    const char* start;
    int length;
    int line;
    int column;
} Token;

/**
 * Method for initialising our scanner for the given source string.
 */
void initScanner(const char* source);

/**
 * Method for scanning a token.
 */
Token scanToken();

#endif
