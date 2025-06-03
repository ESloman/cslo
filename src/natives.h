/**
 * @file natives.h
 */

#include "object.h"
#include "value.h"

/**
 * Method for defining a native function.
 */
void defineNative(const char* name, NativeFn function);

/**
 * Method for defining all the natives.
 */
void defineNatives();

/**
 * Clock native function.
 */
Value clockNative(int argCount, Value* args);

/**
 * Print native function.
 */
Value printNative(int argCount, Value* args);

/**
 * Sleep native function.
 */
Value sleepNative(int argCount, Value* args);

/**
 * Time native function.
 * Returns the number of seconds since epoch.
 */
Value timeNative(int argCount, Value* args);

/**
 * Exit native function.
 * Exits the current running script or REPL.
 */
Value exitNative(int argCount, Value* args);

/**
 * len native function.
 */
Value lenNative(int argCount, Value* args);

/**
 * append native function.
 */
Value appendNative(int argCount, Value* args);

/**
 * Pop native function.
 */
Value popNative(int argCount, Value *args);
