/**
 * @file random.h
 * @brief Header of random number generation functions.
 */

#ifndef cslo_std_random_h
#define cslo_std_random_h

#include "core/object.h"
#include "core/value.h"

/**
 * @brief Gets the random module with all its functions.
 * @return A pointer to the ObjDict containing random functions.
 */
ObjModule* getRandomModule();

/**
 * @brief Sets the seed for the random number generator.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return NIL_VAL if successful or an error if the arguments are invalid.
 */
Value randomSeedNative(int argCount, Value* args);

/**
 * @brief Generates a random number between 0 and 1.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return A random number between 0 and 1 or an error if the arguments are invalid.
 */
Value randomNative(int argCount, Value* args);

/**
 * @brief Generates a random integer between the given range.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return A random integer between the range or an error if the arguments are invalid.
 */
Value randomIntNative(int argCount, Value* args);

/**
 * @brief Generates a random number within a specified range.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return A random number within the range or an error if the arguments are invalid.
 */
Value randomRangeNative(int argCount, Value* args);


/**
 * @brief Generates a random choice from a list.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return A random choice from the list or an error if the arguments are invalid.
 */
Value randomChoiceNative(int argCount, Value* args);

/**
 * @brief Shuffles a list in place.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return The shuffled list or an error if the arguments are invalid.
 */
Value randomShuffleNative(int argCount, Value* args);

/**
 * @brief Generates a random boolean value.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return A random boolean value or an error if the arguments are invalid.
 */
Value randomBoolNative(int argCount, Value* args);

/**
 * @brief Generates a random byte array of specified length.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return A byte array containing random bytes or an error if the arguments are invalid.
 */
Value randomBytesNative(int argCount, Value* args);

/**
 * @brief Generates a random Gaussian (normal) distributed number.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return A random Gaussian distributed number or an error if the arguments are invalid.
 */
Value randomGaussNative(int argCount, Value* args);

/**
 * @brief Samples a list in place using the Fisher-Yates algorithm.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return The sampled list or an error if the arguments are invalid.
 */
Value randomSampleNative(int argCount, Value* args);

#endif // cslo_std_random_h
