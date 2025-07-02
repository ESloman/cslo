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
    // emitConstant(OBJ_VAL(copyString(parser.previous.start +1, parser.previous.length - 2)));
    const char* start = parser.previous.start + 1;
    const char* end = start + parser.previous.length - 2;
    const char* curr = start;
    int chunkCount = 0;

    while (curr < end) {
        if (curr[0] == '$' && curr[1] == '{') {
            // Emit string chunk before ${
            if (curr > start) {
                emitConstant(OBJ_VAL(copyString(start, curr - start)));
                chunkCount++;
            }
            curr += 2; // skip ${
            // Parse the expression inside ${...}
            const char* exprStart = curr;
            int braceDepth = 1;
            while (curr < end && braceDepth > 0) {
                if (curr[0] == '{') braceDepth++;
                else if (curr[0] == '}') braceDepth--;
                curr++;
            }
            int exprLen = (curr - exprStart) - 1; // exclude closing }
            // Parse the expression in exprStart[0..exprLen]
            // Option 1: Temporarily re-init scanner/parser for this substring
            // Option 2: Add a helper to parse an expression from a string
            parseEmbeddedExpression(exprStart, exprLen);
            chunkCount++;
            start = curr;
        } else {
            curr++;
        }
    }
    // Emit remaining string chunk, if any
    if (start < end) {
        emitConstant(OBJ_VAL(copyString(start, end - start)));
        chunkCount++;
    }

    if (chunkCount == 0) {
        emitConstant(OBJ_VAL(copyString("", 0))); // Emit empty string if nothing was emitted
    } else {
        for (int i = 1; i < chunkCount; i++) {
            emitByte(OP_INTERPOLATE, parser.previous.line);
        }
    }
}
