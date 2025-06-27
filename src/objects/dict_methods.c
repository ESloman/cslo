/**
 * @file dict_methods.c
 * @brief Implementation of dict methods in CSLO.
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "builtins/util.h"
#include "core/memory.h"
#include "core/object.h"
#include "core/table.h"
#include "core/value.h"
#include "objects/dict_methods.h"


// forward declarations of native functions
Value keysNative(int argCount, Value* args);
Value valuesNative(int argCount, Value* args);
Value getNative(int argCount, Value* args);
Value updateNative(int argCount, Value* args);
Value itemsNative(int argCount, Value* args);


/**
 * @brief Registers dict methods for the given ObjClass.
 * @param dictClass The ObjClass representing the string type.
 */
void registerDictMethods(ObjClass* cls) {
    defineBuiltIn(&cls->methods, "keys", keysNative, 1, 1, ((ParamInfo[]){{copyString("self", 4), true}}));
    defineBuiltIn(&cls->methods, "values", valuesNative, 1, 1, ((ParamInfo[]){{copyString("self", 4), true}}));
    defineBuiltIn(&cls->methods, "get", getNative, 2, 3, ((ParamInfo[]){{copyString("self", 4), true}, {copyString("key", 3), true}, {copyString("default", 7), false}}));
    defineBuiltIn(&cls->methods, "update", updateNative, 2, 2, ((ParamInfo[]){{copyString("self", 4), true}, {copyString("other", 6), true}}));
    defineBuiltIn(&cls->methods, "items", itemsNative, 1, 1, ((ParamInfo[]){{copyString("self", 4), true}}));
}

/**
 * keys native function.
 * Returns the keys of a dictionary as a list.
 */
Value keysNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_DICT(args[0])) {
        return ERROR_VAL_PTR("keys() must be called on a dict.");
    }
    ObjDict* dict = AS_DICT(args[0]);
    ObjList* keys = newList();
    for (int i = 0; i < dict->data.capacity; i++) {
        Entry* entry = &dict->data.entries[i];
        if (!IS_EMPTY(entry->key) && !IS_NIL(entry->value)) {
            if (keys->count >= keys->values.capacity) {
                growValueArray(&keys->values);
            }
            keys->values.values[keys->count++] = entry->key;
        }
    }
    return OBJ_VAL(keys);
}

/**
 * values native function.
 * Returns the values of a dictionary as a list.
 */
Value valuesNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_DICT(args[0])) {
        return ERROR_VAL_PTR("values() must be called on a dict.");
    }
    ObjDict* dict = AS_DICT(args[0]);
    ObjList* values = newList();
    for (int i = 0; i < dict->data.capacity; i++) {
        Entry* entry = &dict->data.entries[i];
        if (!IS_EMPTY(entry->key) && !IS_NIL(entry->value)) {
            if (values->count >= values->values.capacity) {
                growValueArray(&values->values);
            }
            values->values.values[values->count++] = entry->value;
        }
    }
    return OBJ_VAL(values);
}

/**
 * get native function.
 * Retrieves a value from a dictionary by its key.
 * Returns the value if found, or NIL_VAL if not found.
 * Optionally, returns a default if not found instead of NIL_VAL.
 */
Value getNative(int argCount, Value* args) {
    if (argCount < 2 || !IS_DICT(args[0])) {
        return ERROR_VAL_PTR("get() must be called on a dict with a key.");
    }
    ObjDict* dict = AS_DICT(args[0]);
    Value key = args[1];
    Value value;
    if (tableGet(&dict->data, key, &value)) {
        return value; // return the found value
    } else {
        if (argCount == 3) {
            // if a default value is provided, return it
            return args[2];
        }
        return NIL_VAL; // key not found, return nil
    }
}

/**
 * update native function.
 * Updates a dict with another dict.
 * If the key already exists, it updates the value.
 * If the key does not exist, it adds the key-value pair.
 */
Value updateNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_DICT(args[0]) || !IS_DICT(args[1])) {
        return ERROR_VAL_PTR("update() must be called on a dict with another dict.");
    }
    ObjDict* target = AS_DICT(args[0]);
    ObjDict* source = AS_DICT(args[1]);

    for (int i = 0; i < source->data.capacity; i++) {
        Entry* entry = &source->data.entries[i];
        if (!IS_EMPTY(entry->key) && !IS_NIL(entry->value)) {
            // Set the key-value pair in the target dict
            tableSet(&target->data, entry->key, entry->value);
        }
    }
    return NIL_VAL;
}

/**
 * items native method.
 * Returns a list of key-value pairs from a dict.
 */
Value itemsNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_DICT(args[0])) {
        return ERROR_VAL_PTR("items() must be called on a dict.");
    }
    ObjDict* dict = AS_DICT(args[0]);
    ObjList* items = newList();
    for (int i = 0; i < dict->data.capacity; i++) {
        Entry* entry = &dict->data.entries[i];
        if (!IS_EMPTY(entry->key) && !IS_NIL(entry->value)) {
            ObjList* pair = newList();
            growValueArray(&pair->values);
            pair->values.values[0] = entry->key; // key
            pair->values.values[1] = entry->value; // value
            pair->count = 2;
            pair->values.count = 2;
            if (items->values.capacity < items->count + 1) {
                growValueArray(&items->values);
            }
            items->values.values[items->count++] = OBJ_VAL(pair);
        }
    }
    return OBJ_VAL(items);
}
