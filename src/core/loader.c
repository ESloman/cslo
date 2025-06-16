/**
 * @file loader.c
 * @brief Implementation of the module loader for CSLO.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "core/loader.h"
#include "core/value.h"
#include "core/vm.h"

// add all the std library imports here

#include "std/math.h"
#include "std/random.h"

/**
 * @brief Loads a module by its name.
 */
bool loadModule(const char* moduleName) {
    if (strcmp(moduleName, "math") == 0) {
        ObjModule* mathModule = getMathModule();
        tableSet(&vm.globals, OBJ_VAL(copyString("math", 4)), OBJ_VAL(mathModule));
        return true;
    } else if (strcmp(moduleName, "random") == 0) {
        ObjModule* randomModule = getRandomModule();
        tableSet(&vm.globals, OBJ_VAL(copyString("random", 6)), OBJ_VAL(randomModule));
        return true;
    } else if (strcmp(moduleName, "os") == 0) {
        // Load OS module if implemented
        // Example: ObjModule* osModule = getOsModule();
        // tableSet(&vm.globals, OBJ_VAL(copyString("os", 2)), OBJ_VAL(osModule));
        return true;
    } else {
        printf("Error: Module '%s' not found.\n", moduleName);
    }
    return false;
}
