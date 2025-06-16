/**
 * @file math.h
 * @brief Header file for math-related functions and constants.
 */

#ifndef cslo_std_math_h
#define cslo_std_math_h

#include "core/object.h"
#include "core/value.h"

/**
 * @brief Gets the math module with all its functions.
 * @return A pointer to the ObjDict containing math functions.
 */
ObjModule* getMathModule();

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


#endif  // cslo_std_math_h
