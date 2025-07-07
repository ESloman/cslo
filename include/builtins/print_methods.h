/**
 * @file print_methods.h
 * @brief Methods for printing
 */

#ifndef cslo_print_methods_h
#define cslo_print_methods_h

#include "core/object.h"
#include "core/table.h"
#include "core/value.h"

/**
 * @brief Registers built-in print methods
 * @param tbl The Table to register the methods in.
 */
void registerBuiltInPrintMethods(Table* tbl);

#endif  // cslo_print_methods_h
