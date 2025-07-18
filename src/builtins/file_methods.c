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

static Value open(int argCount, Value* args, ParamInfo* params);

/**
 * @brief Registers built-in file methods
 * @param tbl The Table to register the methods in.
 */
void registerBuiltInFileMethods(Table* tbl) {
    defineBuiltIn(tbl, "open", open, 1, 2, PARAMS({copyString("path", 4), true}, {copyString("mode", 4), false}));
}

/**
 * @brief Opens a file and returns an ObjFile object.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return An ObjFile object or an error value if the file cannot be opened.
 */
static Value open(int argCount, Value* args, ParamInfo* params) {
    if (!IS_STRING(args[0])) {
        return ERROR_VAL_PTR("open() expects a file path (string) and optional mode.");
    }

    const char* path = AS_CSTRING(args[0]);
    const char* mode = (argCount == 2 && IS_STRING(args[1])) ? AS_CSTRING(args[1]) : "r";
    FileMode fileMode = mode[0] == 'r' ? FILE_READ : (mode[0] == 'w' ? FILE_WRITE : FILE_APPEND);
    FILE* f = fopen(path, mode);
    if (!f) {
        return ERROR_VAL_PTR("Failed to open file.");
    }

    // Ownership of 'f' is transferred to ObjFile.
    // ObjFile is responsible for closing the file
    ObjFile* sFile = newFile(f, fileMode, AS_STRING(args[0]));
    return OBJ_VAL(sFile);
}
