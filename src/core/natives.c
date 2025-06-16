/**
 * @file natives.c
 */

#define _POSIX_C_SOURCE 200112L

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "core/natives.h"
#include "core/object.h"
#include "core/value.h"
#include "core/vm.h"

/**
 * Method for defining all our natives.
 */
void defineNatives() {
    defineNative("clock", clockNative);
    defineNative("exit", exitNative);
    defineNative("sleep", sleepNative);
    defineNative("time", timeNative);
    defineNative("print", printNative);
    defineNative("len", lenNative);

    // math functions
    defineNative("abs", absNative);
    defineNative("ceil", ceilNative);
    defineNative("floor", floorNative);
    defineNative("sqrt", sqrtNative);
    defineNative("sin", sinNative);
    defineNative("cos", cosNative);
    defineNative("tan", tanNative);
    defineNative("min", minNative);
    defineNative("max", maxNative);

    // random functions
    defineNative("random", randomNative);
    defineNative("randomInt", randomIntNative);
    defineNative("randomRange", randomRangeNative);
    defineNative("randomSeed", randomSeedNative);

    // os functions
    defineNative("getenv", getEnvNative);
    defineNative("setenv", setEnvNative);
    defineNative("unsetenv", unsetEnvNative);
}

/**
 * Method for defining a native function.
 * We wrap the C function pointer as an ObjNative and adds it to the globals
 * table with the gven name.
 *
 * Pushes the name and native values onto the stack to prevent GC.
 */
void defineNative(const char* name, NativeFn function) {
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, OBJ_VAL(AS_STRING(vm.stack[0])), vm.stack[1]);
    pop();
    pop();
}

/**
 * Clock native function.
 */
Value clockNative(int argCount, Value* args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

/**
 * Time native function.
 * Returns the number of seconds since epoch.
 */
Value timeNative(int argCount, Value* args) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return NUMBER_VAL((double)tv.tv_sec + (double)tv.tv_usec / 1e6);
}

/**
 * Sleep native function.
 */
Value sleepNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL;
    }
    double t = AS_NUMBER(args[0]);
    sleep(t);
    return NIL_VAL;
}

/**
 * Print native function.
 */
Value printNative(int argCount, Value* args) {
    if (argCount < 1) {
        return ERROR_VAL;
    }
    for (int i = 0; i < argCount; i++) {
        printValue(args[i]);
    }
    printf("\n");
    return NIL_VAL;
}

/**
 * Exit native function.
 *
 * Exits the program.
 */
Value exitNative(int argCount, Value* args) {
    if (argCount > 0 && !IS_NUMBER(args[0])) {
        return ERROR_VAL;
    }
    int status = 0;
    if (0 < argCount) {
        status = (int) AS_NUMBER(args[0]);
    }
    exit(status);
    // not reachable
    return NIL_VAL;
}

/**
 * len native function.
 */
Value lenNative(int argCount, Value* args) {
    if (argCount != 1 || (!IS_LIST(args[0]) && !IS_STRING(args[0]) && !IS_DICT(args[0]))) {
        printf("len() expects a single argument of type string, list, or dict.\n");
        return NIL_VAL;
    }
    switch (OBJ_TYPE(args[0])) {
        case OBJ_STRING:
             return NUMBER_VAL((double)AS_STRING(args[0])->length);
            break;
        case OBJ_LIST:
            return NUMBER_VAL((double)AS_LIST(args[0])->count);
            break;
        case OBJ_DICT:
            return NUMBER_VAL((double)AS_DICT(args[0])->data.count);
        default:
            printf("len() expects a single argument of type string or list.\n");
            return NIL_VAL;
    }
}

// CONTAINER FUNCTIONS
// shared by all containers (aka lists and dictionaries)

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

// LIST FUNCTIONS

/**
 * Append native function.
 */
Value appendNative(int argCount, Value *args) {
    if (argCount != 2 || !IS_LIST(args[0])) {
        printf("append() must be called on a list with one argument.");
        return NIL_VAL;
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
        printf("insert() must be called on a list with an index and a value.\n");
        return NIL_VAL;
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

    // Shift elements to the right
    // #ifdef DEBUG_LOGGING
    // printf("Inserting at idx=%d, count=%d\n", idx, list->count);
    // #endif
    // for (int i = list->count; i > idx; i--) {
    //     #ifdef DEBUG_LOGGING
    //     printf("shifting %d to %d\n", i-1, i);
    //     #endif
    //     list->values.values[i] = list->values.values[i - 1];
    // }

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
        printf("remove() must be called on a list with an index.\n");
        return NIL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    int idx = (int)AS_NUMBER(args[1]);
    if (idx < 0 || idx >= list->count) {
        printf("Index out of bounds for remove().\n");
        return NIL_VAL;
    }

    Value removed = list->values.values[idx];

    // Shift elements to the left
    // for (int i = idx; i < list->count - 1; i++) {
    //     list->values.values[i] = list->values.values[i + 1];
    // }
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
        printf("reverse() must be called on a list.\n");
        return NIL_VAL;
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
        printf("index() must be called on a list with a value.\n");
        return NIL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    if (list->count == 0) {
        printf("index() called on an empty list.\n");
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
        printf("count() must be called on a list with a value.\n");
        return NIL_VAL;
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
        printf("extend() must be called on a list with a list.\n");
        return NIL_VAL;
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
        printf("sort() must be called on a list.\n");
        return NIL_VAL;
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

/// DICT FUNCTIONS

/**
 * keys native function.
 * Returns the keys of a dictionary as a list.
 */
Value keysNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_DICT(args[0])) {
        printf("keys() must be called on a dict.\n");
        return NIL_VAL;
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
        printf("values() must be called on a dict.\n");
        return NIL_VAL;
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
        printf("get() must be called on a dict with a key.\n");
        return NIL_VAL;
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
        printf("update() must be called on a dict with another dict.\n");
        return NIL_VAL;
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
        printf("items() must be called on a dict.\n");
        return NIL_VAL;
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

// MATH FUNCTIONS

/**
 * Calculates the absolute value of a number.
 * If the argument is not a number, returns NIL_VAL or throws an error.
 */
Value absNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL;
    }
    double value = AS_NUMBER(args[0]);
    return NUMBER_VAL(value < 0 ? -value : value);
}

/**
 * Calculates the ceil of a number.
 * If the argument is not a number, returns NIL_VAL or throws an error.
 */
Value ceilNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL;
    }
    double value = AS_NUMBER(args[0]);
    return NUMBER_VAL(ceil(value));
}

