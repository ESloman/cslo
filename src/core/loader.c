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
#include "std/json.h"
#include "std/math.h"
#include "std/os.h"
#include "std/random.h"

/**
 * @brief Loads a module by its name.
 */
bool loadModule(const char* moduleName, const char* nickName) {
    if (strcmp(moduleName, "math") == 0) {
        ObjModule* mathModule = getMathModule();
        tableSet(&vm.globals, OBJ_VAL(copyString(nickName, strlen(nickName))), OBJ_VAL(mathModule));
        return true;
    } else if (strcmp(moduleName, "random") == 0) {
        ObjModule* randomModule = getRandomModule();
        tableSet(&vm.globals, OBJ_VAL(copyString(nickName, strlen(nickName))), OBJ_VAL(randomModule));
        return true;
    } else if (strcmp(moduleName, "os") == 0) {
        ObjModule* osModule = getOSModule();
        tableSet(&vm.globals, OBJ_VAL(copyString(nickName, strlen(nickName))), OBJ_VAL(osModule));
        return true;
    } else if (strcmp(moduleName, "json") == 0) {
        ObjModule* jsonModule = getJsonModule();
        tableSet(&vm.globals, OBJ_VAL(copyString(nickName, strlen(nickName))), OBJ_VAL(jsonModule));
        return true;
    } else {
        printf("Error: Module '%s' not found.\n", moduleName);
    }
    return false;
}
