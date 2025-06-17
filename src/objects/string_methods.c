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

// forward declarations of native functions
Value upper(int argCount, Value* args);
Value lower(int argCount, Value* args);
Value title(int argCount, Value* args);
Value split(int argCount, Value* args);
Value strip(int argCount, Value* args);
Value startsWith(int argCount, Value* args);
Value endsWith(int argCount, Value* args);
Value isAlpha(int argCount, Value* args);
Value isAlphaNumeric(int argCount, Value* args);
Value isDigit(int argCount, Value* args);
Value find(int argCount, Value* args);
Value replace(int argCount, Value* args);
Value count(int argCount, Value* args);
Value strIndex(int argCount, Value* args);

/**
 * @brief Registers string methods for the given ObjClass.
 * @param cls The ObjClass representing the string type.
 */
void registerStringMethods(ObjClass* cls) {
    tableSet(&cls->methods, OBJ_VAL(copyString("upper", 5)), OBJ_VAL(newNative(upper)));
    tableSet(&cls->methods, OBJ_VAL(copyString("lower", 5)), OBJ_VAL(newNative(lower)));
    tableSet(&cls->methods, OBJ_VAL(copyString("title", 5)), OBJ_VAL(newNative(title)));
    tableSet(&cls->methods, OBJ_VAL(copyString("split", 5)), OBJ_VAL(newNative(split)));
    tableSet(&cls->methods, OBJ_VAL(copyString("strip", 5)), OBJ_VAL(newNative(strip)));
    tableSet(&cls->methods, OBJ_VAL(copyString("startswith", 10)), OBJ_VAL(newNative(startsWith)));
    tableSet(&cls->methods, OBJ_VAL(copyString("endswith", 8)), OBJ_VAL(newNative(endsWith)));
    tableSet(&cls->methods, OBJ_VAL(copyString("isalpha", 7)), OBJ_VAL(newNative(isAlpha)));
    tableSet(&cls->methods, OBJ_VAL(copyString("isdigit", 7)), OBJ_VAL(newNative(isDigit)));
    tableSet(&cls->methods, OBJ_VAL(copyString("isalphanum", 10)), OBJ_VAL(newNative(isAlphaNumeric)));
    tableSet(&cls->methods, OBJ_VAL(copyString("find", 4)), OBJ_VAL(newNative(find)));
    tableSet(&cls->methods, OBJ_VAL(copyString("replace", 7)), OBJ_VAL(newNative(replace)));
    tableSet(&cls->methods, OBJ_VAL(copyString("count", 5)), OBJ_VAL(newNative(count)));
    tableSet(&cls->methods, OBJ_VAL(copyString("index", 5)), OBJ_VAL(newNative(strIndex)));
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

/**
 * @brief Checks if the string only contains alpha characters.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return True if the string contains only alpha characters, false otherwise.
 */
Value isAlpha(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    for (int i = 0; i < str->length; i++) {
        if (!isalpha((unsigned char)str->chars[i])) {
            return BOOL_VAL(false);
        }
    }
    return BOOL_VAL(true);
}

/**
 * @brief Checks if the string only contains alphanumeric characters.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return True if the string contains only alphanumeric characters, false otherwise.
 */
Value isAlphaNumeric(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    for (int i = 0; i < str->length; i++) {
        if (!isalpha((unsigned char)str->chars[i]) && !isdigit((unsigned char)str->chars[i])) {
            return BOOL_VAL(false);
        }
    }
    return BOOL_VAL(true);
}

/**
 * @brief Checks if the string only contains numeric characters.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return True if the string contains only numeric characters, false otherwise.
 */
Value isDigit(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    for (int i = 0; i < str->length; i++) {
        if (!isdigit((unsigned char)str->chars[i])) {
            return BOOL_VAL(false);
        }
    }
    return BOOL_VAL(true);
}

/**
 * @brief Finds the index of the first occurrence of a substring in a string.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return The index of the first occurrence of the substring or -1 if not found.
 */
Value find(int argCount, Value* args) {
    if (argCount != 2 || !IS_STRING(args[0]) || !IS_STRING(args[1])) {
        return ERROR_VAL;
    }
    ObjString* original = AS_STRING(args[0]);
    ObjString* sub = AS_STRING(args[1]);

    if (sub->length > original->length) {
        return NUMBER_VAL(-1.0);
    }

    for (int i = 0; i <= original->length - sub->length; i++) {
        bool match = true;
        for (int j = 0; j < sub->length; j++) {
            if (original->chars[i + j] != sub->chars[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            return NUMBER_VAL((double)i);
        }
    }
    return NUMBER_VAL(-1.0);
}

/**
 * @brief Replaces all occurrences of a substring in a string with another substring.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return A new string with the replacements made or an error if the arguments are invalid.
 */
Value replace(int argCount, Value* args) {
    if (argCount != 3 || !IS_STRING(args[0]) || !IS_STRING(args[1]) || !IS_STRING(args[2])) {
        return ERROR_VAL;
    }
    ObjString* original = AS_STRING(args[0]);
    ObjString* oldSub = AS_STRING(args[1]);
    ObjString* newSub = AS_STRING(args[2]);

    if (oldSub->length == 0) {
        return OBJ_VAL(copyString(original->chars, original->length));
    }

    int count = 0;
    for (int i = 0; i <= original->length - oldSub->length; i++) {
        bool match = true;
        for (int j = 0; j < oldSub->length; j++) {
            if (original->chars[i + j] != oldSub->chars[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            count++;
            i += oldSub->length - 1;
        }
    }

    if (count == 0) {
        return OBJ_VAL(copyString(original->chars, original->length));
    }

    int newLength = original->length + count * (newSub->length - oldSub->length);
    char* newChars = ALLOCATE(char, newLength + 1);
    int newIndex = 0;

    for (int i = 0; i < original->length;) {
        bool match = true;
        for (int j = 0; j < oldSub->length && i + j < original->length; j++) {
            if (original->chars[i + j] != oldSub->chars[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            memcpy(newChars + newIndex, newSub->chars, newSub->length);
            newIndex += newSub->length;
            i += oldSub->length;
        } else {
            newChars[newIndex++] = original->chars[i++];
        }
    }
    newChars[newIndex] = '\0';

    ObjString* resultString = takeString(newChars, newLength);
    return OBJ_VAL(resultString);
}

/**
 * @brief Counts the occurrences of a substring in a string.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return The number of occurrences of the substring or an error if the arguments are invalid.
 */
Value count(int argCount, Value* args) {
    if (argCount != 2 || !IS_STRING(args[0]) || !IS_STRING(args[1])) {
        return ERROR_VAL;
    }
    ObjString* original = AS_STRING(args[0]);
    ObjString* sub = AS_STRING(args[1]);

    if (sub->length == 0) {
        return NUMBER_VAL((double)(original->length + 1));
    }

    int count = 0;
    for (int i = 0; i <= original->length - sub->length; i++) {
        bool match = true;
        for (int j = 0; j < sub->length; j++) {
            if (original->chars[i + j] != sub->chars[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            count++;
            i += sub->length - 1;
        }
    }
    return NUMBER_VAL((double)count);
}

/**
 * @brief Gets the index of a character in a string.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return The index of the character or -1 if not found.
 */
Value strIndex(int argCount, Value* args) {
    if (argCount != 2 || !IS_STRING(args[0]) || !IS_STRING(args[1])) {
        return ERROR_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    ObjString* index = AS_STRING(args[1]);

    if (index->length != 1) {
        return ERROR_VAL;
    }

    for (int i = 0; i < str->length; i++) {
        if (str->chars[i] == index->chars[0]) {
            return NUMBER_VAL((double)i);
        }
    }
    return ERROR_VAL;
}
