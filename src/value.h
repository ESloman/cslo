/**
 * @file value.h
 */

#ifndef cslo_value_h
#define cslo_value_h

#include "common.h"

/**
 * @struct Obj
 */
typedef struct Obj Obj;

/**
 * @struct ObjString
 */
typedef struct ObjString ObjString;

/**
 * @enum ValueType
 */
typedef enum ValueType {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ,
    VAL_EMPTY // used for tombstoning
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
        Obj* obj;
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

/** Macro for checking if the given value is a VAL_OBJ. */
#define IS_OBJ(value)     ((value).type == VAL_OBJ)

/** Macro for checking if the given value is a VAL_EMPTY. */
#define IS_EMPTY(value)   ((value).type == VAL_EMPTY)

/**
 * Macros for converting slo Values into C values.
 */

/** Macro for converting a boolean Value into a bool. */
#define AS_BOOL(value)    ((value).as.boolean)

/** Macro for converting a number Value into a double. */
#define AS_NUMBER(value)  ((value).as.number)

/** Macro for converting a value to an object. */
#define AS_OBJ(value)     ((value).as.obj)

/**
 * Macros for converting C values into slo Values.
 */

/**Macro for creating a boolean Value from true/false. */
#define BOOL_VAL(value)   ((Value){VAL_BOOL, {.boolean = value}})

/**Macro for creating a nil Value - sets number to 0. */
#define NIL_VAL           ((Value){VAL_NIL, {.number = 0}})

/**Macro for creating a number Value from the given number. */
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})

/** Macro for creating a Obj Value from the given value. */
#define OBJ_VAL(object)   ((Value){VAL_OBJ, {.obj = (Obj*)object}})

/** Macro for creating an Empty value. */
#define EMPTY_VAL         ((Value){VAL_EMPTY, { .number = 0 } })

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

/**
 * Method for hashing a Value.
 */
uint32_t hashValue(Value value);

#endif
