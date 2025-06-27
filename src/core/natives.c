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

#include "util.h"

/**
 * Method for defining all our natives.
 */
void defineNatives() {
    defineNative("clock", clockNative, 0, 0, NULL);
    defineNative("exit", exitNative, 0, 1, ((ParamInfo[]){{copyString("code", 4), false}}));
    defineNative("sleep", sleepNative, 1, 1, ((ParamInfo[]){{copyString("seconds", 7), true}}));
    defineNative("time", timeNative, 0, 0, NULL);
    defineNative("print", printNative, 1, -1, ((ParamInfo[]){{copyString("message", 7), true}, {copyString("...args", 7), false}}));
    defineNative("println", printLNNative, 1, -1, ((ParamInfo[]){{copyString("message", 7), true}, {copyString("...args", 7), false}}));
    defineNative("len", lenNative, 1, 1, ((ParamInfo[]){{copyString("sequence", 9), true}}));

    // math functions
    defineNative("abs", absNative, 1, 1, ((ParamInfo[]){{copyString("value", 5), true}}));
    defineNative("min", minNative, 2, 2, ((ParamInfo[]){{copyString("a", 1), true}, {copyString("b", 1), true}}));
    defineNative("max", maxNative, 2, 2, ((ParamInfo[]){{copyString("a", 1), true}, {copyString("b", 1), true}}));
}

/**
 * Method for defining a native function.
 * We wrap the C function pointer as an ObjNative and adds it to the globals
 * table with the gven name.
 *
 * Pushes the name and native values onto the stack to prevent GC.
 */
void defineNative(const char* name, NativeFn function, int arityMin, int arityMax, ParamInfo* params) {
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function, arityMin, arityMax, params)));
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
        return ERROR_VAL_PTR("sleep() expects a single numeric argument.");
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
        return ERROR_VAL_PTR("print() expects at least one argument.");
    }
    for (int i = 0; i < argCount; i++) {
        if (IS_STRING(args[i])) {
            ObjString* str = AS_STRING(args[i]);
            size_t unescLen;
            char* unesc = unescapeString(str->chars, str->length, &unescLen);
            printf("%s", unesc);
            free(unesc);
        } else {
            printValue(args[i]);
        }
    }
    // printf("\n");
    return NIL_VAL;
}

/**
 * Println native function.
 */
Value printLNNative(int argCount, Value* args) {
    if (argCount < 1) {
        return ERROR_VAL_PTR("print() expects at least one argument.");
    }
    printNative(argCount, args);
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
        return ERROR_VAL_PTR("exit() expects a numeric argument (if any).");
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
        return ERROR_VAL_PTR("len() expects a single argument of type string, list, or dict.");
    }
    switch (OBJ_TYPE(args[0])) {
        case OBJ_STRING:
            return NUMBER_VAL((double)AS_STRING(args[0])->length);
        case OBJ_LIST:
            return NUMBER_VAL((double)AS_LIST(args[0])->count);
        case OBJ_DICT:
            return NUMBER_VAL((double)AS_DICT(args[0])->data.count);
        default:
            return ERROR_VAL_PTR("len() expects a single argument of type string, list, or dict.");
    }
}

// MATH FUNCTIONS

/**
 * Calculates the absolute value of a number.
 * If the argument is not a number, returns NIL_VAL or throws an error.
 */
Value absNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL_PTR("abs() expects a single numeric argument.");
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
        return ERROR_VAL_PTR("min() expects two numeric arguments.");
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
        return ERROR_VAL_PTR("max() expects two numeric arguments.");
    }
    double a = AS_NUMBER(args[0]);
    double b = AS_NUMBER(args[1]);
    return NUMBER_VAL(a > b ? a : b);
}
