/**
 * @file util.c
 * This file contains utility functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

/**
 * @brief Unescapes escape sequences in a string literal.
 * @param src: pointer to the start of the string (not including quotes)
 * @param len: length of the string (not including quotes)
 * @param outLen: pointer to size_t to receive the output length (optional)
 * @return Returns a newly allocated null-terminated string.
 */
char* unescapeString(const char* src, size_t len, size_t* outLen) {
    char* dest = (char*)malloc(len + 1); // max possible size
    size_t di = 0;
    for (size_t si = 0; si < len; ++si) {
        if (src[si] == '\\' && si + 1 < len) {
            switch (src[si + 1]) {
                case 'n': dest[di++] = '\n'; si++; break;
                case 't': dest[di++] = '\t'; si++; break;
                case 'r': dest[di++] = '\r'; si++; break;
                case '\\': dest[di++] = '\\'; si++; break;
                case '"': dest[di++] = '"'; si++; break;
                case '\'': dest[di++] = '\''; si++; break;
                case '0': dest[di++] = '\0'; si++; break;
                default: dest[di++] = src[si + 1]; si++; break;
            }
        } else {
            dest[di++] = src[si];
        }
    }
    dest[di] = '\0';
    if (outLen) *outLen = di;
    return dest;
}
