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
Value boolCvrt(int argCount, Value* args);
Value numberCvrt(int argCount, Value* args);
Value strCvrt(int argCount, Value* args);

/**
 * @brief Registers built-in type methods
 * @param tbl The Table to register the methods in.
 */
void registerBuiltInTypeMethods(Table* tbl) {
    defineBuiltIn(tbl, "bool", boolCvrt);
    defineBuiltIn(tbl, "number", numberCvrt);
    defineBuiltIn(tbl, "str", strCvrt);
}

/**
 * @brief Converts a value to a boolean.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return The boolean representation of the value.
 */
Value boolCvrt(int argCount, Value* args) {
    if (argCount != 1) {
        return ERROR_VAL_PTR("bool() expects a single argument.");
    }
    Value value = args[0];
    if (IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value)) || (IS_NUMBER(value) && AS_NUMBER(value) == 0) || (IS_STRING(value) && AS_STRING(value)->length == 0) || (IS_LIST(value) && AS_LIST(value)->count == 0) || (IS_DICT(value) && AS_DICT(value)->data.count == 0)) {
        return BOOL_VAL(false);
    }
    return BOOL_VAL(true);
}

/**
 * @brief Converts a value to a number.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return The number representation of the value.
 */
Value numberCvrt(int argCount, Value* args) {
    if (argCount != 1) {
        return ERROR_VAL_PTR("number() expects a single argument.");
    }
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
Value strCvrt(int argCount, Value* args) {
    if (argCount != 1) {
        return ERROR_VAL_PTR("str() expects a single argument.");
    }
    Value value = args[0];

    if (IS_NIL(value)) {
        return OBJ_VAL(copyString("nil", 3));
    } else if (IS_BOOL(value)) {
        return OBJ_VAL(copyString(AS_BOOL(value) ? "true" : "false", AS_BOOL(value) ? 4 : 5));
    } else if (IS_NUMBER(value)) {
        char buffer[32];
        int len = snprintf(buffer, sizeof(buffer), "%.14g", AS_NUMBER(value));
        return OBJ_VAL(copyString(buffer, len));
    } else if (IS_STRING(value)) {
        return value;
    }
    return ERROR_VAL_PTR("str() could not convert value to string.");
}
