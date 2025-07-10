/**
 * @file repl.c
 *
 * Methods for handling cslo's REPL
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <sys/utsname.h>
#endif

#include "core/vm.h"
#include "linenoise.h"

#include "version.h"

static const char* get_history_path() {
    const char* home = getenv("HOME");
    if (!home) home = ".";
    static char path[512];
    snprintf(path, sizeof(path), "%s/.cslo_history", home);
    return path;
}

/**
 * Main REPL method.
 */
void repl() {
    char* line;
    const char* historyPath = get_history_path();

    linenoiseHistoryLoad(historyPath);

    /** TODO: print out environment, version, etc here. */
    printf("Welcome to slo!\n");
#if defined(_WIN32)
    printf("Running on: Windows\n");
#else
    struct utsname sysinfo;
    if (uname(&sysinfo) == 0) {
        printf("Running on: %s %s\n", sysinfo.sysname, sysinfo.release);
    } else {
        printf("Running on: Unknown OS\n");
    }
#endif
    printf("slo version %s.\n", SLO_VERSION);
    printf("Type 'exit' to quit.\n\n");

    for (;;) {
        line = linenoise(">> ");
        if (line == NULL) {
            printf("\n");
            break;
        }
        // Special exit handling for REPL.
        if (strcmp(line, "exit") == 0) {
            linenoiseHistorySave(historyPath);
            linenoiseFree(line);
            exit(0);
        }
        if (line[0] != '\0') {
            linenoiseHistoryAdd(line);
            linenoiseHistorySave(historyPath);
            interpret(line, NULL);
        }
        linenoiseFree(line);
    }
}
