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
    Value moduleVal;
    ObjString* canonicalName = copyString(moduleName, strlen(moduleName));

    if (tableGet(&vm.globals, OBJ_VAL(canonicalName), &moduleVal)) {
        tableSet(&vm.globals, OBJ_VAL(copyString(nickName, strlen(nickName))), moduleVal);
        return true;
    }
    if (strcmp(moduleName, "math") == 0) {
        ObjModule* mathModule = getMathModule();
        moduleVal = OBJ_VAL(mathModule);
    } else if (strcmp(moduleName, "random") == 0) {
        ObjModule* randomModule = getRandomModule();
        moduleVal = OBJ_VAL(randomModule);
    } else if (strcmp(moduleName, "os") == 0) {
        ObjModule* osModule = getOSModule();
        moduleVal = OBJ_VAL(osModule);
    } else if (strcmp(moduleName, "json") == 0) {
        ObjModule* jsonModule = getJsonModule();
        moduleVal = OBJ_VAL(jsonModule);
    } else {
        printf("Error: Module '%s' not found.\n", moduleName);
    }

    tableSet(&vm.globals, OBJ_VAL(canonicalName), moduleVal);
    tableSet(&vm.globals, OBJ_VAL(copyString(nickName, strlen(nickName))), moduleVal);
    return true;
}
