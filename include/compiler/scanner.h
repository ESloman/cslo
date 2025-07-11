/**
 * @file scanner.h
 */

#ifndef cslo_scanner_h
#define cslo_scanner_h

#include "compiler/tokens.h"

/**
 * Method for initialising our scanner for the given source string.
 */
void initScanner(const char* source);

/**
 * Method for scanning a token.
 */
Token scanToken();

typedef struct Scanner {
  const char* start;
  const char* current;
  int line;
} Scanner;

extern Scanner scanner;

#endif
