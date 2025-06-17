/**
 * @file os.c
 * @brief Implementation of os functions.
 */

#define _POSIX_C_SOURCE 200112L

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtins/util.h"
#include "core/object.h"
#include "core/value.h"
#include "std/os.h"


// forward declarations of native functions
static Value getEnvNative(int argCount, Value* args);
static Value setEnvNative(int argCount, Value* args);
static Value unsetEnvNative(int argCount, Value* args);

/**
 * @brief Gets the random module with all its functions.
 * @return A pointer to the ObjModule containing random functions.
 */
ObjModule* getOSModule() {
    ObjModule* module = newModule();
    defineBuiltIn(&module->methods, "getenv", getEnvNative);
    defineBuiltIn(&module->methods, "setenv", setEnvNative);
    defineBuiltIn(&module->methods, "unsetenv", unsetEnvNative);
    return module;
}

/**
 * Gets the value of an environment variable.
 * Usage: getenv("HOME")
 */
static Value getEnvNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL;
    }
    const char* name = AS_CSTRING(args[0]);
    char* value = getenv(name);
    if (value == NULL) {
        return NIL_VAL;
    }
    return OBJ_VAL(copyString(value, (int)strlen(value)));
}

/**
 * Sets the value of an environment variable.
 * Usage: setenv("MYVAR", "value")
 */
static Value setEnvNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_STRING(args[0]) || !IS_STRING(args[1])) {
        return ERROR_VAL;
    }
    const char* name = AS_CSTRING(args[0]);
    const char* value = AS_CSTRING(args[1]);
#if defined(_WIN32)
    // Windows uses _putenv_s
    int result = _putenv_s(name, value);
#else
    // POSIX uses setenv
    int result = setenv(name, value, 1);
#endif
    if (result != 0) {
        return ERROR_VAL;
    }
    return NIL_VAL;
}

/**
 * Unsets an environment variable.
 * Usage: unsetenv("MYVAR")
 */
static Value unsetEnvNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL;
    }
    const char* name = AS_CSTRING(args[0]);
#if defined(_WIN32)
    // Windows uses _putenv_s with an empty value
    int result = _putenv_s(name, "");
#else
    // POSIX uses unsetenv
    int result = unsetenv(name);
#endif
    if (result != 0) {
        return ERROR_VAL;
    }
    return NIL_VAL;
}
