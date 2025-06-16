/**
 * @file string_methods.c
 * @brief Implementation of string methods in CSLO.
 */

#include <ctype.h>
#include <string.h>

#include "core/memory.h"
#include "core/object.h"
#include "core/table.h"
#include "core/value.h"
#include "objects/string_methods.h"

/**
 * @brief Registers string methods for the given ObjClass.
 * @param stringClass The ObjClass representing the string type.
 */
void registerStringMethods(ObjClass* stringClass) {
    tableSet(&stringClass->methods, OBJ_VAL(copyString("upper", 5)), OBJ_VAL(newNative(upper)));
    tableSet(&stringClass->methods, OBJ_VAL(copyString("lower", 5)), OBJ_VAL(newNative(lower)));
    tableSet(&stringClass->methods, OBJ_VAL(copyString("title", 5)), OBJ_VAL(newNative(title)));
    tableSet(&stringClass->methods, OBJ_VAL(copyString("split", 5)), OBJ_VAL(newNative(split)));
    tableSet(&stringClass->methods, OBJ_VAL(copyString("strip", 5)), OBJ_VAL(newNative(strip)));
    tableSet(&stringClass->methods, OBJ_VAL(copyString("startswith", 10)), OBJ_VAL(newNative(startsWith)));
    tableSet(&stringClass->methods, OBJ_VAL(copyString("endswith", 8)), OBJ_VAL(newNative(endsWith)));
}

/**
 * @brief Converts a string to uppercase.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return A new string with all characters converted to uppercase.
 */
Value upper(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL;
    }

    ObjString* original = AS_STRING(args[0]);
    int length = original->length;
    char* upperChars = ALLOCATE(char, length + 1);

    for (int i = 0; i < length; i++) {
        upperChars[i] = toupper(original->chars[i]);
    }
    upperChars[length] = '\0';

    ObjString* upperString = takeString(upperChars, length);
    return OBJ_VAL(upperString);
}

/**
 * @brief Converts a string to lowercase.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return A new string with all characters converted to lowercase.
 */
Value lower(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL;
    }

    ObjString* original = AS_STRING(args[0]);
    int length = original->length;
    char* lowerChars = ALLOCATE(char, length + 1);

    for (int i = 0; i < length; i++) {
        lowerChars[i] = tolower(original->chars[i]);
    }
    lowerChars[length] = '\0';

    ObjString* lowerString = takeString(lowerChars, length);
    return OBJ_VAL(lowerString);
}

/**
 * @brief Converts a string to title case.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return A new string with the first character of each word capitalized.
 */
Value title(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL;
    }

    ObjString* original = AS_STRING(args[0]);
    int length = original->length;
    char* titleChars = ALLOCATE(char, length + 1);

    bool capitalizeNext = true;
    for (int i = 0; i < length; i++) {
        if (isspace(original->chars[i])) {
            titleChars[i] = original->chars[i];
            capitalizeNext = true;
        } else if (capitalizeNext) {
            titleChars[i] = toupper(original->chars[i]);
            capitalizeNext = false;
        } else {
            titleChars[i] = tolower(original->chars[i]);
        }
    }
    titleChars[length] = '\0';

    ObjString* titleString = takeString(titleChars, length);
    return OBJ_VAL(titleString);
}

/**
 * @brief Splits a string into a list of substrings based on a delimiter.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return A list of substrings or an error if the arguments are invalid.
 */
Value split(int argCount, Value* args) {
    if (argCount != 2 || !IS_STRING(args[0]) || !IS_STRING(args[1])) {
        return ERROR_VAL;
    }

    ObjString* original = AS_STRING(args[0]);
    ObjString* delimiter = AS_STRING(args[1]);
    if (delimiter->length != 1) {
        return ERROR_VAL;
    }
    char delim = delimiter->chars[0];
    ObjList* list = newList();
    int start = 0;
    for (int i = 0; i < original->length; i++) {
        if (original->chars[i] == delim) {
            if (i > start) {
                int length = i - start;
                char* substring = ALLOCATE(char, length + 1);
                memcpy(substring, original->chars + start, length);
                substring[length] = '\0';
                ObjString* subStrObj = takeString(substring, length);
                if (list->values.count >= list->values.capacity) {
                    growValueArray(&list->values);
                }
                list->values.values[list->count++] = OBJ_VAL(subStrObj);
            }
            start = i + 1;
        }
    }
    if (start < original->length) {
        int length = original->length - start;
        char* substring = ALLOCATE(char, length + 1);
        memcpy(substring, original->chars + start, length);
        substring[length] = '\0';
        ObjString* subStrObj = takeString(substring, length);
        if (list->values.count >= list->values.capacity) {
            growValueArray(&list->values);
        }
        list->values.values[list->count++] = OBJ_VAL(subStrObj);
    }
    list->values.count = list->count;
    return OBJ_VAL(list);
}

/**
 * @brief Strips whitespace from the start and end of a string.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return A new string with leading and trailing whitespace removed.
 */
Value strip(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL;
    }

    ObjString* original = AS_STRING(args[0]);

    if (original->length == 0) {
        return OBJ_VAL(takeString("", 0));
    }

    int start = 0;
    int end = original->length - 1;

    // Find the first non-whitespace character
    while (start <= end && isspace(original->chars[start])) {
        start++;
    }

    // Find the last non-whitespace character
    while (end >= start && isspace(original->chars[end])) {
        end--;
    }

    // If the string is empty after stripping, return an empty string
    if (start > end) {
        return OBJ_VAL(takeString("", 0));
    }

    int length = end - start + 1;
    char* strippedChars = ALLOCATE(char, length + 1);
    memcpy(strippedChars, original->chars + start, length);
    strippedChars[length] = '\0';

    ObjString* strippedString = takeString(strippedChars, length);
    return OBJ_VAL(strippedString);
}

/**
 * @brief Checks if the string starts with the given prefix.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return True if the string starts with the prefix, false otherwise.
 */
Value startsWith(int argCount, Value* args) {
    if (argCount != 2 || !IS_STRING(args[0]) || !IS_STRING(args[1])) {
        return ERROR_VAL;
    }

    ObjString* original = AS_STRING(args[0]);
    ObjString* prefix = AS_STRING(args[1]);

    if (prefix->length > original->length) {
        return BOOL_VAL(false);
    }

    for (int i = 0; i < prefix->length; i++) {
        if (original->chars[i] != prefix->chars[i]) {
            return BOOL_VAL(false);
        }
    }

    return BOOL_VAL(true);
}

/**
 * @brief Checks if the string ends with the given suffix.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return True if the string ends with the suffix, false otherwise.
 */
Value endsWith(int argCount, Value* args) {
    if (argCount != 2 || !IS_STRING(args[0]) || !IS_STRING(args[1])) {
        return ERROR_VAL;
    }

    ObjString* original = AS_STRING(args[0]);
    ObjString* suffix = AS_STRING(args[1]);

    if (suffix->length > original->length) {
        return BOOL_VAL(false);
    }

    int start = original->length - suffix->length;
    for (int i = 0; i < suffix->length; i++) {
        if (original->chars[start + i] != suffix->chars[i]) {
            return BOOL_VAL(false);
        }
    }

    return BOOL_VAL(true);
}
