/**
 * @file util.h
 * This file contains utility functions.
 */

#ifndef cslo_util_h
#define cslo_util_h

#include <stdlib.h>

/**
 * @brief Unescapes escape sequences in a string literal.
 * @param src: pointer to the start of the string (not including quotes)
 * @param len: length of the string (not including quotes)
 * @param outLen: pointer to size_t to receive the output length (optional)
 * @return Returns a newly allocated null-terminated string.
 */
char* unescapeString(const char* src, size_t len, size_t* outLen);

#endif // cslo_util_h
