/**
 * @file file_methods.h
 * @brief Methods for file operations
 */

#ifndef cslo_file_methods_h
#define cslo_file_methods_h

#include "core/object.h"
#include "core/table.h"
#include "core/value.h"

/**
 * @brief Registers built-in file methods
 * @param tbl The Table to register the methods in.
 */
void registerBuiltInFileMethods(Table* tbl);

#endif  // cslo_file_methods_h
