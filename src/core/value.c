/**
 * @file value.c
 */

#include <stdio.h>
#include <string.h>

#include "core/object.h"
#include "core/memory.h"
#include "core/value.h"

/**
 * Implemention of method to initialise a value array.
 *
 * Sets values to NULL and capacity/count to 0.
 */
void initValueArray(ValueArray* array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

/**
 * Method to grow a ValueArray.
 */
void growValueArray(ValueArray* array) {
    int oldCapacity = array->capacity;
    array->capacity = GROW_CAPACITY(oldCapacity);
    array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
}

/**
 * Method to shrink a ValueArray.
 */
void shrinkValueArray(ValueArray* array) {
    int newCapacity = array->capacity / 2;
    if (newCapacity < 8) {
        newCapacity = 8;
    }
    if (newCapacity < array->count) {
        newCapacity = array->count;
    }
    if (newCapacity < array->capacity) {
        array->values = GROW_ARRAY(Value, array->values, array->capacity, newCapacity);
        array->capacity = newCapacity;
    }
}

/**
 * Implementation of method to write a value to an array.
 *
 * If the array is out of capacity, grow the array to fit the new element.
 */
void writeValueArray(ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        growValueArray(array);
    }

    array->values[array->count] = value;
    array->count++;
}

/**
 * Implementation of method to free the array.
 */
void freeValueArray(ValueArray* array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

/**
 * Implementation of method to print a value.
 */
void printValue(Value value) {
    switch (value.type) {
        case VAL_BOOL:
            printf(AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NIL:
            printf("nil");
            break;
        case VAL_NUMBER:
            printf("%g", AS_NUMBER(value));
            break;
        case VAL_OBJ:
            printObject(value);
            break;
        case VAL_EMPTY:
            printf("<empty>");
            break;
        default:
            return;
    }
}

/**
 * Method for comparing two values for equality.
 *
 * First checks the types - if they're not the same then easy false.
 * Otherwise, get the raw values and compare.
 */
bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) {
        return false;
    }

    switch (a.type) {
        case VAL_BOOL:
            return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:
            return true;
        case VAL_NUMBER:
            return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ:
            if (OBJ_TYPE(a) != OBJ_TYPE(b)) {
                return false;
            }
            switch (OBJ_TYPE(a)) {
                case OBJ_STRING:
                    return AS_STRING(a)->length == AS_STRING(b)->length &&
                           memcmp(AS_STRING(a)->chars, AS_STRING(b)->chars, AS_STRING(a)->length) == 0;
                case OBJ_LIST:
                    ObjList* la = AS_LIST(a);
                    ObjList* lb = AS_LIST(b);
                    if (la->count != lb->count) return false;
                    for (int i = 0; i < la->count; i++) {
                        if (!valuesEqual(la->values.values[i], lb->values.values[i])) return false;
                    }
                    return true;
                default:
                    return AS_OBJ(a) == AS_OBJ(b);
            }
        case VAL_EMPTY:
            return true;
        default:
            return false;
    }
}

/**
 * Method for comparing values.
 */
int valueCompare(const void* a, const void* b) {
    const Value* va = (const Value*)a;
    const Value* vb = (const Value*)b;

    if (IS_NUMBER(*va) && IS_NUMBER(*vb)) {
        double diff = AS_NUMBER(*va) - AS_NUMBER(*vb);
        return (diff > 0) - (diff < 0);
    } else if (IS_STRING(*va) && IS_STRING(*vb)) {
        return strcmp(AS_CSTRING(*va), AS_CSTRING(*vb));
    } else {
        // Fallback: numbers < strings < others
        if (IS_NUMBER(*va)) return -1;
        if (IS_NUMBER(*vb)) return 1;
        if (IS_STRING(*va)) return -1;
        if (IS_STRING(*vb)) return 1;
        return 0;
    }
}

/**
 * Method for hashing a double.
 *
 * Taken from LUA's implementation for hashing a double.
 */
static uint32_t hashDouble(double value) {
    union BitCast {
        double value;
        uint32_t ints[2];
    };

    union BitCast cast;
    cast.value = (value) + 1.0;
    return cast.ints[0] + cast.ints[1];
}

/**
 * Method for hashing a value.
 */
uint32_t hashValue(Value value) {
    switch (value.type) {
        case VAL_BOOL:
            return AS_BOOL(value) ? 3 : 5;
        case VAL_NIL:
            return 7;
        case VAL_NUMBER:
            return hashDouble(AS_NUMBER(value));
        case VAL_OBJ:
            return AS_STRING(value)->hash;
        case VAL_EMPTY:
            return 0;
        default:
            printf("Unknown hash type: '%d'", value.type);
            return 1;
    }
}

/**
 * Method for getting a ValueType as a string.
 */
char* valueTypeToString(Value value) {
    switch (value.type) {
        case VAL_BOOL: return "bool";
        case VAL_NIL: return "nil";
        case VAL_NUMBER: return "number";
        case VAL_OBJ: {
            switch (OBJ_TYPE(value)) {
                case OBJ_STRING: return "string";
                case OBJ_LIST: return "list";
                case OBJ_CLASS: return "class";
                case OBJ_INSTANCE: return "instance";
                case OBJ_FUNCTION: return "function";
                case OBJ_NATIVE: return "native function";
                default: return "object";
            }
        }
        case VAL_EMPTY: return "empty";
        case VAL_ERROR: return "error";
        default: return "unknown";
    }
}
