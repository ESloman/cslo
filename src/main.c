/**
 * @file main.c
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/common.h"
#include "core/chunk.h"
#include "core/debug.h"
#include "runtime/repl.h"
#include "core/vm.h"

#include "version.h"

/**
 * Method for reading a slo file.
 */
static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    if (fileSize == 0) {
        fclose(file);
        return strdup("");
    }

    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }

    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

/**
 * Method for running a slo file.
 */
static void runFile(const char* path) {
    char* source = readFile(path);
    InterpretResult result = interpret(source, path);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

/**
 * Our main entry point for slo.
 *
 * This is currently our REPL handler and program runner.
 */
int main(int argc, const char* argv[]) {
    initVM();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
            printf("slo version %s\n", SLO_VERSION);
            return 0;
        }
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: cslo [path] [--version]\n");
        exit(64);
    }

    freeVM();

    return 0;
}
