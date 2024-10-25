/**
 * @file value.h
 */

#ifndef cslo_value_h
#define cslo_value_h

#include "common.h"

/**
 * @enum ValueType
 */
typedef enum ValueType {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER
} ValueType;

typedef double Value;

/**
 * @struct ValueArray
 * 
 * Constant pool for values.
 */
typedef struct ValueArray {
    int capacity;
    int count;
    Value* values;
} ValueArray;

/**
 * Method for initialising a value array.
 */
void initValueArray(ValueArray* array);

/**
 * Method for writing to a value array.
 */
void writeValueArray(ValueArray* array, Value value);

/**
 * Method for freeing a value array.
 */
void freeValueArray(ValueArray* array);

/**
 * Method for printing a cslo value.
 */
void printValue(Value value);

#endif
