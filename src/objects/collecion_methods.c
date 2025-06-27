/**
 * @file collecion_methods.c
 * @brief Implementation of collection methods in CSLO.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins/util.h"
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
    defineBuiltIn(&cls->methods, "__index__", internalIndexNative, 2, 2, ((ParamInfo[]){{copyString("self", 4), true}, {copyString("index", 5), true}}));
    defineBuiltIn(&cls->methods, "clear", clearNative, 1, 1, ((ParamInfo[]){{copyString("self", 4), true}}));
    defineBuiltIn(&cls->methods, "pop", popNative, 1, 1, ((ParamInfo[]){{copyString("self", 4), true}}));
    defineBuiltIn(&cls->methods, "clone", cloneNative, 1, 1, ((ParamInfo[]){{copyString("self", 4), true}}));
}


/**
 * This is an internal method for getting the value from an index in a container.
 * For lists, it simply returns the value at the index.
 * For dictionaries, it retrieves the nth key value.
 */
Value internalIndexNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_CONTAINER(args[0]) || !IS_NUMBER(args[1])) {
        return ERROR_VAL_PTR("Invalid arguments for __index__(). Expected a container and a number.");
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
        return ERROR_VAL_PTR("clear() must be called on a container.");
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
        return ERROR_VAL_PTR("pop() must be called on a container.");
    }
    switch (OBJ_TYPE(args[0])) {
        case OBJ_LIST:
            if (argCount != 1) {
                return ERROR_VAL_PTR("pop() must be called on a list with no arguments.");
            }
            ObjList* list = AS_LIST(args[0]);
            if (list->count == 0) {
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
                return ERROR_VAL_PTR("pop() must be called on a dict with the key to pop.");
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
                return ERROR_VAL_PTR("Key not found in dict and no default provided.");
            }
            return NIL_VAL;
        default:
            return ERROR_VAL_PTR("pop() must be called on a container.");
    }
}

/**
 * Clone native function.
 * Shallow clones/copies a container.
 */
Value cloneNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_CONTAINER(args[0])) {
        printf("clone() must be called on a container.\n");
        return ERROR_VAL;
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
