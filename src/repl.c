/**
 * @file repl.c
 * 
 * Methods for handling cslo's REPL
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm.h"

/**
 * Main REPL method.
 */
void repl() {
    char line[1024];

    /** TODO: print out environment, version, etc here. */

    for (;;) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        /**
         * Special exit handling for REPL.
         * Prevents having to write out fully 'exit();'.
         */
        if (strcmp(line, "exit\n") == 0) {
            exit(0);
        }

        interpret(line);

    }
}
