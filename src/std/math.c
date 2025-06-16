/**
 * @file math.c
 * @brief Implementation of math-related functions and constants.
 */

#include <math.h>

#include "core/value.h"
#include "std/math.h"

/**
 * @brief Gets the math module with all its functions.
 * @return A pointer to the ObjDict containing math functions.
 */
ObjModule* getMathModule() {
    ObjModule* mathModule = newModule();
    tableSet(&mathModule->methods, OBJ_VAL(copyString("ceil", 4)), OBJ_VAL(newNative(ceilNative)));
    tableSet(&mathModule->methods, OBJ_VAL(copyString("floor", 5)), OBJ_VAL(newNative(floorNative)));
    tableSet(&mathModule->methods, OBJ_VAL(copyString("sqrt", 4)), OBJ_VAL(newNative(sqrtNative)));
    tableSet(&mathModule->methods, OBJ_VAL(copyString("sin", 3)), OBJ_VAL(newNative(sinNative)));
    tableSet(&mathModule->methods, OBJ_VAL(copyString("cos", 3)), OBJ_VAL(newNative(cosNative)));
    tableSet(&mathModule->methods, OBJ_VAL(copyString("tan", 3)), OBJ_VAL(newNative(tanNative)));
    return mathModule;
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
