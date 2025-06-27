/**
 * @file os.c
 * @brief Implementation of os functions.
 */

#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE 700

#include <dirent.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "builtins/util.h"
#include "core/object.h"
#include "core/value.h"
#include "std/os.h"

// forward declarations of native functions

// env stuff
static Value getEnvNative(int argCount, Value* args);
static Value setEnvNative(int argCount, Value* args);
static Value unsetEnvNative(int argCount, Value* args);

// system info
static Value getCWD(int argCount, Value* args);
static Value getPID(int argCount, Value* args);
static Value getUID(int argCount, Value* args);

// file / directory stuff
static Value changeDir(int argCount, Value* args);
static Value makeDir(int argCount, Value* args);
static Value rmDir(int argCount, Value* args);
static Value removeFile(int argCount, Value* args);
static Value listDir(int argCount, Value* args);
static Value existsNtv(int argCount, Value* args);
static Value isFile(int argCount, Value* args);
static Value isDir(int argCount, Value* args);
static Value absPath(int argCount, Value* args);
static Value joinPath(int argCount, Value* args);
static Value baseName(int argCount, Value* args);
static Value dirName(int argCount, Value* args);

/**
 * @brief Gets the random module with all its functions.
 * @return A pointer to the ObjModule containing random functions.
 */
ObjModule* getOSModule() {
    ObjModule* module = newModule();
    defineBuiltIn(&module->methods, "getenv", getEnvNative, 1, 1, ((ParamInfo[]){{copyString("name", 4), true}}));
    defineBuiltIn(&module->methods, "setenv", setEnvNative, 2, 2, ((ParamInfo[]){{copyString("name", 4), true}, {copyString("value", 5), true}}));
    defineBuiltIn(&module->methods, "unsetenv", unsetEnvNative, 1, 1, ((ParamInfo[]){{copyString("name", 4), true}}));

    defineBuiltIn(&module->methods, "getcwd", getCWD, 0, 0, NULL);
    defineBuiltIn(&module->methods, "getpid", getPID, 0, 0, NULL);
    defineBuiltIn(&module->methods, "getuid", getUID, 0, 0, NULL);
    defineBuiltIn(&module->methods, "chdir", changeDir, 1, 1, ((ParamInfo[]){{copyString("path", 4), true}}));
    defineBuiltIn(&module->methods, "mkdir", makeDir, 1, 1, ((ParamInfo[]){{copyString("path", 4), true}}));
    defineBuiltIn(&module->methods, "rmdir", rmDir, 1, 1, ((ParamInfo[]){{copyString("path", 4), true}}));
    defineBuiltIn(&module->methods, "remove", removeFile, 1, 1, ((ParamInfo[]){{copyString("path", 4), true}}));
    defineBuiltIn(&module->methods, "listdir", listDir, 1, 1, ((ParamInfo[]){{copyString("path", 4), true}}));
    defineBuiltIn(&module->methods, "exists", existsNtv, 1, 1, ((ParamInfo[]){{copyString("path", 4), true}}));
    defineBuiltIn(&module->methods, "isfile", isFile, 1, 1, ((ParamInfo[]){{copyString("path", 4), true}}));
    defineBuiltIn(&module->methods, "isdir", isDir, 1, 1, ((ParamInfo[]){{copyString("path", 4), true}}));
    defineBuiltIn(&module->methods, "abspath", absPath, 1, 1, ((ParamInfo[]){{copyString("path", 4), true}}));
    defineBuiltIn(&module->methods, "join", joinPath, 1, -1, ((ParamInfo[]){{copyString("path", 4), true}, {copyString("...args", 7), true}}));
    defineBuiltIn(&module->methods, "basename", baseName, 1, 1, ((ParamInfo[]){{copyString("path", 4), true}}));
    defineBuiltIn(&module->methods, "dirname", dirName, 1, 1, ((ParamInfo[]){{copyString("path", 4), true}}));
    return module;
}

/**
 * Gets the value of an environment variable.
 * Usage: getenv("HOME")
 */
static Value getEnvNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL_PTR("getenv() expects a single string argument.");
    }
    const char* name = AS_CSTRING(args[0]);
    char* value = getenv(name);
    if (value == NULL) {
        return NIL_VAL;
    }
    return OBJ_VAL(copyString(value, (int)strlen(value)));
}

