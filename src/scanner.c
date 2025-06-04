/**
 * @file scanner.c
 *
 * The scanner is responsible for turning source code into tokens.
 */

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct Scanner {
  const char* start;
  const char* current;
  int line;
} Scanner;

Scanner scanner;

/**
 * Method for initialising our scanner.
 *
 * Sets the start and 'current' to the start of our code and sets the line to 1.
 */
void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

/**
 * Method for determining if a character is a digit.
 */
static bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

/**
 * Method for determining if a character is a letter or _.
 */
static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

/**
 * Method for checking if we're at the end of the given source file.
 */
static bool isAtEnd() {
    return *scanner.current == '\0';
}

/**
 * Method for checking an identifier for matching keywords.
 */
static TokenType checkKeyword(int start, int length, const char* rest, TokenType type) {
    if (scanner.current - scanner.start == start + length &&
        memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

/**
 * Method for working out the identifier type.
 *
 * We check the first letter of the identifier to see if it matches a keyword;
 *  - if it does, we parse the rest of it to see if it matches anything.
 * Otherwise, we just assume we have an identifier and return.
 */
static TokenType identifierType() {
    switch (scanner.start[0]) {
        case 'a':
            return checkKeyword(1, 2, "nd", TOKEN_AND);
        case 'c':
            return checkKeyword(1, 4, "lass", TOKEN_CLASS);
        case 'e': {
            if (scanner.current - scanner.start > 1 && scanner.start[1] == 'l') {
                switch (scanner.start[2]) {
                    case 's': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
                    case 'i': return checkKeyword(1, 3, "lif", TOKEN_ELIF);
                    default:
                        break;
                }
            } else if (scanner.current - scanner.start > 1 && scanner.start[1] == 'x') {
                return checkKeyword(1, 6, "xtends", TOKEN_EXTENDS);
            }
            break;
        }
        case 'f':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a':
                        return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o':
                        return checkKeyword(2, 1, "r", TOKEN_FOR);
                    case 'u':
                        return checkKeyword(2, 2, "nc", TOKEN_FUN);
                    default:
                        break;
                }
            }
            break;
        case 'i':
            return checkKeyword(1, 1, "f", TOKEN_IF);
        case 'n':
            return checkKeyword(1, 2, "il", TOKEN_NIL);
        case 'o':
            return checkKeyword(1, 1, "r", TOKEN_OR);
        case 'r':
            return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
        case 's':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'e':
                        return checkKeyword(2, 2, "lf", TOKEN_SELF);
                    case 'u':
                        return checkKeyword(2, 3, "per", TOKEN_SUPER);
                    default:
                        break;
                }
            }
            break;
        case 't': {
            return checkKeyword(1, 3, "rue", TOKEN_TRUE);
        }
        case 'v':
            return checkKeyword(1, 2, "ar", TOKEN_VAR);
        case 'w':
            return checkKeyword(1, 4, "hile", TOKEN_WHILE);
        default:
            return TOKEN_IDENTIFIER;
    }
    return TOKEN_IDENTIFIER;
}

/**
 * Method for advancing the scanner.
 *
 * This advances the pointer and returns the character we just consumed.
 */
static char advance() {
  scanner.current++;
  return scanner.current[-1];
}

/**
 * Method for peeking at the current character.
 *
 * Returns the character at the scanner's 'current' position.
 */
static char peek() {
  return *scanner.current;
}

/**
 * Method for peeking at the next character.
 *
 * Returns the next character we're about to consume.
 */
static char peekNext() {
    if (isAtEnd()) {
        return '\0';
    }
    return scanner.current[1];
}

/**
 * Method for matching and consuming a character.
 *
 * If we match the given character, we advance the scanner to consume it
 * and return true.
 */
static bool match(char expected) {
    if (isAtEnd()) {
        return false;
    }
    if (*scanner.current != expected) {
        return false;
    }
    scanner.current++;
    return true;
}

/**
 * Method for making a token.
 *
 * Given the token type, initialise a Token and set
 * all the fields to the expected values.
 *
 * The lexeme for the token is captured with the start + length
 * values.
 */
static Token makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

