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
 * @param dictClass The ObjClass representing the string type.
 */
void registerDictMethods(ObjClass* dictClass);

/**
 * Returns the keys of a dictionary as a list.
 */
Value keysNative(int argCount, Value* args);

/**
 * Returns the values of a dictionary as a list.
 */
Value valuesNative(int argCount, Value* args);

/**
 * Returns the value associated with a key in a dictionary.
 * If not present - returns NIL_VAL.
 */
Value getNative(int argCount, Value* args);

/**
 * update native method.
 * Updates a dict with another dict.
 */
Value updateNative(int argCount, Value* args);

/**
 * items native method.
 * Returns a list of key-value pairs from a dict.
 */
Value itemsNative(int argCount, Value* args);

#endif  // cslo_dict_methods_h
