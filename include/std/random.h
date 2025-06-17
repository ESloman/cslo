/**
 * @file random.h
 * @brief Header of random number generation functions.
 */

#ifndef cslo_std_random_h
#define cslo_std_random_h

#include "core/object.h"
#include "core/value.h"

/**
 * @brief Gets the random module with all its functions.
 * @return A pointer to the ObjModule containing random functions.
 */
ObjModule* getRandomModule();

#endif // cslo_std_random_h
