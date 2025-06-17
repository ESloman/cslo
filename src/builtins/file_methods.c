/**
 * @file file_methods.c
 */

#include <stdio.h>
#include <stdlib.h>

#include "core/object.h"
#include "core/table.h"
#include "core/value.h"

#include "builtins/file_methods.h"
#include "builtins/util.h"

static Value open(int argCount, Value* args);

/**
 * @brief Registers built-in file methods
 * @param tbl The Table to register the methods in.
 */
void registerBuiltInFileMethods(Table* tbl) {
    defineBuiltIn(tbl, "open", open);
}

/**
 * @brief Opens a file and returns an ObjFile object.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return An ObjFile object or an error value if the file cannot be opened.
 */
static Value open(int argCount, Value* args) {
    if (argCount < 1 || argCount > 2 || !IS_STRING(args[0])) {
        return ERROR_VAL;
    }

    const char* path = AS_CSTRING(args[0]);
    const char* mode = (argCount == 2 && IS_STRING(args[1])) ? AS_CSTRING(args[1]) : "r";
    FILE* f = fopen(path, mode);
    if (!f) {
        return ERROR_VAL;
    }

    ObjFile* sFile = newFile(f);
    return OBJ_VAL(sFile);
}
