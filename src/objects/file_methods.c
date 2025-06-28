/**
 * @file file_methods.c
 * @brief Implementation of file methods in CSLO.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "builtins/util.h"

#include "core/object.h"
#include "core/value.h"

#include "objects/file_methods.h"

#include "util.h"

static Value fileRead(int argCount, Value* args);
static Value fileReadline(int argCount, Value* args);
static Value fileReadLines(int argCount, Value* args);
static Value fileClose(int argCount, Value* args);
static Value fileWrite(int argCount, Value* args);
static Value fileWriteLine(int argCount, Value* args);
static Value fileWriteLines(int argCount, Value* args);
static Value fileSeek(int argCount, Value* args);
static Value fileFlush(int argCount, Value* args);
static Value fileTell(int argCount, Value* args);
static Value fileTruncate(int argCount, Value* args);

// native properties
static void registerNativeProperties(ObjClass* cls);
static Value propertyMode(Value arg);
static Value propertyClosed(Value arg);
static Value propertyName(Value arg);

/**
 * @brief Registers file methods for the given ObjClass.
 * @param cls The ObjClass representing the file type.
 */
void registerFileMethods(ObjClass* cls) {
    defineBuiltIn(&cls->methods, "read", fileRead, 1, 1, PARAMS({copyString("self", 4), true}));
    defineBuiltIn(&cls->methods, "readline", fileReadline, 1, 1, PARAMS({copyString("self", 4), true}));
    defineBuiltIn(&cls->methods, "readlines", fileReadLines, 1, 1, PARAMS({copyString("self", 4), true}));
    defineBuiltIn(&cls->methods, "close", fileClose, 1, 1, PARAMS({copyString("self", 4), true}));
    defineBuiltIn(&cls->methods, "write", fileWrite, 2, 2, PARAMS({copyString("self", 4), true}, {copyString("data", 4), true}));
    defineBuiltIn(&cls->methods, "writeline", fileWriteLine, 2, 2, PARAMS({copyString("self", 4), true}, {copyString("line", 4), true}));
    defineBuiltIn(&cls->methods, "writelines", fileWriteLines, 1, 1, PARAMS({copyString("self", 4), true}));
    defineBuiltIn(&cls->methods, "seek", fileSeek, 2, 2, PARAMS({copyString("self", 4), true}, {copyString("offset", 6), true}));
    defineBuiltIn(&cls->methods, "flush", fileFlush, 1, 1, PARAMS({copyString("self", 4), true}));
    defineBuiltIn(&cls->methods, "tell", fileTell, 1, 1, PARAMS({copyString("self", 4), true}));
    defineBuiltIn(&cls->methods, "truncate", fileTruncate, 1, 1, PARAMS({copyString("self", 4), true}));

    registerNativeProperties(cls);
}

