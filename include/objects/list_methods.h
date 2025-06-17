/**
 * @file list_methods.h
 * @brief Header file for list methods in CSLO.
 */

#ifndef cslo_list_methods_h
#define cslo_list_methods_h

#include "core/object.h"
#include "core/value.h"

/**
 * @brief Registers list methods for the given ObjClass.
 * @param cls The ObjClass representing the string type.
 */
void registerListMethods(ObjClass* cls);

#endif  // cslo_list_methods_h
