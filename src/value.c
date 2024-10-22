/**
 * @file value.c
 */

#include <stdio.h>

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
    printf("%g", value);
}