/**
 * Method for making an error token.
 *
 * Similarly to makeToken, creates a token with the type TOKEN_ERROR.
 * The 'lexeme' is set to the error message provided.
 */
static Token errorToken(const char* message) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = scanner.line;
  return token;
}

/**
 * Method for skipping whitespace.
 *
 * Whilst there are whitespace characters, consume them.
 * This also handles comments that start with double slashes and '#' by also just skipping them.
 */
static void skipWhitespace() {
    for (;;) {
        char c = peek();
        switch (c) {
        case ' ':
        case '\r':
        case '\t': {
            advance();
            break;
        }
        case '\n': {
            scanner.line++;
            advance();
            break;
        }
        case '/': {
            if (peekNext() == '/') {
                while (peek() != '\n' && !isAtEnd()) {
                    advance();
                }
            } else {
                return;
            }
            break;
        }
        case '#': {
            while (peek() != '\n' && !isAtEnd()) {
                advance();
            }
            break;
        }
        default:
            return;
        }
    }
}

/**
 * Method for scanning and creating a string token.
 *
 * Will consume characters until we reach the closing quote.
 */
static Token string() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            scanner.line++;
        }
        advance();
    }

    if (isAtEnd()) {
        return errorToken("Unterminated string.");
    }

    // The closing quote.
    advance();
    return makeToken(TOKEN_STRING);
}

/**
 * Method for scanning and creating a number token.
 *
 */
static Token number() {
    while (isDigit(peek())) {
        advance();
    }

    if (peek() == '.' && isDigit(peekNext())) {
        // consume the '.'
        advance();
    }

    while (isDigit(peek())) {
        advance();
    }

    return makeToken(TOKEN_NUMBER);
}

/**
 * Method for scanning and creating an identifier token.
 */
static Token identifier() {
    while (isAlpha(peek()) || isDigit(peek())) {
        advance();
    }
    return makeToken(identifierType());
}

/**
 * Method for scanning a token.
 *
 * Set scanner.start to the current token so know where
 * the lexeme we're about to scan starts.
 *
 * The actual token scanning is a big switch statement to handle
 * different possible token types.
 *
 *
 * If we get to the end and haven't successfully scanned a token;
 * we must therefore have encountered an unexpected character and
 * we indicate as such.
 */
Token scanToken() {
    skipWhitespace();
    scanner.start = scanner.current;
    if (isAtEnd()) {
        return makeToken(TOKEN_EOF);
    }

    char c = advance();

    if (isAlpha(c)) {
        return identifier();
    }

    if (isDigit(c)) {
        return number();
    }

    switch (c) {
        // single char tokens
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '[': return makeToken(TOKEN_LEFT_BRACKET);
        case ']': return makeToken(TOKEN_RIGHT_BRACKET);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case ';': return makeToken(TOKEN_SEMICOLON);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return makeToken(TOKEN_DOT);
        case '%': return makeToken(TOKEN_MODULO);
        case ':': return makeToken(TOKEN_COLON);

        // these tokens can either be single or two char tokens
        // aka ! or !=
        // simply match on the possible second token
        // if true, return the valid two char token, if not the
        // single char token
        case '+': {
            if (match('+')) return makeToken(TOKEN_PLUS_PLUS);
            if (match('=')) return makeToken(TOKEN_PLUS_EQUAL);
            return makeToken(TOKEN_PLUS);
        }
        case '-': {
            if (match('-')) return makeToken(TOKEN_MINUS_MINUS);
            if (match('=')) return makeToken(TOKEN_MINUS_EQUAL);
            return makeToken(TOKEN_MINUS);
        }
        case '*': {
            if (match('=')) return makeToken(TOKEN_STAR_EQUAL);
            if (match('*')) return makeToken(TOKEN_EXPO);
            return makeToken(TOKEN_STAR);
        }
        case '/': {
            return makeToken(match('=') ? TOKEN_SLASH_EQUAL : TOKEN_SLASH);
        }
        case '!': {
            return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        }
        case '=': {
            return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        }
        case '<': {
            return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        }
        case '>':
        {
            return makeToken(
                match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER
            );
        }
        // literal matching
        case '"': return string();

        default:
            // break into the error token
            break;
    }

    return errorToken("Unexpected character.");
}
