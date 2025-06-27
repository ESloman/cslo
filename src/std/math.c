/**
 * @file math.c
 * @brief Implementation of math-related functions and constants.
 */

#include <math.h>

#include "builtins/util.h"
#include "core/value.h"
#include "std/math.h"


// forward declarations of native functions
static Value ceilNative(int argCount, Value* args);
static Value floorNative(int argCount, Value* args);
static Value sqrtNative(int argCount, Value* args);
static Value sinNative(int argCount, Value* args);
static Value cosNative(int argCount, Value* args);
static Value tanNative(int argCount, Value* args);


/**
 * @brief Gets the math module with all its functions.
 * @return A pointer to the ObjDict containing math functions.
 */
ObjModule* getMathModule() {
    ObjModule* module = newModule();
    defineBuiltIn(&module->methods, "ceil", ceilNative, 1, 1, ((ParamInfo[]){{copyString("value", 5), true}}));
    defineBuiltIn(&module->methods, "floor", floorNative, 1, 1, ((ParamInfo[]){{copyString("value", 5), true}}));
    defineBuiltIn(&module->methods, "sqrt", sqrtNative, 1, 1, ((ParamInfo[]){{copyString("value", 5), true}}));
    defineBuiltIn(&module->methods, "sin", sinNative, 1, 1, ((ParamInfo[]){{copyString("value", 5), true}}));
    defineBuiltIn(&module->methods, "cos", cosNative, 1, 1, ((ParamInfo[]){{copyString("value", 5), true}}));
    defineBuiltIn(&module->methods, "tan", tanNative, 1, 1, ((ParamInfo[]){{copyString("value", 5), true}}));
    return module;
}

/**
 * Calculates the ceil of a number.
 * If the argument is not a number, returns NIL_VAL or throws an error.
 */
static Value ceilNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL_PTR("ceil() expects a single numeric argument.");
    }
    double value = AS_NUMBER(args[0]);
    return NUMBER_VAL(ceil(value));
}

/**
 * Calculates the floor of a number.
 * If the argument is not a number, returns NIL_VAL or throws an error.
 */
static Value floorNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL_PTR("floor() expects a single numeric argument.");
    }
    double value = AS_NUMBER(args[0]);
    return NUMBER_VAL(floor(value));
}

/**
 * Calculates the square root of a number.
 * If the argument is not a number, returns NIL_VAL or throws an error.
 */
static Value sqrtNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL_PTR("sqrt() expects a single numeric argument.");
    }
    double value = AS_NUMBER(args[0]);
    if (value < 0) {
        return ERROR_VAL_PTR("sqrt() domain error: negative input.");
    }
    return NUMBER_VAL(sqrt(value));
}

/**
 * Calculates the sine of a number.
 * If the argument is not a number, returns NIL_VAL or throws an error.
 */
static Value sinNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL_PTR("sin() expects a single numeric argument.");
    }
    double value = AS_NUMBER(args[0]);
    return NUMBER_VAL(sin(value));
}

/**
 * Calculates the cosine of a number.
 * If the argument is not a number, returns NIL_VAL or throws an error.
 */
static Value cosNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL_PTR("cos() expects a single numeric argument.");
    }
    double value = AS_NUMBER(args[0]);
    return NUMBER_VAL(cos(value));
}

/**
 * Calculates the tangent of a number.
 * If the argument is not a number, returns NIL_VAL or throws an error.
 */
static Value tanNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL_PTR("tan() expects a single numeric argument.");
    }
    double value = AS_NUMBER(args[0]);
    return NUMBER_VAL(tan(value));
}