/**
 * Sets the value of an environment variable.
 * Usage: setenv("MYVAR", "value")
 */
static Value setEnvNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_STRING(args[0]) || !IS_STRING(args[1])) {
        return ERROR_VAL_PTR("setenv() expects two string arguments.");
    }
    const char* name = AS_CSTRING(args[0]);
    const char* value = AS_CSTRING(args[1]);
#if defined(_WIN32)
    // Windows uses _putenv_s
    int result = _putenv_s(name, value);
#else
    // POSIX uses setenv
    int result = setenv(name, value, 1);
#endif
    if (result != 0) {
        return ERROR_VAL_PTR("setenv() failed to set environment variable.");
    }
    return NIL_VAL;
}

/**
 * Unsets an environment variable.
 * Usage: unsetenv("MYVAR")
 */
static Value unsetEnvNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL_PTR("unsetenv() expects a single string argument.");
    }
    const char* name = AS_CSTRING(args[0]);
#if defined(_WIN32)
    // Windows uses _putenv_s with an empty value
    int result = _putenv_s(name, "");
#else
    // POSIX uses unsetenv
    int result = unsetenv(name);
#endif
    if (result != 0) {
        return ERROR_VAL_PTR("unsetenv() failed to unset environment variable.");
    }
    return NIL_VAL;
}

/**
 * @brief Gets the current working directory.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return The current working directory as a string or an error if the arguments are invalid.
 */
static Value getCWD(int argCount, Value* args) {
    if (argCount != 0) {
        return ERROR_VAL_PTR("getcwd() expects no arguments.");
    }
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return ERROR_VAL_PTR("getcwd() failed to get current working directory.");
    }
    return OBJ_VAL(copyString(cwd, (int)strlen(cwd)));
}

/**
 * @brief Gets the process ID of the current process.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return The process ID as a number or an error if the arguments are invalid.
 */
static Value getPID(int argCount, Value* args) {
    if (argCount != 0) {
        return ERROR_VAL_PTR("getpid() expects no arguments.");
    }
    pid_t pid = getpid();
    return NUMBER_VAL((double)pid);
}

/**
 * @brief Gets the user ID of the current process.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return The user ID as a number or an error if the arguments are invalid.
 */
static Value getUID(int argCount, Value* args) {
    if (argCount != 0) {
        return ERROR_VAL_PTR("getuid() expects no arguments.");
    }
    uid_t uid = getuid();
    return NUMBER_VAL((double)uid);
}

/**
 * @brief Changes the current working directory.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return NIL_VAL on success or an error if the arguments are invalid.
 */
static Value changeDir(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL_PTR("chdir() expects a single string argument.");
    }
    const char* path = AS_CSTRING(args[0]);
    if (chdir(path) != 0) {
        return ERROR_VAL_PTR("chdir() failed to change directory.");
    }
    return NIL_VAL;
}

/**
 * @brief Creates a new directory.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return NIL_VAL on success or an error if the arguments are invalid.
 */
static Value makeDir(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL_PTR("mkdir() expects a single string argument.");
    }
    const char* path = AS_CSTRING(args[0]);
    if (mkdir(path, 0755) != 0) {
        return ERROR_VAL_PTR("mkdir() failed to create directory.");
    }
    return NIL_VAL;
}

/**
 * @brief Removes a directory.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return NIL_VAL on success or an error if the arguments are invalid.
 */
static Value rmDir(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL_PTR("rmdir() expects a single string argument.");
    }
    const char* path = AS_CSTRING(args[0]);
    if (rmdir(path) != 0) {
        return ERROR_VAL_PTR("rmdir() failed to remove directory.");
    }
    return NIL_VAL;
}

/**
 * @brief Removes a file.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return NIL_VAL on success or an error if the arguments are invalid.
 */
static Value removeFile(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL_PTR("remove() expects a single string argument.");
    }
    const char* path = AS_CSTRING(args[0]);
    if (unlink(path) != 0) {
        return ERROR_VAL_PTR("remove() failed to remove file.");
    }
    return NIL_VAL;
}


/**
 * @brief Lists the contents of a directory.
 */
