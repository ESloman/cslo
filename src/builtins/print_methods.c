/**
 * @file print.c
 * @brief methods for printing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/object.h"
#include "core/value.h"

#include "builtins/print_methods.h"
#include "builtins/util.h"

#include "util.h"

Value printNative(int argCount, Value* args, ParamInfo* params);
Value printLNNative(int argCount, Value* args, ParamInfo* params);

/**
 * @brief Registers print methods
 * @param tbl The Table to register the methods in.
 */
void registerBuiltInPrintMethods(Table* tbl) {
    defineBuiltIn(tbl, "print", printNative, 0, -1, PARAMS({copyString("message", 7), true}, {copyString("...args", 7), false}));
    defineBuiltIn(tbl, "println", printLNNative, 0, -1, PARAMS({copyString("message", 7), true}, {copyString("...args", 7), false}));
}

/**
 * Print native function.
 */
Value printNative(int argCount, Value* args, ParamInfo* params) {
    for (int i = 0; i < argCount; i++) {
        if (IS_STRING(args[i])) {
            ObjString* str = AS_STRING(args[i]);
            size_t unescLen;
            char* unesc = unescapeString(str->chars, str->length, &unescLen);
            printf("%s", unesc);
            free(unesc);
        } else {
            printValue(args[i]);
        }
    }
    printf("\n");
    return NIL_VAL;
}

/**
 * Println native function.
 */
Value printLNNative(int argCount, Value* args, ParamInfo* params) {
    printNative(argCount, args, params);
    return NIL_VAL;
}
