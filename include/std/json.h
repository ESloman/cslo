/**
 * @file json.h
 * @brief JSON handling utilities.
 */

#ifndef cslo_std_json_h
#define cslo_std_json_h

#include "core/object.h"
#include "core/value.h"

/**
 * @brief Gets the JSON module with all its functions.
 * @return A pointer to the ObjModule containing JSON functions.
 */
ObjModule* getJsonModule();

#endif // cslo_std_json_h
