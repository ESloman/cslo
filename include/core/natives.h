/**
 * @file natives.h
 */

#include "core/object.h"
#include "core/value.h"

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

// CONTAINER NATIVES

Value internalIndexNative(int argCount, Value* args);

// LIST NATIVES

/**
 * append native function.
 */
Value appendNative(int argCount, Value* args);

/**
 * Pop native function.
 */
Value popNative(int argCount, Value *args);

/**
 * insert native function.
 */
Value insertNative(int argCount, Value* args);

/**
 * remove native function.
 */
Value removeNative(int argCount, Value* args);

/**
 * Reverse native function.
 */
Value reverseNative(int argCount, Value* args);

/**
 * Index native function.
 */
Value indexNative(int argCount, Value* args);

/**
 * Count native function.
 */
Value countNative(int argCount, Value* args);

/**
 * Clear native function.
 * Removes all elements from the list.
 */
Value clearNative(int argCount, Value* args);

/**
 * Clone native function.
 * Shallow copies/clones a list.
 */
Value cloneNative(int argCount, Value* args);

/**
 * Extends native function.
 * Extends a list with another list.
 */
Value extendNative(int argCount, Value* args);

/**
 * Sort native function.
 * Sorts a list in-place.
 */
Value sortNative(int argCount, Value* args);

// DICT NATIVES

/**
 * Returns the keys of a dictionary as a list.
 */
Value keysNative(int argCount, Value* args);

/**
 * Returns the values of a dictionary as a list.
 */
Value valuesNative(int argCount, Value* args);

/**
 * Returns the value associated with a key in a dictionary.
 * If not present - returns NIL_VAL.
 */
Value getNative(int argCount, Value* args);

/**
 * update native method.
 * Updates a dict with another dict.
 */
Value updateNative(int argCount, Value* args);

/**
 * items native method.
 * Returns a list of key-value pairs from a dict.
 */
Value itemsNative(int argCount, Value* args);

// MATH NATIVES

/**
 * Calculates the absolute value of a number.
 */
Value absNative(int argCount, Value* args);

/**
 * Calculates the minimum of two numbers.
 */
Value minNative(int argCount, Value* args);

/**
 * Calculates the maximum of two numbers.
 */
Value maxNative(int argCount, Value* args);


// OS NATIVES

/**
 * Gets an environment variable.
 * If the variable does not exist, returns NIL_VAL.
 */
Value getEnvNative(int argCount, Value* args);

/**
 * Sets an environment variable.
 * If the variable does not exist, it will be created.
 */
Value setEnvNative(int argCount, Value* args);

/**
 * Unsets an environment variable.
 * If the variable does not exist, it will do nothing.
 */
Value unsetEnvNative(int argCount, Value* args);
