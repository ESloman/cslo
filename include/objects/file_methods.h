/**
 * @file file_methods.h
 * @brief Header file for file methods in CSLO.
 */

#ifndef cslo_obj_file_methods_h
#define cslo_obj_file_methods_h

#include "core/object.h"
#include "core/value.h"

/**
 * @brief Registers file methods for the given ObjClass.
 * @param cls The ObjClass representing the file type.
 */
void registerFileMethods(ObjClass* cls);

#endif  // cslo_file_methods_h
