/**
 * @file loader.h
 * @brief Header file for the module loader in CSLO.
 */

#ifndef cslo_loader_h
#define cslo_loader_h

#include <stdbool.h>

bool loadModule(const char* moduleName, const char* nickName);

#endif  // cslo_loader_h
