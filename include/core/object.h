/**
 * @file object.h
 */

#ifndef cslo_object_h
#define cslo_object_h

#include <stdio.h>

#include "chunk.h"
#include "core/common.h"
#include "table.h"
#include "core/value.h"

/**
 * Macro for getting the ObjType from a given Value.
 */
#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

/**
 * Macros for checking object types.
 */

/** Macro for checking if the given object is an ObjBoundMethod. */
#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)

/** Macro for checking if the given object is an ObjClass. */
#define IS_CLASS(value)        isObjType(value, OBJ_CLASS)

/** Macro for checking if the given object is an ObjInstance. */
#define IS_INSTANCE(value)     isObjType(value, OBJ_INSTANCE)

/** Macro for checking if the given object is an ObjClosure. */
#define IS_CLOSURE(value)      isObjType(value, OBJ_CLOSURE)

/** Macro for checking the given object is an ObjFunction. */
#define IS_FUNCTION(value)     isObjType(value, OBJ_FUNCTION)

/** Macro for checking the given object is an ObjNative. */
#define IS_NATIVE(value)       isObjType(value, OBJ_NATIVE)

/** Macro for checking the given object is an ObjNativeProperty. */
#define IS_NATIVE_PROPERTY(value) isObjType(value, OBJ_NATIVE_PROPERTY)

/** Macro for checking the given object is an ObjString. */
#define IS_STRING(value)       isObjType(value, OBJ_STRING)

/** Macro for checking the given object is a container. */
#define IS_CONTAINER(value)    (isObjType(value, OBJ_LIST) || isObjType(value, OBJ_DICT))

/** Macro for checking the given object is an ObjList. */
#define IS_LIST(value)         isObjType(value, OBJ_LIST)

/** Macro for checking the given object is an ObjDict. */
#define IS_DICT(value)        isObjType(value, OBJ_DICT)

/** Macro for checking the given object is an ObjEnum. */
#define IS_ENUM(value)        isObjType(value, OBJ_ENUM)

/** Macro for checking the given object is an ObjModule. */
#define IS_MODULE(value)      isObjType(value, OBJ_MODULE)

/** Macro for checking the given object is an ObjFile. */
#define IS_FILE(value)        isObjType(value, OBJ_FILE)

/**
 * Macros for converting values to objects.
 */

/** Macro for convering the Value to an ObjBoundMethod. */
#define AS_BOUND_METHOD(value) ((ObjBoundMethod*)AS_OBJ(value))

/** Macro for converting the Value to an ObjClass. */
#define AS_CLASS(value)        ((ObjClass*)AS_OBJ(value))

/** Macro for converting the Value to an ObjInstance. */
#define AS_INSTANCE(value)     ((ObjInstance*)AS_OBJ(value))

/** Macro for converting the Value to an ObjClosure. */
#define AS_CLOSURE(value)      ((ObjClosure*)AS_OBJ(value))

/** Macro for converting the Value to an ObjFunction. */
#define AS_FUNCTION(value)     ((ObjFunction*)AS_OBJ(value))

/** Macro for converting the Value to an ObjNative. */
#define AS_NATIVE(value)       (((ObjNative*)AS_OBJ(value))->function)

/** Macro for converting the Value to an ObjNativeProperty. */
#define AS_NATIVE_PROPERTY(value) (((ObjNativeProperty*)AS_OBJ(value))->getter)

/** Macro for converting a Value to an ObjString. */
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))

/** Macro for returning the ObjString character array for the given Value. */
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)

/** Macro for converting a Value to an ObjList. */
#define AS_LIST(value)        ((ObjList*)AS_OBJ(value))

/** Macro for convering a Value to an ObjDict. */
#define AS_DICT(value)        ((ObjDict*)AS_OBJ(value))

/** Macro for converting a Value to an ObjEnum. */
#define AS_ENUM(value)        ((ObjEnum*)AS_OBJ(value))

/** Macro for converting a Value to an ObjModule. */
#define AS_MODULE(value)      ((ObjModule*)AS_OBJ(value))

/** Macro for converting a Value to an ObjFile. */
#define AS_FILE(value)        ((ObjFile*)AS_OBJ(value))

/** Macro for convering a Value to an ObjError. */
#define AS_ERROR(value)       ((ObjError*)AS_OBJ(value))

/**
 * @enum ObjType
 */
typedef enum ObjType {
    OBJ_BOUND_METHOD,
    OBJ_CLASS,
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_NATIVE_PROPERTY,
    OBJ_INSTANCE,
    OBJ_STRING,
    OBJ_UPVALUE,
    OBJ_LIST,
    OBJ_DICT,
    OBJ_MODULE,
    OBJ_ENUM,
    OBJ_FILE,
    OBJ_ERROR,
} ObjType;

/**
 * @enum FileMode
 *
 * Enum for defining the file mode.
 * Used to determine how a file should be opened.
 */
typedef enum FileMode {
    FILE_READ,
    FILE_WRITE,
    FILE_APPEND
} FileMode;

/**
 * @struct Obj
 *
 * Struct for defining an Obj.
 * Stores the type enum a reference to the next object.
 *
 * All Objs will 'inherit' from this struct.
 */
struct Obj {
    ObjType type;
    bool mark;
    struct Obj* next;
};

