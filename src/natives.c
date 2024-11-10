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
void sleepNative(int argCount, Value* args) {
    double t = AS_NUMBER(args[0]);
    sleep(t);
}

/**
 * Print native function.
 */
void printNative(int argCount, Value* args) {
    // TODO: make this more robust. Will segfault on no args.
    printValue(args[0]);
    printf("\n");
}

/**
 * Exit native function. 
 * 
 * Exits the program.
 * TODO: add optional status
 */
void exitNative(int argCount, Value* args) {
    exit(0);
}
