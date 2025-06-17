/**
 * @file collection_methods.h
 * @brief Header file for collection methods in CSLO.
 */

#ifndef cslo_collection_methods_h
#define cslo_collection_methods_h

#include "core/object.h"
#include "core/value.h"

/**
 * @brief Registers Container methods for the given ObjClass.
 * @param cls The ObjClass representing the string type.
 */
void registerContainerMethods(ObjClass* cls);

#endif  // cslo_collection_methods_h
