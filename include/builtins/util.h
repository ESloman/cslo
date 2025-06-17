/**
 * @file util.h
 * @brief Utility functions for built-in methods
 */

#ifndef cslo_builtins_util_h
#define cslo_builtins_util_h

#include "core/object.h"
#include "core/table.h"
#include "core/value.h"

/**
 * Method for defining a builtin/native function.
 * These are functions that are built into the language
 * and implemented natively in C.
 */
void defineBuiltIn(Table* tbl, const char* name, NativeFn function);


#endif
