/**
 * @file expressions.h
 * This file contains the methods for parsing expressions in the CSLO language.
 * It includes methods for parsing various types of expressions such as literals,
 * variables, and control flow statements.
 */

#ifndef cslo_expressions_h
#define cslo_expressions_h

#include "stdbool.h"

/**
 * Method for parsing a dictionary literal.
 */
void parseDictLiteral(bool canAssign);

#endif
