/**
 * @file loader.c
 * @brief Implementation of the module loader for CSLO.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/loader.h"
#include "core/value.h"
#include "core/vm.h"

// add all the std library imports here
#include "std/json.h"
#include "std/math.h"
#include "std/os.h"
#include "std/random.h"

typedef struct {
    const char* name;
    ObjModule* (*initFunc)(VM* vm);
} NativeModule;

/**
 * @
 */
NativeModule nativeModules[] = {
    {"math", getMathModule},
    {"random", getRandomModule},
    {"os", getOSModule},
    {"json", getJsonModule},
    {NULL, NULL}
};

static char* findModule(const char* moduleName) {
    // check current directory first
    // malloc for name + '.slo\0'
    char* localPath = malloc(strlen(moduleName) + 5);
    if (localPath == NULL) {
        fprintf(stderr, "Memory allocation failed for module path.\n");
        return NULL;
    }
    sprintf(localPath, "%s.slo", moduleName);
    printf("Trying local path: %s\n", localPath);
    // check if path exists
    FILE* file = fopen(localPath, "r");
    if (file != NULL) {
        printf("Found module at local path: %s\n", localPath);
        fclose(file);
        return localPath;
    }
    free(localPath);

    // check lib directory
    char* value = getenv("SLO_PATH");
    if (value == NULL) {
        value = "/usr/bin/slo/lib";
    }
    printf("%s\n", value);
    char* libPath = malloc(strlen(value) + strlen(moduleName) + 6);
    if (libPath == NULL) {
        fprintf(stderr, "Memory allocation failed for module path.\n");
        return NULL;
    }
    sprintf(libPath, "%s/%s.slo", value, moduleName);
    printf("Trying lib path: %s\n", libPath);
    // check if path exists
    FILE* libFile = fopen(libPath, "r");
    if (libFile != NULL) {
        printf("Found module at lib path: %s\n", libPath);
        fclose(libFile);
        return libPath;
    }
    free(libPath);
    return NULL;
}

static bool loadFileModule(const char* path) {
    // read the file object
    FILE* file = fopen(path, "rb");
    if (!file) return NULL;
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    char* buffer = malloc(size + 1);
    if (!buffer) {
        fclose(file);
        return false;
    }
    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);

    InterpretResult result = interpret(buffer, path);
    free(buffer);

    if (result != INTERPRET_OK) {
        return false;
    }

    return true;
}

/**
 * @brief Loads a module by its name.
 */
bool loadModule(const char* moduleName, const char* nickName) {
    Value moduleVal = NIL_VAL;
    ObjString* canonicalName = copyString(moduleName, strlen(moduleName));

    if (tableGet(&vm.globals, OBJ_VAL(canonicalName), &moduleVal)) {
        tableSet(&vm.globals, OBJ_VAL(copyString(nickName, strlen(nickName))), moduleVal);
        return true;
    }

    for (int i = 0; nativeModules[i].name != NULL; i++) {
        if (strcmp(nativeModules[i].name, moduleName) == 0) {
            moduleVal = OBJ_VAL(nativeModules[i].initFunc(&vm));
            break;
        }
    }

    if (!IS_NIL(moduleVal)) {
        tableSet(&vm.globals, OBJ_VAL(canonicalName), moduleVal);
        tableSet(&vm.globals, OBJ_VAL(copyString(nickName, strlen(nickName))), moduleVal);
        return true;
    }
    return false;

    // commenting out our loading local modules for now
    // couldn't find a native module - look for named module
    if (IS_NIL(moduleVal)) {

        char* path = findModule(moduleName);
        if (path == NULL) {
            return false;
        }

        // Mark as loading: insert a placeholder (could just be NIL_VAL)
        tableSet(&vm.globals, OBJ_VAL(canonicalName), NIL_VAL);
        tableSet(&vm.globals, OBJ_VAL(copyString(nickName, strlen(nickName))), NIL_VAL);

        // we found the module! We need to import it now
        loadFileModule(path);
        free(path);
    }
    return true;
}
