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

// MATH NATIVES

/**
 * Calculates the absolute value of a number.
 */
Value absNative(int argCount, Value* args);

/**
 * Calculates the ceil of a number.
 * If the argument is not a number, returns NIL_VAL or throws an error.
 */
Value ceilNative(int argCount, Value* args);

/**
 * Calculates the floor of a number.
 * If the argument is not a number, returns NIL_VAL or throws an error.
 */
Value floorNative(int argCount, Value* args);

/**
 * Calculates the square root of a number.
 */
Value sqrtNative(int argCount, Value* args);

/**
 * Calculates the sine of a number.
 */
Value sinNative(int argCount, Value* args);

/**
 * Calculates the cosine of a number.
 */
Value cosNative(int argCount, Value* args);

/**
 * Calculates the tangent of a number.
 */
Value tanNative(int argCount, Value* args);

/**
 * Calculates the minimum of two numbers.
 */
Value minNative(int argCount, Value* args);

/**
 * Calculates the maximum of two numbers.
 */
Value maxNative(int argCount, Value* args);



// RANDOM NATIVES

/**
 * Generates a random number between 0 and 1.
 */
Value randomNative(int argCount, Value* args);

/**
 * Generates a random integer between the given range.
 * If the arguments are not numbers, returns NIL_VAL or throws an error.
 */
Value randomIntNative(int argCount, Value* args);

/**
 * Generates a random number between the given range.
 * If the arguments are not numbers, returns NIL_VAL or throws an error.
 */
Value randomRangeNative(int argCount, Value* args);

/**
 * Sets the seed for the random number generator.
 */
Value randomSeedNative(int argCount, Value* args);

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
