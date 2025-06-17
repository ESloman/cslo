/**
 * @file dict_methods.h
 * @brief Header file for dict methods in CSLO.
 */

#ifndef cslo_dict_methods_h
#define cslo_dict_methods_h

#include "core/object.h"
#include "core/value.h"

/**
 * @brief Registers dict methods for the given ObjClass.
 * @param cls The ObjClass representing the string type.
 */
void registerDictMethods(ObjClass* cls);

#endif  // cslo_dict_methods_h
