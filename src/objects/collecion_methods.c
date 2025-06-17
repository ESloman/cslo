
/**
 * @file collecion_methods.c
 * @brief Implementation of collection methods in CSLO.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/object.h"
#include "core/table.h"
#include "core/value.h"

#include "objects/collection_methods.h"


// forward declarations of native functions
Value internalIndexNative(int argCount, Value* args);
Value clearNative(int argCount, Value* args);
Value popNative(int argCount, Value *args);
Value cloneNative(int argCount, Value* args);

/**
 * @brief Registers Container methods for the given ObjClass.
 * @param cls The ObjClass representing the string type.
 */
void registerContainerMethods(ObjClass* cls) {
    tableSet(&cls->methods, OBJ_VAL(copyString("__index__", 9)), OBJ_VAL(newNative(internalIndexNative)));
    tableSet(&cls->methods, OBJ_VAL(copyString("clear", 5)), OBJ_VAL(newNative(clearNative)));
    tableSet(&cls->methods, OBJ_VAL(copyString("pop", 3)), OBJ_VAL(newNative(popNative)));
    tableSet(&cls->methods, OBJ_VAL(copyString("clone", 5)), OBJ_VAL(newNative(cloneNative)));
}


/**
 * This is an internal method for getting the value from an index in a container.
 * For lists, it simply returns the value at the index.
 * For dictionaries, it retrieves the nth key value.
 */
Value internalIndexNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_CONTAINER(args[0]) || !IS_NUMBER(args[1])) {
        printf("__index__() must be called on a container with a number argument.\n");
        return NIL_VAL;
    }
    switch (OBJ_TYPE(args[0])) {
        case OBJ_LIST: {
            int idx = (int)AS_NUMBER(args[1]);
            ObjList* list = AS_LIST(args[0]);
            return list->values.values[idx];
        }
        case OBJ_DICT: {
            int idx = (int)AS_NUMBER(args[1]);
            ObjDict* dict = AS_DICT(args[0]);
            int found = 0;
            for (int i = 0; i < dict->data.capacity; i++) {
                Entry* entry = &dict->data.entries[i];
                if (!IS_EMPTY(entry->key) && !IS_NIL(entry->value)) {
                    if (found == idx) {
                        return entry->key;
                    }
                    found++;
                }
            }
            break;
        }
        default:
            return NIL_VAL;
    }
    return NIL_VAL; // If not found, return NIL_VAL
}

/**
 * Clear native function.
 * Removes all elements from the list.
 */
Value clearNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_CONTAINER(args[0])) {
        printf("clear() must be called on a list.\n");
        return NIL_VAL;
    }

    switch (OBJ_TYPE(args[0]))
    {
    case OBJ_LIST:
        ObjList* list = AS_LIST(args[0]);
        // Set all elements to NIL_VAL to avoid holding references
        for (int i = 0; i < list->count; i++) {
            list->values.values[i] = NIL_VAL;
        }
        if (list->values.capacity > 8 && list->count < list->values.capacity / 4) {
            shrinkValueArray(&list->values);
        }
        list->count = 0;
        list->values.count = 0;
        break;
    case OBJ_DICT:
        ObjDict* dict = AS_DICT(args[0]);
        tableClear(&dict->data);
        break;
    default:
        break;
    }
    return NIL_VAL;
}

/**
 * Pop native function.
 */
Value popNative(int argCount, Value *args) {
    if (argCount == 0 || !IS_CONTAINER(args[0])) {
        printf("pop() must be called on a container.");
        return NIL_VAL;
    }
    switch (OBJ_TYPE(args[0])) {
        case OBJ_LIST:
            if (argCount != 1) {
                printf("pop() must be called on a list with no arguments.\n");
                return NIL_VAL;
            }
            ObjList* list = AS_LIST(args[0]);
            if (list->count == 0) {
                // popping an empty list returns nil
                return NIL_VAL;
            }
            Value val = list->values.values[list->count - 1];
            list->values.values[list->count - 1] = NIL_VAL; // Clear the popped value
            if (list->values.capacity > 8 && list->count < list->values.capacity / 4) {
                shrinkValueArray(&list->values);
            }
            list->count--;
            list->values.count = list->count;
            return val;
        case OBJ_DICT:
            if (argCount < 2) {
                printf("pop() must be called on a dict with the key to pop.\n");
                return NIL_VAL;
            }
            ObjDict* dict = AS_DICT(args[0]);
            if (dict->data.count == 0) {
                // popping an empty dict returns nil
                return NIL_VAL;
            }
            Value key = args[1];
            Value value;
            if (tableGet(&dict->data, key, &value)) {
                // remove the key-value pair from the dict
                tableDelete(&dict->data, key);
                return value; // return the found value
            } else {
                if (argCount == 3) {
                    // if a default value is provided, return it
                    return args[2];
                }
                return NIL_VAL; // key not found, return nil
            }
            return NIL_VAL;
        default:
            return NIL_VAL;
    }
}

/**
 * Clone native function.
 * Shallow clones/copies a container.
 */
Value cloneNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_CONTAINER(args[0])) {
        printf("clone() must be called on a container.\n");
        return NIL_VAL;
    }
    switch (OBJ_TYPE(args[0])) {
    case OBJ_LIST:
        ObjList* list = AS_LIST(args[0]);
        ObjList* clone = newList();
        if (list->count > 0) {
            while (clone->values.capacity < list->count) {
                growValueArray(&clone->values);
            }
            // copying memory blocks is faster than copying each element
            memcpy(clone->values.values, list->values.values, sizeof(Value) * list->count);
            clone->count = list->count;
            clone->values.count = clone->count;
        }
        return OBJ_VAL(clone);
    case OBJ_DICT:
        ObjDict* dict = AS_DICT(args[0]);
        ObjDict* cloneD = newDict();
        if (dict->data.count > 0) {
            // ensure the clone has enough capacity
            tableAddAll(&dict->data, &cloneD->data);
            cloneD->data.count = dict->data.count;
        }
        return OBJ_VAL(cloneD);
    default:
        return NIL_VAL;
    }
}
