/**
 * @file errors.c
 * @brief Implementation of error handling utilities for the project.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "core/errors.h"

static const char* errorTypeToString(enum ErrorType type) {
    switch (type) {
        case ERROR_RUNTIME:
            return "RuntimeException";
        case ERROR_SYNTAX:
            return "SyntaxException";
        case ERROR_TYPE:
            return "TypeException";
        case ERROR_ATTRIBUTE:
            return "AttributeException";
        case ERROR_IO:
            return "IOException";
        case ERROR_MEMORY:
            return "MemoryException";
        case ERROR_INDEX:
            return "IndexException";
        case ERROR_IMPORT:
            return "ImportException";
        case ERROR_NAME:
            return "NameException";
        case ERROR_ASSERTION:
            return "AssertionException";
        default:
            return "UnknownException";
    }
}

/**
 * @brief Reports an error to the standard error output.
 *
 * This function takes an Exception pointer and prints its details to stderr.
 * If the pointer is NULL, it does nothing.
 *
 * @param exc Pointer to the Exception to report.
 */
void reportError(Exception* exc) {
    if (exc == NULL) {
        return;
    }

    fprintf(stderr, "[%s] %s at %s:%d:%d\n", errorTypeToString(exc->type), exc->message, exc->file, exc->line, exc->column);

    // Print the actual source line if possible
    if (exc->file && exc->line > 0) {
        FILE* f = fopen(exc->file, "r");
        if (f) {
            char buf[512];
            int current = 1;
            while (fgets(buf, sizeof(buf), f)) {
                if (current == exc->line) {
                    // Remove trailing newline for cleaner output
                    size_t len = strlen(buf);
                    if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
                    fprintf(stderr, "    %d | %s\n", exc->line, buf);
                    // Optionally print a caret at the column
                    if (exc->column > 0) {
                        fprintf(stderr, "    %*s^\n", exc->column + (int)log10(exc->line) + 3, "");
                    }
                    break;
                }
                current++;
            }
            fclose(f);
        }
    }

    if (exc->stacktrace && exc->stacktrace[0]) {
        fprintf(stderr, "Stack trace:\n%s", exc->stacktrace);
    }
}
