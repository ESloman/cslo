/**
 * @file os.h
 * @brief Header of os functions.
 */

#ifndef cslo_std_os_h
#define cslo_std_os_h

#include "core/object.h"
#include "core/value.h"

/**
 * @brief Gets the os module with all its functions.
 * @return A pointer to the ObjModule containing os functions.
 */
ObjModule* getOSModule();

#endif // cslo_std_os_h
