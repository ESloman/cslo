/**
 * @file type_methods.c
 * @brief methods for converting types
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/object.h"
#include "core/value.h"

#include "builtins/type_methods.h"
#include "builtins/util.h"

// forward declarations
Value boolCvrt(int argCount, Value* args, ParamInfo* params);
Value numberCvrt(int argCount, Value* args, ParamInfo* params);
Value strCvrt(int argCount, Value* args, ParamInfo* params);

/**
 * @brief Registers built-in type methods
 * @param tbl The Table to register the methods in.
 */
void registerBuiltInTypeMethods(Table* tbl) {
    defineBuiltIn(tbl, "bool", boolCvrt, 1, 1, PARAMS({copyString("value", 5), true}));
    defineBuiltIn(tbl, "number", numberCvrt, 1, 1, PARAMS({copyString("value", 5), true}));
    defineBuiltIn(tbl, "str", strCvrt, 1, 1, PARAMS({copyString("value", 5), true}));
}

/**
 * @brief Converts a value to a boolean.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return The boolean representation of the value.
 */
Value boolCvrt(int argCount, Value* args, ParamInfo* params) {
    return BOOL_VAL(!isFalsey(args[0]));
}

/**
 * @brief Converts a value to a number.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return The number representation of the value.
 */
Value numberCvrt(int argCount, Value* args, ParamInfo* params) {
    Value value = args[0];
    if (IS_NIL(value)) {
        return NUMBER_VAL(0);
    } else if (IS_BOOL(value)) {
        return NUMBER_VAL(AS_BOOL(value) ? 1 : 0);
    } else if (IS_NUMBER(value)) {
        return value;
    } else if (IS_STRING(value)) {
        ObjString* str = AS_STRING(value);
        char* endptr;
        double num = strtod(str->chars, &endptr);
        // Check if the entire string was consumed and it's not empty
        if (endptr != str->chars && *endptr == '\0') {
            return NUMBER_VAL(num);
        } else {
            return ERROR_VAL_PTR("number() could not convert string to number.");
        }
    }
    return ERROR_VAL_PTR("number() could not convert value to number.");
}

/**
 * @brief Converts a value to a string.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return The string representation of the value.
*/
Value strCvrt(int argCount, Value* args, ParamInfo* params) {
    Value value = args[0];

    Value result = valueToString(value);
    if (IS_STRING(result)) {
        return result;
    }
    return ERROR_VAL_PTR("str() could not convert value to string.");
}
