/**
 * @file string_methods.h
 * @brief Header file for string methods in CSLO.
 */

#ifndef cslo_string_methods_h
#define cslo_string_methods_h

#include "core/object.h"
#include "core/value.h"

/**
 * @brief Registers string methods for the given ObjClass.
 * @param cls The ObjClass representing the string type.
 */
void registerStringMethods(ObjClass* cls);

#endif  // cslo_string_methods_h
