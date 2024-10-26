/**
 * @file object.h
 */

#ifndef cslo_object_h
#define cslo_object_h

#include "common.h"
#include "value.h"

/**
 * Macro for getting the ObjType from a given Value.
 */
#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

/**
 * Macros for checking object types.
 */

/** Macro for checking the given object is a ObjString. */
#define IS_STRING(value)       isObjType(value, OBJ_STRING)

/**
 * Macros for converting values to objects.
 */

/** Macro for converting a Value to an ObjString. */
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))

/** Macro for returning the ObjString character array for the given Value. */
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)

/**
 * @enum ObjType
 */
typedef enum ObjType {
    OBJ_STRING,
} ObjType;

/**
 * @struct Obj
 * 
 * Struct for defining an Obj.
 * Stores the type enum a reference to the next object.
 * 
 * All Objs will 'inhereit' from this struct.
 */
struct Obj {
    ObjType type;
    struct Obj* next;
};

/**
 * Struct for ObjString.
 * 
 * As well as the base Obj, contains the length of the string
 * and a pointer to the actual string. 
 */
struct ObjString {
    Obj obj;
    int length;
    char* chars;
    uint32_t hash;
};

/**
 * Method for creating an ObjString an taking ownership of the given string.
 */
ObjString* takeString(char* chars, int length);

/**
 * Method for creating an ObjString and copying the given string onto the heap.
 */
ObjString* copyString(const char* chars, int length);

/**
 * Method for printing an object.
 */
void printObject(Value value);

/**
 * Method for checking a Value is a particular ObjType.
 */
static inline bool isObjType(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
