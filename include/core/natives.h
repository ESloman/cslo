/**
 * @file natives.h
 */

#include "core/object.h"
#include "core/value.h"

/**
 * Method for defining a native function.
 */
void defineNative(const char* name, NativeFn function, int arityMin, int arityMax, ParamInfo* params);

/**
 * Method for defining all the natives.
 */
void defineNatives();

/**
 * Clock native function.
 */
Value clockNative(int argCount, Value* args, ParamInfo* params);

/**
 * Print native function.
 */
Value printNative(int argCount, Value* args, ParamInfo* params);

/**
 * Println native function.
 */
Value printLNNative(int argCount, Value* args, ParamInfo* params);

/**
 * Sleep native function.
 */
Value sleepNative(int argCount, Value* args, ParamInfo* params);

/**
 * Time native function.
 * Returns the number of seconds since epoch.
 */
Value timeNative(int argCount, Value* args, ParamInfo* params);

/**
 * Exit native function.
 * Exits the current running script or REPL.
 */
Value exitNative(int argCount, Value* args, ParamInfo* params);

/**
 * len native function.
 */
Value lenNative(int argCount, Value* args, ParamInfo* params);

// MATH NATIVES

/**
 * Calculates the absolute value of a number.
 */
Value absNative(int argCount, Value* args, ParamInfo* params);

/**
 * Calculates the minimum of two numbers.
 */
Value minNative(int argCount, Value* args, ParamInfo* params);

/**
 * Calculates the maximum of two numbers.
 */
Value maxNative(int argCount, Value* args, ParamInfo* params);
