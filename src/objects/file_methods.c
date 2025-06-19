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

#include "util.h"

static Value fileRead(int argCount, Value* args);
static Value fileClose(int argCount, Value* args);
static Value fileWrite(int argCount, Value* args);

/**
 * @brief Registers file methods for the given ObjClass.
 * @param cls The ObjClass representing the file type.
 */
void registerFileMethods(ObjClass* cls) {
    defineBuiltIn(&cls->methods, "read", fileRead);
    defineBuiltIn(&cls->methods, "close", fileClose);
    defineBuiltIn(&cls->methods, "write", fileWrite);
    // defineBuiltIn(&cls->methods, "flush", fileFlush);
}


static Value fileRead(int argCount, Value* args) {
    if (argCount != 1 || !IS_FILE(args[0])) {
        return ERROR_VAL_PTR("read() must be called on a file object.");
    }

    ObjFile* sFile = AS_FILE(args[0]);
    if (sFile->closed) {
        return ERROR_VAL_PTR("read() called on a closed file.");
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
        return ERROR_VAL_PTR("close() must be called on a file object.");
    }

    ObjFile* sFile = AS_FILE(args[0]);
    if (sFile->closed) {
        return ERROR_VAL_PTR("close() called on a closed file.");
    }

    fclose(sFile->file);
    sFile->closed = true;
    return NIL_VAL;
}

static Value fileWrite(int argCount, Value* args) {
    if (argCount != 2 || !IS_FILE(args[0]) || !IS_STRING(args[1])) {
        return ERROR_VAL_PTR("write() must be called on a file object with a string argument.");
    }

    ObjFile* sFile = AS_FILE(args[0]);
    if (sFile->closed) {
        return ERROR_VAL_PTR("write() called on a closed file.");
    }
    if (sFile->mode == FILE_READ) {
        return ERROR_VAL_PTR("write() called on a file opened in read mode.");
    }

    ObjString* str = AS_STRING(args[1]);
    size_t unescLen;
    char* unesc = unescapeString(str->chars, str->length, &unescLen);

    size_t bytesWritten = fwrite(unesc, sizeof(char), unescLen, sFile->file);
    free(unesc);

    if (bytesWritten != unescLen) {
        return ERROR_VAL_PTR("Failed to write to file.");
    }

    return BOOL_VAL(true);
}