static void registerNativeProperties(ObjClass* cls) {
    addNativeProperty(&cls->nativeProperties, "mode", propertyMode);
    addNativeProperty(&cls->nativeProperties, "closed", propertyClosed);
    addNativeProperty(&cls->nativeProperties, "name", propertyName);
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

static Value fileWriteLine(int argCount, Value* args) {
    if (argCount != 2 || !IS_FILE(args[0]) || !IS_STRING(args[1])) {
        return ERROR_VAL_PTR("writeline() must be called on a file object with a string argument.");
    }

    ObjFile* sFile = AS_FILE(args[0]);
    if (sFile->closed) {
        return ERROR_VAL_PTR("writeline() called on a closed file.");
    }
    if (sFile->mode == FILE_READ) {
        return ERROR_VAL_PTR("writeline() called on a file opened in read mode.");
    }

    ObjString* str = AS_STRING(args[1]);
    size_t unescLen;
    char* unesc = unescapeString(str->chars, str->length, &unescLen);
    size_t bytesWritten = fwrite(unesc, sizeof(char), unescLen, sFile->file);
    free(unesc);

    if (bytesWritten != unescLen) {
        return ERROR_VAL_PTR("Failed to write to file.");
    }
    if (fputc('\n', sFile->file) == EOF) {
        return ERROR_VAL_PTR("Failed to write newline to file.");
    }

    return BOOL_VAL(true);
}

static Value fileWriteLines(int argCount, Value* args) {
    if (argCount != 2 || !IS_FILE(args[0]) || !IS_LIST(args[1])) {
        return ERROR_VAL_PTR("writelines() must be called on a file object with a list argument.");
    }

    ObjFile* sFile = AS_FILE(args[0]);
    if (sFile->closed) {
        return ERROR_VAL_PTR("writelines() called on a closed file.");
    }
    if (sFile->mode == FILE_READ) {
        return ERROR_VAL_PTR("writelines() called on a file opened in read mode.");
    }

    ObjList* list = AS_LIST(args[1]);
    if (list->count == 0) {
        return BOOL_VAL(true);
    }

    for (int i = 0; i < list->count; i++) {
        if (!IS_STRING(list->values.values[i])) {
            return ERROR_VAL_PTR("writelines() requires a list of strings.");
        }
    }

    for (int i = 0; i < list->count; i++) {
        ObjString* str = AS_STRING(list->values.values[i]);
        size_t unescLen;
        char* unesc = unescapeString(str->chars, str->length, &unescLen);
        size_t bytesWritten = fwrite(unesc, sizeof(char), unescLen, sFile->file);
        free(unesc);

        if (bytesWritten != unescLen) {
            return ERROR_VAL_PTR("Failed to write to file.");
        }
        if (fputc('\n', sFile->file) == EOF) {
            return ERROR_VAL_PTR("Failed to write newline to file.");
        }
    }

    return BOOL_VAL(true);
}

static Value fileSeek(int argCount, Value* args) {
    if (argCount < 2 || !IS_FILE(args[0]) || !IS_NUMBER(args[1])) {
        return ERROR_VAL_PTR("seek() must be called on a file object with a numeric argument.");
    }

    ObjFile* sFile = AS_FILE(args[0]);
    if (sFile->closed) {
        return ERROR_VAL_PTR("seek() called on a closed file.");
    }

    if (argCount == 3 && !IS_NUMBER(args[2])) {
        return ERROR_VAL_PTR("seek() requires a second numeric argument for whence.");
    }

    int whence = (argCount == 3) ? (int)AS_NUMBER(args[2]) : SEEK_SET;
    int offset = (int)AS_NUMBER(args[1]);
    fseek(sFile->file, offset, whence);
    return NIL_VAL;
}

static Value fileReadline(int argCount, Value* args) {
    if (argCount != 1 || !IS_FILE(args[0])) {
        return ERROR_VAL_PTR("readline() must be called on a file object.");
    }
    ObjFile* sFile = AS_FILE(args[0]);
    if (sFile->closed) {
        return ERROR_VAL_PTR("readline() called on a closed file.");
    }

    char* line = NULL;
    size_t len = 0;
    ssize_t read = getline(&line, &len, sFile->file);
    if (read == -1) {
        free(line);
        return NIL_VAL; // EOF or error
    }

    Value result = OBJ_VAL(copyString(line, read));
    free(line);
    return result;
}

static Value fileFlush(int argCount, Value* args) {
    if (argCount != 1 || !IS_FILE(args[0])) {
        return ERROR_VAL_PTR("flush() must be called on a file object.");
    }

    ObjFile* sFile = AS_FILE(args[0]);
    if (sFile->closed) {
        return ERROR_VAL_PTR("flush() called on a closed file.");
    }

    if (fflush(sFile->file) != 0) {
        return ERROR_VAL_PTR("Failed to flush file.");
    }
    return NIL_VAL;
}

static Value fileReadLines(int argCount, Value* args) {
    if (argCount != 1 || !IS_FILE(args[0])) {
        return ERROR_VAL_PTR("readlines() must be called on a file object.");
    }

    ObjFile* sFile = AS_FILE(args[0]);
    if (sFile->closed) {
        return ERROR_VAL_PTR("readlines() called on a closed file.");
    }

    ObjList* lines = newList();

    char* line = NULL;
    size_t len = 0;
    while (getline(&line, &len, sFile->file) != -1) {
        ObjString* str = copyString(line, strlen(line));
        if (lines->count + 1 > lines->values.capacity) {
            growValueArray(&lines->values);
        }
        lines->values.values[lines->count++] = OBJ_VAL(str);
        lines->values.count = lines->count;
    }
    free(line);
    return OBJ_VAL(lines);
}

static Value fileTell(int argCount, Value* args) {
    if (argCount != 1 || !IS_FILE(args[0])) {
        return ERROR_VAL_PTR("tell() must be called on a file object.");
    }
    ObjFile* sFile = AS_FILE(args[0]);
    if (sFile->closed) {
        return ERROR_VAL_PTR("tell() called on a closed file.");
    }
    return NUMBER_VAL(ftell(sFile->file));
}

static Value fileTruncate(int argCount, Value* args) {
    if (argCount != 1 || !IS_FILE(args[0])) {
        return ERROR_VAL_PTR("truncate() must be called on a file object.");
    }

    ObjFile* sFile = AS_FILE(args[0]);
    if (sFile->closed) {
        return ERROR_VAL_PTR("truncate() called on a closed file.");
    }

    if (sFile->mode == FILE_READ) {
        return ERROR_VAL_PTR("truncate() called on a file opened in read mode.");
    }

    if (ftruncate(fileno(sFile->file), 0) != 0) {
        return ERROR_VAL_PTR("Failed to truncate file.");
    }
    return NIL_VAL;
}

// properties

static Value propertyMode(Value arg) {
    if (!IS_FILE(arg)) {
        return ERROR_VAL_PTR("mode must be called on a file object.");
    }
    ObjFile* sFile = AS_FILE(arg);
    switch (sFile->mode) {
        case FILE_READ: return OBJ_VAL(copyString("r", 1));
        case FILE_WRITE: return OBJ_VAL(copyString("w", 1));
        case FILE_APPEND: return OBJ_VAL(copyString("a", 1));
        default: return NIL_VAL;
    }
}

static Value propertyClosed(Value arg) {
    if (!IS_FILE(arg)) {
        return ERROR_VAL_PTR("closed must be called on a file object.");
    }
    ObjFile* sFile = AS_FILE(arg);
    return BOOL_VAL(sFile->closed);
}

static Value propertyName(Value arg) {
    if (!IS_FILE(arg)) {
        return ERROR_VAL_PTR("name must be called on a file object.");
    }
    ObjFile* sFile = AS_FILE(arg);
    return OBJ_VAL(sFile->name);
}
