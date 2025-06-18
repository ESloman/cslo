/**
 * @file list_methods.h
 * @brief Implementation of list methods in CSLO.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins/util.h"
#include "core/object.h"
#include "core/value.h"

// forward declarations of native functions
Value appendNative(int argCount, Value* args);
Value insertNative(int argCount, Value* args);
Value removeNative(int argCount, Value* args);
Value reverseNative(int argCount, Value* args);
Value indexNative(int argCount, Value* args);
Value countNative(int argCount, Value* args);
Value extendNative(int argCount, Value* args);
Value sortNative(int argCount, Value* args);

/**
 * @brief Registers list methods for the given ObjClass.
 * @param cls The ObjClass representing the string type.
 */
void registerListMethods(ObjClass* cls) {
    defineBuiltIn(&cls->methods, "append", appendNative);
    defineBuiltIn(&cls->methods, "insert", insertNative);
    defineBuiltIn(&cls->methods, "remove", removeNative);
    defineBuiltIn(&cls->methods, "reverse", reverseNative);
    defineBuiltIn(&cls->methods, "index", indexNative);
    defineBuiltIn(&cls->methods, "count", countNative);
    defineBuiltIn(&cls->methods, "extend", extendNative);
    defineBuiltIn(&cls->methods, "sort", sortNative);
}

/**
 * Append native function.
 */
Value appendNative(int argCount, Value *args) {
    if (argCount != 2 || !IS_LIST(args[0])) {
        return ERROR_VAL_PTR("append() must be called on a list with one argument.");
    }
    ObjList* list = AS_LIST(args[0]);
    if (list->count + 1 > list->values.capacity) {
        growValueArray(&list->values);
    }
    list->values.values[list->count] = args[1];
    list->count++;
    list->values.count = list->count;
    return NIL_VAL;
}

/**
 * Insert native function.
 */
Value insertNative(int argCount, Value* args) {
    if (argCount != 3 || !IS_LIST(args[0]) || !IS_NUMBER(args[1])) {
        return ERROR_VAL_PTR("insert() must be called on a list with an index and a value.");
    }
    ObjList* list = AS_LIST(args[0]);
    int idx = (int)AS_NUMBER(args[1]);
    if (idx < 0) {
        idx = 0;
    }
    if (idx > list->count) {
        idx = list->count; // allow insert at end
    }

    #ifdef DEBUG_LOGGING
    printf("insert: idx=%d, value=", idx);
    printValue(args[2]);
    printf("\n");
    #endif

    // Grow the array if needed
    if (list->count + 1 > list->values.capacity) {
        growValueArray(&list->values);
    }

    memmove(&list->values.values[idx + 1], &list->values.values[idx], sizeof(Value) * (list->count - idx));
    list->values.values[idx] = args[2];
    list->count++;
    list->values.count = list->count;
    return NIL_VAL;
}

/**
 * remove native function.
 */
Value removeNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_LIST(args[0]) || !IS_NUMBER(args[1])) {
        return ERROR_VAL_PTR("remove() must be called on a list with an index.");
    }
    ObjList* list = AS_LIST(args[0]);
    int idx = (int)AS_NUMBER(args[1]);
    if (idx < 0 || idx >= list->count) {
        return ERROR_VAL_PTR("Index out of bounds for remove().");
    }

    Value removed = list->values.values[idx];
    memmove(&list->values.values[idx], &list->values.values[idx + 1], sizeof(Value) * (list->count - idx - 1));
    list->count--;
    list->values.count = list->count;
    list->values.values[list->count] = NIL_VAL; // Clear the slot
    if (list->values.capacity > 8 && list->count < list->values.capacity / 4) {
        shrinkValueArray(&list->values);
    }
    return removed;
}

/**
 * Reverse native function.
 */
Value reverseNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_LIST(args[0])) {
        return ERROR_VAL_PTR("reverse() must be called on a list.");
    }
    ObjList* list = AS_LIST(args[0]);
    if (list->count == 0 || list->count == 1) {
        // nothing to reverse; can just return current list as is
        return args[0];
    }
    ObjList* reversed = newList();
    for (int i = list->count - 1; i >= 0; i--) {
        if (reversed->count >= reversed->values.capacity) {
            growValueArray(&reversed->values);
        }
        reversed->values.values[reversed->count++] = list->values.values[i];
    }
    return OBJ_VAL(reversed);
}

/**
 * Index native function.
 */
Value indexNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_LIST(args[0])) {
        return ERROR_VAL_PTR("index() must be called on a list with a value.");
    }
    ObjList* list = AS_LIST(args[0]);
    if (list->count == 0) {
        return NIL_VAL;
    }

    for (int idx = 0; idx < list->count; idx++) {
        if (valuesEqual(list->values.values[idx], args[1])) {
            return NUMBER_VAL((double)idx);
        }
    }
    return NIL_VAL; // not found
}

/**
 * Count native function.
 */
Value countNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_LIST(args[0])) {
        return ERROR_VAL_PTR("count() must be called on a list with a value.");
    }
    ObjList* list = AS_LIST(args[0]);
    if (list->count == 0) {
        // guaranteed to be 0 occurences
        return NUMBER_VAL(0);
    }

    int occurences = 0;
    for (int idx = 0; idx < list->count; idx++) {
        if (valuesEqual(list->values.values[idx], args[1])) {
            occurences++;
        }
    }
    return NUMBER_VAL((double)occurences);
}

/**
 * Extends native function.
 * Extends a list with another list.
 */
Value extendNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_LIST(args[0]) || !IS_LIST(args[1])) {
        return ERROR_VAL_PTR("extend() must be called on a list with a list.");
    }
    ObjList* list = AS_LIST(args[0]);
    ObjList* other = AS_LIST(args[1]);

    int oldCount = list->count;
    int newCount = oldCount + other->count;

    while (list->values.capacity < newCount) {
        growValueArray(&list->values);
    }

    // copy the values from the other list
    // faster than copying the values one by one
    memcpy(&list->values.values[oldCount], other->values.values, sizeof(Value) * other->count);
    list->count = newCount;
    list->values.count = newCount;
    return NIL_VAL;
}

/**
 * Sort native function.
 * Sorts a list in-place.
 */
Value sortNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_LIST(args[0])) {
        return ERROR_VAL_PTR("sort() must be called on a list.");
    }
    ObjList* list = AS_LIST(args[0]);

    if (list->count <= 1) {
        // nothing to sort
        return NIL_VAL;
    }

    // add sort function here
    qsort(list->values.values, list->count, sizeof(Value), valueCompare);

    return NIL_VAL;
}
