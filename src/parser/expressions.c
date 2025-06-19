/**
 * @file expressions.c
 * This file contains the methods for parsing expressions in the CSLO language.
 * It includes methods for parsing various types of expressions such as literals,
 * variables, and control flow statements.
 */

#include "stdbool.h"
#include "stdlib.h"

#include "core/chunk.h"
#include "compiler/codegen.h"
#include "compiler/parser.h"
#include "compiler/scanner.h"
#include "parser/expressions.h"

 /**
  * Method for parsing a dictionary literal.
  */
void parseDictLiteral(bool canAssign) {
    int kVPairsCount = 0;
    if (!checkToken(TOKEN_RIGHT_BRACE)) {
        do {
            parseExpression(); // Parse key
            consumeToken(TOKEN_COLON, "Expected ':' after dict key.");
            parseExpression(); // Parse value
            kVPairsCount++;
        } while (matchToken(TOKEN_COMMA));
    }
    consumeToken(TOKEN_RIGHT_BRACE, "Expected '}' after dict literal.");
    emitByte(OP_DICT, parser.previous.line);
    emitByte((kVPairsCount >> 8) & 0xff, parser.previous.line);
    emitByte(kVPairsCount & 0xff, parser.previous.line);
}

/**
 * @brief Compiles a numeric literal.
 *
 * Emits bytecode to load the number onto the stack.
 *
 * @param canAssign Indicates if assignment is allowed (unused).
 */
void parseNumberLiteral(bool canAssign) {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

/**
 * @brief Compiles a string literal.
 *
 * Emits bytecode to load the string onto the stack.
 *
 * @param canAssign Indicates if assignment is allowed (unused).
 */
void parseStringLiteral(bool canAssign) {
    emitConstant(OBJ_VAL(copyString(parser.previous.start +1, parser.previous.length - 2)));
}
