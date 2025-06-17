/**
 * @file util.c
 * @brief Utility functions for built-in methods
 */

#include <string.h>

#include "core/object.h"
#include "core/table.h"
#include "core/value.h"
#include "core/vm.h"

#include "builtins/util.h"

/**
 * Method for defining a builtin/native function.
 * These are functions that are built into the language
 * and implemented natively in C.
 */
void defineBuiltIn(Table* tbl, const char* name, NativeFn function) {
    // push(OBJ_VAL(copyString(name, (int)strlen(name))));
    // push(OBJ_VAL(newNative(function)));
    tableSet(tbl, OBJ_VAL(copyString(name, (int)strlen(name))), OBJ_VAL(newNative(function)));
    // pop();
    // pop();
}