/**
 * Calculates the floor of a number.
 * If the argument is not a number, returns NIL_VAL or throws an error.
 */
Value floorNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL;
    }
    double value = AS_NUMBER(args[0]);
    return NUMBER_VAL(floor(value));
}

/**
 * Calculates the square root of a number.
 * If the argument is not a number, returns NIL_VAL or throws an error.
 */
Value sqrtNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL;
    }
    double value = AS_NUMBER(args[0]);
    if (value < 0) {
        return ERROR_VAL;
    }
    return NUMBER_VAL(sqrt(value));
}

/**
 * Calculates the sine of a number.
 * If the argument is not a number, returns NIL_VAL or throws an error.
 */
Value sinNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL;
    }
    double value = AS_NUMBER(args[0]);
    return NUMBER_VAL(sin(value));
}

/**
 * Calculates the cosine of a number.
 * If the argument is not a number, returns NIL_VAL or throws an error.
 */
Value cosNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL;
    }
    double value = AS_NUMBER(args[0]);
    return NUMBER_VAL(cos(value));
}

/**
 * Calculates the tangent of a number.
 * If the argument is not a number, returns NIL_VAL or throws an error.
 */
Value tanNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL;
    }
    double value = AS_NUMBER(args[0]);
    return NUMBER_VAL(tan(value));
}

/**
 * Calculates the minimum of two numbers.
 * If the arguments are not numbers, returns NIL_VAL or throws an error.
 */
Value minNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        return ERROR_VAL;
    }
    double a = AS_NUMBER(args[0]);
    double b = AS_NUMBER(args[1]);
    return NUMBER_VAL(a < b ? a : b);
}

/**
 * Calculates the maximum of two numbers.
 * If the arguments are not numbers, returns NIL_VAL or throws an error.
 */
Value maxNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        return ERROR_VAL;
    }
    double a = AS_NUMBER(args[0]);
    double b = AS_NUMBER(args[1]);
    return NUMBER_VAL(a > b ? a : b);
}

// RANDOM FUNCTIONS

/**
 * Generates a random number between 0 and 1.
 */
Value randomNative(int argCount, Value* args) {
    if (argCount != 0) {
        return ERROR_VAL;
    }
    return NUMBER_VAL((double)rand() / RAND_MAX);
}

/**
 * Generates a random integer between the given range.
 * If the arguments are not numbers, returns ERROR_VAL or throws an error.
 */
Value randomIntNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        return ERROR_VAL;
    }
    double min = AS_NUMBER(args[0]);
    double max = AS_NUMBER(args[1]);
    if (min > max) {
        return ERROR_VAL; // or handle error
    }
    int range = (int)(max - min + 1);
    return NUMBER_VAL(min + (rand() % range));
}

/**
 * Generates a random number between the given range.
 * If the arguments are not numbers, returns ERROR_VAL or throws an error.
 */
Value randomRangeNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        return ERROR_VAL;
    }
    double min = AS_NUMBER(args[0]);
    double max = AS_NUMBER(args[1]);
    if (min > max) {
        return ERROR_VAL; // or handle error
    }
    double range = max - min;
    return NUMBER_VAL(min + ((double)rand() / RAND_MAX) * range);
}

/**
 * Sets the seed for the random number generator.
 */
Value randomSeedNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_NUMBER(args[0])) {
        return ERROR_VAL;
    }
    unsigned int seed = (unsigned int)AS_NUMBER(args[0]);
    srand(seed);
    return NIL_VAL;
}

/**
 * Gets the value of an environment variable.
 * Usage: getenv("HOME")
 */
Value getEnvNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL;
    }
    const char* name = AS_CSTRING(args[0]);
    char* value = getenv(name);
    if (value == NULL) {
        return NIL_VAL;
    }
    return OBJ_VAL(copyString(value, (int)strlen(value)));
}

/**
 * Sets the value of an environment variable.
 * Usage: setenv("MYVAR", "value")
 */
Value setEnvNative(int argCount, Value* args) {
    if (argCount != 2 || !IS_STRING(args[0]) || !IS_STRING(args[1])) {
        return ERROR_VAL;
    }
    const char* name = AS_CSTRING(args[0]);
    const char* value = AS_CSTRING(args[1]);
#if defined(_WIN32)
    // Windows uses _putenv_s
    int result = _putenv_s(name, value);
#else
    // POSIX uses setenv
    int result = setenv(name, value, 1);
#endif
    if (result != 0) {
        return ERROR_VAL;
    }
    return NIL_VAL;
}

/**
 * Unsets an environment variable.
 * Usage: unsetenv("MYVAR")
 */
Value unsetEnvNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL;
    }
    const char* name = AS_CSTRING(args[0]);
#if defined(_WIN32)
    // Windows uses _putenv_s with an empty value
    int result = _putenv_s(name, "");
#else
    // POSIX uses unsetenv
    int result = unsetenv(name);
#endif
    if (result != 0) {
        return ERROR_VAL;
    }
    return NIL_VAL;
}
