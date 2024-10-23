/**
 * @file repl.c
 * 
 * Methods for handling cslo's REPL
 */

#include <stdio.h>

#include "vm.h"

/**
 * Main REPL method.
 */
void repl() {
    char line[1024];
    for (;;) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        interpret(line);

    }
}
