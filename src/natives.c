/**
 * @file natives.c
 */

#define _POSIX_C_SOURCE 200112L

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "natives.h"
#include "object.h"
#include "value.h"
#include "vm.h"

/**
 * Method for defining all our natives.
 */
void defineNatives() {
    defineNative("clock", clockNative);
    defineNative("exit", exitNative);
    defineNative("sleep", sleepNative);
    defineNative("time", timeNative);
    defineNative("print", printNative);

    // math functions
    defineNative("abs", absNative);
    defineNative("ceil", ceilNative);
    defineNative("floor", floorNative);
    defineNative("sqrt", sqrtNative);
    defineNative("sin", sinNative);
    defineNative("cos", cosNative);
    defineNative("tan", tanNative);
    defineNative("min", minNative);
    defineNative("max", maxNative);

    // random functions
    defineNative("random", randomNative);
    defineNative("randomInt", randomIntNative);
    defineNative("randomRange", randomRangeNative);
    defineNative("randomSeed", randomSeedNative);

    // os functions
    defineNative("getenv", getEnvNative);
    defineNative("setenv", setEnvNative);
    defineNative("unsetenv", unsetEnvNative);
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
    return NUMBER_VAL((double)time(NULL));
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
 * TODO: add optional status
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
 * Calculates the ceil of a number.
 * If the argument is not a number, returns NIL_VAL or throws an error.
 */
Value ceilNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL;
    }
    double value = AS_NUMBER(args[0]);
    return NUMBER_VAL(ceil(value));
}

/**
 * Calculates the floor of a number.
 * If the argument is not a number, returns NIL_VAL or throws an error.
 */
Value floorNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL;
    }
    double value = AS_NUMBER(args[0]);
    return NUMBER_VAL(floor(value));
}

/**
 * Calculates the square root of a number.
 * If the argument is not a number, returns NIL_VAL or throws an error.
 */
Value sqrtNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL;
    }
    double value = AS_NUMBER(args[0]);
    if (value < 0) {
        return ERROR_VAL;
    }
    return NUMBER_VAL(sqrt(value));
}

/**
 * Calculates the sine of a number.
 * If the argument is not a number, returns NIL_VAL or throws an error.
 */
Value sinNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL;
    }
    double value = AS_NUMBER(args[0]);
    return NUMBER_VAL(sin(value));
}

/**
 * Calculates the cosine of a number.
 * If the argument is not a number, returns NIL_VAL or throws an error.
 */
Value cosNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL;
    }
    double value = AS_NUMBER(args[0]);
    return NUMBER_VAL(cos(value));
}

/**
 * Calculates the tangent of a number.
 * If the argument is not a number, returns NIL_VAL or throws an error.
 */
Value tanNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL;
    }
    double value = AS_NUMBER(args[0]);
    return NUMBER_VAL(tan(value));
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

// RANDOM FUNCTIONS

/**
 * Generates a random number between 0 and 1.
 */
Value randomNative(int argCount, Value* args) {
    if (argCount != 0) {
        return ERROR_VAL;
    }
    return NUMBER_VAL((double)rand() / RAND_MAX);
}

/**
 * Generates a random integer between the given range.
 * If the arguments are not numbers, returns ERROR_VAL or throws an error.
 */
Value randomIntNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        return ERROR_VAL;
    }
    double min = AS_NUMBER(args[0]);
    double max = AS_NUMBER(args[1]);
    if (min > max) {
        return ERROR_VAL; // or handle error
    }
    int range = (int)(max - min + 1);
    return NUMBER_VAL(min + (rand() % range));
}

/**
 * Generates a random number between the given range.
 * If the arguments are not numbers, returns ERROR_VAL or throws an error.
 */
Value randomRangeNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        return ERROR_VAL;
    }
    double min = AS_NUMBER(args[0]);
    double max = AS_NUMBER(args[1]);
    if (min > max) {
        return ERROR_VAL; // or handle error
    }
    double range = max - min;
    return NUMBER_VAL(min + ((double)rand() / RAND_MAX) * range);
}

/**
 * Sets the seed for the random number generator.
 */
Value randomSeedNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL;
    }
    unsigned int seed = (unsigned int)AS_NUMBER(args[0]);
    srand(seed);
    return NIL_VAL;
}

/**
 * Gets the value of an environment variable.
 * Usage: getenv("HOME")
 */
Value getEnvNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL;
    }
    const char* name = AS_CSTRING(args[0]);
    char* value = getenv(name);
    if (value == NULL) {
        return NIL_VAL;
    }
    return OBJ_VAL(copyString(value, (int)strlen(value)));
}

/**
 * Sets the value of an environment variable.
 * Usage: setenv("MYVAR", "value")
 */
Value setEnvNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_STRING(args[0]) || !IS_STRING(args[1])) {
        return ERROR_VAL;
    }
    const char* name = AS_CSTRING(args[0]);
    const char* value = AS_CSTRING(args[1]);
#if defined(_WIN32)
    // Windows uses _putenv_s
    int result = _putenv_s(name, value);
#else
    // POSIX uses setenv
    int result = setenv(name, value, 1);
#endif
    if (result != 0) {
        return ERROR_VAL;
    }
    return NIL_VAL;
}

/**
 * Unsets an environment variable.
 * Usage: unsetenv("MYVAR")
 */
Value unsetEnvNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL;
    }
    const char* name = AS_CSTRING(args[0]);
#if defined(_WIN32)
    // Windows uses _putenv_s with an empty value
    int result = _putenv_s(name, "");
#else
    // POSIX uses unsetenv
    int result = unsetenv(name);
#endif
    if (result != 0) {
        return ERROR_VAL;
    }
    return NIL_VAL;
}