static Value listDir(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL_PTR("listdir() expects a single string argument.");
    }
    const char* path = AS_CSTRING(args[0]);
    DIR* dir = opendir(path);
    if (dir == NULL) {
        return ERROR_VAL_PTR("listdir() failed to open directory.");
    }

    ObjList* list = newList();
    int idx = 0;
    while (true) {
        struct dirent* entry = readdir(dir);
        if (entry == NULL) {
            break;
        }
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            Value name = OBJ_VAL(copyString(entry->d_name, (int)strlen(entry->d_name)));
            if (list->values.capacity <= idx) {
                growValueArray(&list->values);
            }
            list->values.values[idx++] = name;
            list->count++;
            list->values.count++;
        }
    }
    closedir(dir);
    return OBJ_VAL(list);
}

/**
 * @brief Checks if a file or directory exists.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return true if the file or directory exists, false otherwise.
 */
static Value existsNtv(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL_PTR("exists() expects a single string argument.");
    }
    const char* path = AS_CSTRING(args[0]);
    struct stat buffer;
    int exist = stat(path, &buffer);
    return BOOL_VAL(exist == 0);
}

/**
 * @brief Checks if a path is a file.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return true if the path is a file, false otherwise.
 */
static Value isFile(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL_PTR("isfile() expects a single string argument.");
    }
    const char* path = AS_CSTRING(args[0]);
    struct stat buffer;
    if (stat(path, &buffer) != 0) {
        return ERROR_VAL_PTR("isfile() failed to stat path.");
    }
    return BOOL_VAL(S_ISREG(buffer.st_mode));
}

/**
 * @brief Checks if a path is a directory.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return true if the path is a directory, false otherwise.
 */
static Value isDir(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL_PTR("isdir() expects a single string argument.");
    }
    const char* path = AS_CSTRING(args[0]);
    struct stat buffer;
    if (stat(path, &buffer) != 0) {
        return ERROR_VAL_PTR("isdir() failed to stat path.");
    }
    return BOOL_VAL(S_ISDIR(buffer.st_mode));
}

/**
 * @brief Gets the absolute path of a file or directory.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return The absolute path as a string or an error if the arguments are invalid.
 */
static Value absPath(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL_PTR("abspath() expects a single string argument.");
    }
    const char* path = AS_CSTRING(args[0]);
    char* resolved = realpath(path, NULL);
    if (resolved == NULL) {
        return ERROR_VAL_PTR("abspath() failed to resolve path.");
    }
    Value result = OBJ_VAL(copyString(resolved, (int)strlen(resolved)));
    free(resolved);
    return result;
}

/**
 * @brief Joins multiple path components into a single path.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return The joined path as a string or an error if the arguments are invalid.
 */
static Value joinPath(int argCount, Value* args) {
    if (argCount < 1 || !IS_STRING(args[0])) {
        return ERROR_VAL_PTR("join() expects at least one string argument.");
    }
    char path[1024] = {0};
    const char* sep = "/";
    for (int i = 0; i < argCount; i++) {
        if (!IS_STRING(args[i])) {
            return ERROR_VAL_PTR("join() expects all arguments to be strings.");
        }
        const char* part = AS_CSTRING(args[i]);
        if (i > 0) {
            strncat(path, sep, sizeof(path) - strlen(path) - 1);
        }
        strncat(path, part, sizeof(path) - strlen(path) - 1);
    }
    return OBJ_VAL(copyString(path, (int)strlen(path)));
}

/**
 * @brief Gets the base name of a file or directory.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return The base name as a string or an error if the arguments are invalid.
 */
static Value baseName(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL_PTR("basename() expects a single string argument.");
    }
    const char* path = AS_CSTRING(args[0]);
    const char* base = strrchr(path, '/');
    if (base == NULL) {
        return OBJ_VAL(copyString(path, (int)strlen(path)));
    }
    return OBJ_VAL(copyString(base + 1, (int)strlen(base + 1)));
}

/**
 * @brief Gets the directory name of a file or directory.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return The directory name as a string or an error if the arguments are invalid.
 */
static Value dirName(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL_PTR("dirname() expects a single string argument.");
    }
    const char* path = AS_CSTRING(args[0]);
    char dir[1024];
    strncpy(dir, path, sizeof(dir) - 1);
    char* lastSlash = strrchr(dir, '/');
    if (lastSlash == NULL) {
        return OBJ_VAL(copyString(".", 1)); // Current directory
    }
    *lastSlash = '\0'; // Remove the last part
    return OBJ_VAL(copyString(dir, (int)strlen(dir)));
}
