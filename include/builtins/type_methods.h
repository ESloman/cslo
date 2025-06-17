/**
 * @file type_methods.h
 * @brief methods for converting types
 */

#ifndef cslo_builtins_type_methods_h
#define cslo_builtins_type_methods_h

#include "core/object.h"
#include "core/table.h"
#include "core/value.h"

/**
 * @brief Registers built-in type methods
 * @param tbl The Table to register the methods in.
 */
void registerBuiltInTypeMethods(Table* tbl);

#endif // cslo_builtins_type_methods_h
