/**
 * @file statements.h
 * This file contains the declarations for the statements in the CSLO language.
 */

#ifndef cslo_statements_h
#define cslo_statements_h

/**
 * Method for parsing a statement.
 */
void parseStatement();

/**
 * Method for compiling expression statements.
 */
void parseExpressionStatement();

/**
 * Method for compiling if statements.
 *
 * This includes compiling the accompanying elif and else statements.
 * We use jumping and patching here for flow so our VM knows where to jump to and from.
 */
void ifStatement();

/**
 * Method for compiling while statements.
 */
void whileStatement();

/**
 * Method for compiling for statements.
 */
void forStatement();

/**
 * Method for compiling a return statement.
 */
void returnStatement();


#endif
