/**
 * @file random.c
 * @brief Implementation of random number generation functions.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "builtins/util.h"
#include "core/object.h"
#include "core/value.h"
#include "std/random.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// forward declarations
static Value randomSeedNative(int argCount, Value* args);
static Value randomNative(int argCount, Value* args);
static Value randomIntNative(int argCount, Value* args);
static Value randomRangeNative(int argCount, Value* args);
static Value randomChoiceNative(int argCount, Value* args);
static Value randomShuffleNative(int argCount, Value* args);
static Value randomBoolNative(int argCount, Value* args);
static Value randomBytesNative(int argCount, Value* args);
static Value randomGaussNative(int argCount, Value* args);
static Value randomSampleNative(int argCount, Value* args);

/**
 * @brief Gets the random module with all its functions.
 * @return A pointer to the ObjModule containing random functions.
 */
ObjModule* getRandomModule() {
    ObjModule* module = newModule();
    defineBuiltIn(&module->methods, "seed", randomSeedNative);
    defineBuiltIn(&module->methods, "random", randomNative);
    defineBuiltIn(&module->methods, "randint", randomIntNative);
    defineBuiltIn(&module->methods, "randrange", randomRangeNative);
    defineBuiltIn(&module->methods, "choice", randomChoiceNative);
    defineBuiltIn(&module->methods, "shuffle", randomShuffleNative);
    defineBuiltIn(&module->methods, "randbool", randomBoolNative);
    defineBuiltIn(&module->methods, "randbytes", randomBytesNative);
    defineBuiltIn(&module->methods, "gauss", randomGaussNative);
    defineBuiltIn(&module->methods, "sample", randomSampleNative);
    return module;
}

/**
 * Sets the seed for the random number generator.
 */
static Value randomSeedNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL_PTR("seed() expects a single numeric argument.");
    }
    unsigned int seed = (unsigned int)AS_NUMBER(args[0]);
    srand(seed);
    return NIL_VAL;
}

/**
 * Generates a random number between 0 and 1.
 */
static Value randomNative(int argCount, Value* args) {
    if (argCount != 0) {
        return ERROR_VAL_PTR("random() expects no arguments.");
    }
    return NUMBER_VAL((double)rand() / RAND_MAX);
}

/**
 * Generates a random integer between the given range.
 * If the arguments are not numbers, returns ERROR_VAL or throws an error.
 */
static Value randomIntNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        return ERROR_VAL_PTR("randint() expects two numeric arguments.");
    }
    double min = AS_NUMBER(args[0]);
    double max = AS_NUMBER(args[1]);
    if (min > max) {
        return ERROR_VAL_PTR("randint() min must be less than or equal to max.");
    }
    int range = (int)(max - min + 1);
    return NUMBER_VAL(min + (rand() % range));
}

/**
 * Generates a random number between the given range.
 * If the arguments are not numbers, returns ERROR_VAL or throws an error.
 */
static Value randomRangeNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        return ERROR_VAL_PTR("randrange() expects two numeric arguments.");
    }
    double min = AS_NUMBER(args[0]);
    double max = AS_NUMBER(args[1]);
    if (min > max) {
        return ERROR_VAL_PTR("randrange() min must be less than or equal to max.");
    }
    double range = max - min;
    return NUMBER_VAL(min + ((double)rand() / RAND_MAX) * range);
}

/**
 * @brief Generates a random choice from a list.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return A random choice from the list or an error if the arguments are invalid.
 */
static Value randomChoiceNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_LIST(args[0])) {
        return ERROR_VAL_PTR("choice() expects a single list argument.");
    }
    ObjList* list = AS_LIST(args[0]);
    if (list->count == 0) {
        return NIL_VAL;
    }
    int index = rand() % list->count;
    return list->values.values[index];
}

/**
 * @brief Shuffles a list in place.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return The shuffled list or an error if the arguments are invalid.
 */
static Value randomShuffleNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_LIST(args[0])) {
        return ERROR_VAL_PTR("shuffle() expects a single list argument.");
    }
    ObjList* list = AS_LIST(args[0]);
    if (list->count <= 1) {
        return OBJ_VAL(list); // No need to shuffle
    }
    // Fisher-Yates shuffle
    for (int i = list->count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        // Swap elements at indices i and j
        Value temp = list->values.values[i];
        list->values.values[i] = list->values.values[j];
        list->values.values[j] = temp;
    }
    return OBJ_VAL(list);
}

/**
 * @brief Generates a random boolean value.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return A random boolean value or an error if the arguments are invalid.
 */
static Value randomBoolNative(int argCount, Value* args) {
    if (argCount != 0) {
        return ERROR_VAL_PTR("randbool() expects no arguments.");
    }
    int val = rand() & 1;
    return BOOL_VAL(val != 0);
}

/**
 * @brief Generates a random byte array of specified length.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return A byte array containing random bytes or an error if the arguments are invalid.
 */
static Value randomBytesNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL_PTR("randbytes() expects a single numeric argument.");
    }
    int length = (int)AS_NUMBER(args[0]);
    if (length < 0) {
        return ERROR_VAL_PTR("randbytes() length must be non-negative.");
    }
    ObjList* byteArray = newList();
    while (byteArray->values.capacity < length) {
        growValueArray(&byteArray->values);
    }
    for (int i = 0; i < length; i++) {
        byteArray->values.values[i] = NUMBER_VAL(rand() % 256);
    }
    byteArray->count = length;
    byteArray->values.count = length;
    return OBJ_VAL(byteArray);
}

/**
 * @brief Generates a random Gaussian (normal) distributed number.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return A random Gaussian distributed number or an error if the arguments are invalid.
 */
static Value randomGaussNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        return ERROR_VAL_PTR("gauss() expects two numeric arguments.");
    }
    double mu = AS_NUMBER(args[0]);
    double sigma = AS_NUMBER(args[1]);
    if (sigma <= 0) {
        return ERROR_VAL_PTR("gauss() sigma must be positive.");
    }

    double u1 = (double)rand() / RAND_MAX;
    double u2 = (double)rand() / RAND_MAX;
    double z0 = sqrt(-2.0 * log(u1)) * cos(2 * M_PI * u2);
    return NUMBER_VAL(mu + z0 * sigma);
}

/**
 * @brief Samples a list in place using the Fisher-Yates algorithm.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return The sampled list or an error if the arguments are invalid.
 */
static Value randomSampleNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_LIST(args[0]) || !IS_NUMBER(args[1])) {
        return ERROR_VAL_PTR("sample() expects a list and a numeric argument.");
    }
    ObjList* list = AS_LIST(args[0]);
    int sampleSize = (int)AS_NUMBER(args[1]);
    if (sampleSize < 0 || sampleSize > list->count) {
        return ERROR_VAL_PTR("sample() size must be in range 0..list length.");
    }
    ObjList* sample = newList();
    while (sample->values.capacity < sampleSize) {
        growValueArray(&sample->values);
    }
    for (int i = 0; i < sampleSize; i++) {
        int index = rand() % list->count;
        sample->values.values[i] = list->values.values[index];
        // Remove the sampled element from the original list
        list->values.values[index] = list->values.values[list->count - 1];
        list->count--;
    }
    sample->count = sampleSize;
    sample->values.count = sampleSize;
    return OBJ_VAL(sample);
}
