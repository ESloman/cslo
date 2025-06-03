/**
 * @file natives.c
 */

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
    double t = AS_NUMBER(args[0]);
    sleep(t);
    return NIL_VAL;
}

/**
 * Print native function.
 */
Value printNative(int argCount, Value* args) {
    // TODO: make this more robust. Will segfault on no args.
    printValue(args[0]);
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
    exit(0);
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
