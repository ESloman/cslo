/**
 * @file file_methods.c
 * @brief Implementation of file methods in CSLO.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins/util.h"

#include "core/object.h"
#include "core/value.h"

#include "objects/file_methods.h"

static Value fileRead(int argCount, Value* args);
static Value fileClose(int argCount, Value* args);

/**
 * @brief Registers file methods for the given ObjClass.
 * @param cls The ObjClass representing the file type.
 */
void registerFileMethods(ObjClass* cls) {
    defineBuiltIn(&cls->methods, "read", fileRead);
    defineBuiltIn(&cls->methods, "close", fileClose);
    // defineBuiltIn(&cls->methods, "write", fileWrite);
    // defineBuiltIn(&cls->methods, "flush", fileFlush);
}


static Value fileRead(int argCount, Value* args) {
    if (argCount != 1 || !IS_FILE(args[0])) {
        // printf("fileRead() must be called on a file object.\n");
        return ERROR_VAL;
    }

    ObjFile* sFile = AS_FILE(args[0]);
    if (sFile->closed) {
        // printf("fileRead() called on a closed file.\n");
        return ERROR_VAL;
    }

    fseek(sFile->file, 0, SEEK_END);
    long size = ftell(sFile->file);
    rewind(sFile->file);
    char* buffer = (char*)malloc(size + 1);
    size_t bytesRead = fread(buffer, 1, size, sFile->file);
    if (bytesRead != (size_t)size) {
        free(buffer);
        return ERROR_VAL;
    }
    buffer[size] = '\0';

    Value result = OBJ_VAL(copyString(buffer, size));
    free(buffer);
    return result;
}

static Value fileClose(int argCount, Value* args) {
    if (argCount != 1 || !IS_FILE(args[0])) {
        return ERROR_VAL;
    }

    ObjFile* sFile = AS_FILE(args[0]);
    if (sFile->closed) {
        return ERROR_VAL;
    }

    fclose(sFile->file);
    sFile->closed = true;
    return NIL_VAL;
}
