/**
 * @file value.c
 */

#include <stdio.h>
#include <string.h>

#include "object.h"
#include "memory.h"
#include "value.h"

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
 * Implementation of method to write a value to an array.
 * 
 * If the array is out of capacity, grow the array to fit the new element.
 */
void writeValueArray(ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
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
            return AS_OBJ(a) == AS_OBJ(b);
        case VAL_EMPTY:
            return true;
        default:
            return false;
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
    }
}
