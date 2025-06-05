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
    defineNative("len", lenNative);

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
    if (argCount != 1 || (!IS_LIST(args[0]) && !IS_STRING(args[0]))) {
        printf("len() expects a single argument of type string or list.\n");
        return NIL_VAL;
    }
    switch (OBJ_TYPE(args[0])) {
        case OBJ_STRING:
             return NUMBER_VAL((double)AS_STRING(args[0])->length);
            break;
        case OBJ_LIST:
            return NUMBER_VAL((double)AS_LIST(args[0])->count);
            break;
        default:
            printf("len() expects a single argument of type string or list.\n");
            return NIL_VAL;
    }
}

// LIST FUNCTIONS

/**
 * Append native function.
 */
Value appendNative(int argCount, Value *args) {
    if (argCount != 2 || !IS_LIST(args[0])) {
        printf("append() must be called on a list with one argument.");
        return NIL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    if (list->count + 1 > list->values.capacity) {
        growValueArray(&list->values);
    }
    list->values.values[list->count] = args[1];
    list->count++;
    list->values.count = list->count;
    return NIL_VAL;
}

/**
 * Pop native function.
 */
Value popNative(int argCount, Value *args) {
    if (argCount != 1 || !IS_LIST(args[0])) {
        printf("pop() must be called on a list.");
        return NIL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    if (list->count == 0) {
        // popping an empty list returns nil
        return NIL_VAL;
    }
    Value val = list->values.values[list->count - 1];
    list->values.values[list->count - 1] = NIL_VAL; // Clear the popped value
    list->count--;
    list->values.count = list->count;
    return val;
}

/**
 * Insert native function.
 */
Value insertNative(int argCount, Value* args) {
    if (argCount != 3 || !IS_LIST(args[0]) || !IS_NUMBER(args[1])) {
        printf("insert() must be called on a list with an index and a value.\n");
        return NIL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    int idx = (int)AS_NUMBER(args[1]);
    if (idx < 0) {
        idx = 0;
    }
    if (idx > list->count) {
        idx = list->count; // allow insert at end
    }

    #ifdef DEBUG_LOGGING
    printf("insert: idx=%d, value=", idx);
    printValue(args[2]);
    printf("\n");
    #endif

    // Grow the array if needed
    if (list->count + 1 > list->values.capacity) {
        growValueArray(&list->values);
    }

    // Shift elements to the right
    #ifdef DEBUG_LOGGING
    printf("Inserting at idx=%d, count=%d\n", idx, list->count);
    for (int i = list->count; i > idx; i--) {
        printf("shifting %d to %d\n", i-1, i);
        list->values.values[i] = list->values.values[i - 1];
    }
    #endif
    list->values.values[idx] = args[2];
    list->count++;
    list->values.count = list->count;
    return NIL_VAL;
}

/**
 * remove native function.
 */
Value removeNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_LIST(args[0]) || !IS_NUMBER(args[1])) {
        printf("remove() must be called on a list with an index.\n");
        return NIL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    int idx = (int)AS_NUMBER(args[1]);
    if (idx < 0 || idx >= list->count) {
        printf("Index out of bounds for remove().\n");
        return NIL_VAL;
    }

    Value removed = list->values.values[idx];

    // Shift elements to the left
    for (int i = idx; i < list->count - 1; i++) {
        list->values.values[i] = list->values.values[i + 1];
    }
    list->count--;
    list->values.count = list->count;
    list->values.values[list->count] = NIL_VAL; // Clear the slot
    return removed;
}

/**
 * Reverse native function.
 */
Value reverseNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_LIST(args[0])) {
        printf("reverse() must be called on a list.\n");
        return NIL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    if (list->count == 0 || list->count == 1) {
        // nothing to reverse; can just return current list as is
        return args[0];
    }
    ObjList* reversed = newList();
    for (int i = list->count - 1; i >= 0; i--) {
        if (reversed->count >= reversed->values.capacity) {
            growValueArray(&reversed->values);
        }
        reversed->values.values[reversed->count++] = list->values.values[i];
    }
    return OBJ_VAL(reversed);
}

/**
 * Index native function.
 */
Value indexNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_LIST(args[0])) {
        printf("index() must be called on a list with a value.\n");
        return NIL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    if (list->count == 0) {
        printf("index() called on an empty list.\n");
        return NIL_VAL;
    }

    for (int idx = 0; idx < list->count; idx++) {
        if (valuesEqual(list->values.values[idx], args[1])) {
            return NUMBER_VAL((double)idx);
        }
    }
    return NIL_VAL; // not found
}

/**
 * Count native function.
 */
Value countNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_LIST(args[0])) {
        printf("count() must be called on a list with a value.\n");
        return NIL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    if (list->count == 0) {
        // guaranteed to be 0 occurences
        return NUMBER_VAL(0);
    }

    int occurences = 0;
    for (int idx = 0; idx < list->count; idx++) {
        if (valuesEqual(list->values.values[idx], args[1])) {
            occurences++;
        }
    }
    return NUMBER_VAL((double)occurences);
}

/**
 * Clear native function.
 * Removes all elements from the list.
 */
Value clearNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_LIST(args[0])) {
        printf("clear() must be called on a list.\n");
        return NIL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    // Set all elements to NIL_VAL to avoid holding references
    for (int i = 0; i < list->count; i++) {
        list->values.values[i] = NIL_VAL;
    }
    list->count = 0;
    list->values.count = 0;
    return NIL_VAL;
}

/**
 * Clone native function.
 * Shallow clones/copies a list.
 */
Value cloneNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_LIST(args[0])) {
        printf("clone() must be called on a list.\n");
        return NIL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    ObjList* clone = newList();
    for (int i = 0; i < list->count; i++) {
        if (clone->count + 1 > clone->values.capacity) {
            growValueArray(&clone->values);
        }
        clone->values.values[clone->count++] = list->values.values[i];
        clone->values.count = clone->count;
    }
    return OBJ_VAL(clone);
}

/**
 * Extends native function.
 * Extends a list with another list.
 */
Value extendNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_LIST(args[0]) || !IS_LIST(args[1])) {
        printf("extend() must be called on a list with a list.\n");
        return NIL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    ObjList* other = AS_LIST(args[1]);

    while (list->values.capacity < list->count + other->count) {
        growValueArray(&list->values);
    }

    for (int i = 0; i < other->count; i++) {
        list->values.values[list->count++] = other->values.values[i];
    }
    list->values.count = list->count;
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
