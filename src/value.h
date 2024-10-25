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

/**
 * @struct Value
 * 
 * Defines a Value in slo.
 * Uses a union for the actual type values.
 */
typedef struct Value {
    ValueType type;
    union {
        bool boolean;
        double number;
    } as;
} Value;

/**
 * Macros for checking the value types.
 */

/** Macro for checking if the given value is a VAL_BOOL. */
#define IS_BOOL(value)    ((value).type == VAL_BOOL)

/** Macro for checking if the given value is a VAL_NIL. */
#define IS_NIL(value)     ((value).type == VAL_NIL)

/** Macro for checking if the given value is a VAL_NUMBER. */
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)



/**
 * Macros for convering slo Values into C values.
 */

/** Macro for convering a boolean Value into a bool. */
#define AS_BOOL(value)    ((value).as.boolean)

/** Macro for convering a number Value into a double. */
#define AS_NUMBER(value)  ((value).as.number)

/**
 * Macros for converting C values into slo Values.
 */

/**Macro for creating a boolean Value from true/false. */
#define BOOL_VAL(value)   ((Value){VAL_BOOL, {.boolean = value}})

/**Macro for creating a nil Value - sets number to 0. */
#define NIL_VAL           ((Value){VAL_NIL, {.number = 0}})

/**Macro for creating a number Value from the given number. */
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})

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
 * Method for comparing two values.
 */
bool valuesEqual(Value a, Value b);

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
