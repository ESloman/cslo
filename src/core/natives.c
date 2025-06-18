/**
 * @file natives.c
 */

#define _POSIX_C_SOURCE 200112L

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "core/natives.h"
#include "core/object.h"
#include "core/value.h"
#include "core/vm.h"

/**
 * Method for defining all our natives.
 */
void defineNatives() {
    defineNative("clock", clockNative);
    defineNative("exit", exitNative);
    defineNative("sleep", sleepNative);
    defineNative("time", timeNative);
    defineNative("print", printNative);
    defineNative("len", lenNative);

    // math functions
    defineNative("abs", absNative);
    defineNative("min", minNative);
    defineNative("max", maxNative);
}

/**
 * Method for defining a native function.
 * We wrap the C function pointer as an ObjNative and adds it to the globals
 * table with the gven name.
 *
 * Pushes the name and native values onto the stack to prevent GC.
 */
void defineNative(const char* name, NativeFn function) {
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, OBJ_VAL(AS_STRING(vm.stack[0])), vm.stack[1]);
    pop();
    pop();
}

/**
 * Clock native function.
 */
Value clockNative(int argCount, Value* args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

/**
 * Time native function.
 * Returns the number of seconds since epoch.
 */
Value timeNative(int argCount, Value* args) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return NUMBER_VAL((double)tv.tv_sec + (double)tv.tv_usec / 1e6);
}

/**
 * Sleep native function.
 */
Value sleepNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL;
    }
    double t = AS_NUMBER(args[0]);
    sleep(t);
    return NIL_VAL;
}

/**
 * Print native function.
 */
Value printNative(int argCount, Value* args) {
    if (argCount < 1) {
        return ERROR_VAL;
    }
    for (int i = 0; i < argCount; i++) {
        printValue(args[i]);
    }
    printf("\n");
    return NIL_VAL;
}

/**
 * Exit native function.
 *
 * Exits the program.
 */
Value exitNative(int argCount, Value* args) {
    if (argCount > 0 && !IS_NUMBER(args[0])) {
        return ERROR_VAL;
    }
    int status = 0;
    if (0 < argCount) {
        status = (int) AS_NUMBER(args[0]);
    }
    exit(status);
    // not reachable
    return NIL_VAL;
}

/**
 * len native function.
 */
Value lenNative(int argCount, Value* args) {
    if (argCount != 1 || (!IS_LIST(args[0]) && !IS_STRING(args[0]) && !IS_DICT(args[0]))) {
        printf("len() expects a single argument of type string, list, or dict.\n");
        return ERROR_VAL;
    }
    switch (OBJ_TYPE(args[0])) {
        case OBJ_STRING:
            return NUMBER_VAL((double)AS_STRING(args[0])->length);
        case OBJ_LIST:
            return NUMBER_VAL((double)AS_LIST(args[0])->count);
        case OBJ_DICT:
            return NUMBER_VAL((double)AS_DICT(args[0])->data.count);
        default:
            printf("len() expects a single argument of type string or list.\n");
            return ERROR_VAL;
    }
}

// MATH FUNCTIONS

/**
 * Calculates the absolute value of a number.
 * If the argument is not a number, returns NIL_VAL or throws an error.
 */
Value absNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL;
    }
    double value = AS_NUMBER(args[0]);
    return NUMBER_VAL(value < 0 ? -value : value);
}

/**
 * Calculates the minimum of two numbers.
 * If the arguments are not numbers, returns NIL_VAL or throws an error.
 */
Value minNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        return ERROR_VAL;
    }
    double a = AS_NUMBER(args[0]);
    double b = AS_NUMBER(args[1]);
    return NUMBER_VAL(a < b ? a : b);
}

/**
 * Calculates the maximum of two numbers.
 * If the arguments are not numbers, returns NIL_VAL or throws an error.
 */
Value maxNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        return ERROR_VAL;
    }
    double a = AS_NUMBER(args[0]);
    double b = AS_NUMBER(args[1]);
    return NUMBER_VAL(a > b ? a : b);
}