/**
 * @struct ParamInfo
 *
 * Struct for defining a parameter information.
 * Contains the name of the parameter and whether it is required.
 */
typedef struct ParamInfo {
    ObjString* name;
    bool required;
} ParamInfo;

/**
 * @struct ObjFunction
 */
typedef struct ObjFunction {
    Obj obj;
    int arity;
    int upvalueCount;
    Chunk chunk;
    ObjString* file;
    ObjString* name;
} ObjFunction;


typedef Value (*NativeFn)(int argCount, Value* args, ParamInfo* params);
typedef Value (*NativeProperty)(Value arg);

/**
 * @struct ObjNative defintion
 */
typedef struct ObjNative {
    Obj obj;
    NativeFn function;
    int arityMin;
    int arityMax;
    ParamInfo* params;
} ObjNative;

/**
 * @struct ObjNativeProperty defintion
 */
typedef struct ObjNativeProperty {
    Obj obj;
    NativeProperty getter;
} ObjNativeProperty;

/**
 * @struct ObjString.
 *
 * As well as the base Obj, contains the length of the string
 * and a pointer to the actual string.
 */
typedef struct ObjString {
    Obj obj;
    int length;
    char* chars;
    uint32_t hash;
} ObjString;

/**
 * @struct ObjUpvalue
 */
typedef struct ObjUpvalue {
    Obj obj;
    Value* location;
    Value closed;
    struct ObjUpvalue* next;
} ObjUpvalue;

/**
 * @struct ObjClosure
 *
 * Contains the Obj header and a pointer to the actual function.
 */
typedef struct ObjClosure {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    int upvalueCount;
} ObjClosure;

/**
 * @struct ObjClass.
 *
 * Contains the usual Obj header and other class properties.
 */
typedef struct ObjClass{
    Obj obj;
    ObjString* name;
    struct ObjClass* superclass;
    Table methods;
    Table nativeProperties;
} ObjClass;

/**
 * @struct ObjInstance
 */
typedef struct ObjInstance {
    Obj obj;
    ObjClass* sClass;
    Table fields;
} ObjInstance;

/**
 * @struct ObjBoundMethod
 */
typedef struct ObjBoundMethod {
    Obj obj;
    Value receiver;
    ObjClosure* method;
} ObjBoundMethod;

/**
 * @struct ObjList
 */
typedef struct {
    Obj obj;
    int count;
    int capacity;
    ObjClass* sClass;
    ValueArray values;
} ObjList;

/**
 * @struct ObjDict
 */
typedef struct {
    Obj obj;
    ObjClass* sClass;
    Table data;
} ObjDict;

/**
 * @struct ObjModule
 */
typedef struct {
    Obj obj;
    Table methods;
} ObjModule;

/**
 * @struct ObjEnum
 */
typedef struct {
    Obj obj;
    ObjString* name;
    Table values;
} ObjEnum;

/**
 * @struct ObjFile
 */
typedef struct {
    Obj obj;
    FILE* file;
    bool closed;
    FileMode mode;
    ObjString* name;
} ObjFile;

/**
 * @struct ObjError
 */
typedef struct {
    Obj obj;
    ObjString* message;
} ObjError;

/**
 * Method for creating a new bound method.
 */
ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method);

/**
 * Method for creating a new ObjClass.
 */
ObjClass* newClass(ObjString* name, ObjClass* superClass);

/**
 * Method for creating a ObjClosure.
 */
ObjClosure* newClosure(ObjFunction* function);

/**
 * Method for creating a new ObjInstance.
 */
ObjInstance* newInstance(ObjClass* sClass);

/**
 * Method for creating a ObjFunction.
 */
ObjFunction* newFunction();

/**
 * Method for creating a new ObjNative.
 */
ObjNative* newNative(NativeFn function, int arityMin, int arityMax, ParamInfo* params);

/**
 *
 */
ParamInfo* createParamInfoArray(ParamInfo* src, int count);

/**
 * Method for creating a new ObjNativeProperty.
 */
ObjNativeProperty* newNativeProperty(NativeProperty getter);

/**
 * Method for creating an ObjString an taking ownership of the given string.
 */
ObjString* takeString(char* chars, int length);

/**
 * Method for creating an ObjString and copying the given string onto the heap.
 */
ObjString* copyString(const char* chars, int length);

/**
 * Method for creating a new upvalue.
 */
ObjUpvalue* newUpvalue(Value* slot);

/**
 * Method for creating a new ObjList.
 */
ObjList* newList();

/**
 * Method for creating a new ObjDict.
 */
ObjDict* newDict();

/**
 * Method for creating a new ObjModule.
 */
ObjModule* newModule();

/**
 * Method for creating a new ObjEnum.
 */
ObjEnum* newEnum(ObjString* name);

/**
 * Method for creating a new ObjFile.
 */
ObjFile* newFile(FILE* file, FileMode mode, ObjString* name);

/**
 * Method for creating a new ObjError.
 */
ObjError* newError(const char* message);

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

// helper macro for defining inline params
#define PARAMS(...)  createParamInfoArray(((ParamInfo[]){__VA_ARGS__}), sizeof((ParamInfo[]){__VA_ARGS__})/sizeof(ParamInfo))

#endif
