/**
 * @file string_methods.h
 * @brief Header file for string methods in CSLO.
 */

#ifndef cslo_string_methods_h
#define cslo_string_methods_h

#include "core/object.h"
#include "core/value.h"

/**
 * @brief Registers string methods for the given ObjClass.
 * @param stringClass The ObjClass representing the string type.
 */
void registerStringMethods(ObjClass* stringClass);

/**
 * @brief Converts a string to uppercase.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return A new string with all characters converted to uppercase.
 */
Value upper(int argCount, Value* args);

/**
 * @brief Converts a string to lowercase.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return A new string with all characters converted to lowercase.
 */
Value lower(int argCount, Value* args);

/**
 * @brief Converts a string to title case.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return A new string with the first character of each word capitalized.
 */
Value title(int argCount, Value* args);

/**
 * @brief Splits a string into a list of substrings based on a delimiter.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return A list of substrings or an error if the arguments are invalid.
 */
Value split(int argCount, Value* args);

/**
 * @brief Strips whitespace from the start and end of a string.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return A new string with leading and trailing whitespace removed.
 */
Value strip(int argCount, Value* args);

/**
 * @brief Checks if the string starts with the given prefix.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return True if the string starts with the prefix, false otherwise.
 */
Value startsWith(int argCount, Value* args);

/**
 * @brief Checks if the string ends with the given suffix.
 * @param argCount The number of arguments passed to the function.
 * @param args The arguments passed to the function.
 * @return True if the string ends with the suffix, false otherwise.
 */
Value endsWith(int argCount, Value* args);

#endif  // cslo_string_methods_h
