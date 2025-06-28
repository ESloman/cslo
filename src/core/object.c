/**
 * @file object.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/memory.h"
#include "core/object.h"
#include "core/table.h"
#include "core/value.h"
#include "core/vm.h"

/** Macro for wrapping allocateObject with the right type. */
#define ALLOCATE_OBJ(type, objectType) \
      (type*)allocateObject(sizeof(type), objectType)

/**
 * Method for creating the given ObjType with the right size.
 */
static Obj* allocateObject(size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;
    object->mark = !vm.markValue;
    object->next = vm.objects;
    vm.objects = object;

#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif

    return object;
}

/**
 * Method for creating a new bound method.
 */
ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method) {
    ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

/**
 * Method for creating a new ObjClass.
 */
ObjClass* newClass(ObjString* name, ObjClass* superClass) {
    ObjClass* sClass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    sClass->name = name;
    sClass->superclass = superClass;
    initTable(&sClass->methods);
    initTable(&sClass->nativeProperties);
    return sClass;
}

/**
 * Method for creating a new closure.
 */
ObjClosure* newClosure(ObjFunction* function) {
    ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);
    for (int i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = NULL;
    }

    ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

/**
 * Method for creating a new ObjInstance.
 */
ObjInstance* newInstance(ObjClass* sClass) {
    ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->sClass = sClass;
    initTable(&instance->fields);
    return instance;
}

/**
 * Method for creating a ObjFunction.
 */
ObjFunction* newFunction() {
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->name = NULL;
    function->file = NULL;
    function->upvalueCount = 0;
    initChunk(&function->chunk);
    return function;
}

/**
 * Method for creating a new ObjNative.
 */
ObjNative* newNative(NativeFn function, int arityMin, int arityMax, ParamInfo* params) {
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    native->arityMin = arityMin;
    native->arityMax = arityMax;
    native->params = params;
    return native;
}

ParamInfo* createParamInfoArray(ParamInfo* src, int count) {
    if (!src || count == 0) return NULL;
    ParamInfo* dest = malloc(sizeof(ParamInfo) * count);
    for (int i = 0; i < count; i++) {
        dest[i].name = src[i].name;
        dest[i].required = src[i].required;
    }
    return dest;
}

/**
 * Method for creating a new ObjNativeProperty.
 */
ObjNativeProperty* newNativeProperty(NativeProperty getter) {
    ObjNativeProperty* nativeProperty = ALLOCATE_OBJ(ObjNativeProperty, OBJ_NATIVE_PROPERTY);
    nativeProperty->getter = getter;
    return nativeProperty;
}

/**
 * Method for creating a new upvalue.
 */
ObjUpvalue* newUpvalue(Value* slot) {
    ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->closed = NIL_VAL;
    upvalue->location = slot;
    upvalue->next = NULL;
    return upvalue;
}

ObjList* newList() {
    ObjList* list = ALLOCATE_OBJ(ObjList, OBJ_LIST);
    list->count = 0;
    initValueArray(&list->values);
    list->sClass = vm.listClass;
    return list;
}

ObjDict* newDict() {
    ObjDict* dict = ALLOCATE_OBJ(ObjDict, OBJ_DICT);
    initTable(&dict->data);
    dict->sClass = vm.dictClass;
    return dict;
}

ObjModule* newModule() {
    ObjModule* module = ALLOCATE_OBJ(ObjModule, OBJ_MODULE);
    initTable(&module->methods);
    return module;
}

ObjEnum* newEnum(ObjString* name) {
    ObjEnum* sEnum = ALLOCATE_OBJ(ObjEnum, OBJ_ENUM);
    initTable(&sEnum->values);
    sEnum->name = name;
    return sEnum;
}

ObjFile* newFile(FILE* file, FileMode mode, ObjString* name) {
    ObjFile* sFile = ALLOCATE_OBJ(ObjFile, OBJ_FILE);
    sFile->file = file;
    sFile->closed = false;
    sFile->mode = mode;
    sFile->name = name;
    return sFile;
}

/**
 * Method for creating a new ObjError.
 */
ObjError* newError(const char* message) {
    ObjError* error = ALLOCATE_OBJ(ObjError, OBJ_ERROR);
    error->message = copyString(message, strlen(message) + 1);
    return error;
}

/**
 * Method for creating an ObjString.
 *
 * This creates the ObjString and
 * allocates enough memory for it.
 * Initialises the other struct values too.
 */
static ObjString* allocateString(char* chars, int length, uint32_t hash) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;

    push(OBJ_VAL(string));
    tableSet(&vm.strings, OBJ_VAL(string), NIL_VAL);
    pop();
    return string;
}

/**
 * Method for generating a hash for a string.
 */
static uint32_t hashString(const char* key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

/**
 * Method for creating an ObjString an taking ownership of the given string.
 */
ObjString* takeString(char* chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }
    return allocateString(chars, length, hash);
}

/**
 * Method for creating an ObjString and copying the given string onto the heap.
 */
ObjString* copyString(const char* chars, int length) {
    char* heapChars = ALLOCATE(char, length + 1);
    uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) {
        return interned;
    }
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length, hash);
}

/**
 * Method for prining a function.
 */
static void printFunction(ObjFunction* function) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);
}

/**
 * Method for printing an object.
 */
void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_BOUND_METHOD: {
            printFunction(AS_BOUND_METHOD(value)->method->function);
            break;
        }
        case OBJ_CLASS: {
            printf("%s", AS_CLASS(value)->name->chars);
            break;
        }
        case OBJ_CLOSURE: {
            printFunction(AS_CLOSURE(value)->function);
            break;
        }
        case OBJ_INSTANCE:
        printf("%s instance", AS_INSTANCE(value)->sClass->name->chars);
        break;
        case OBJ_FUNCTION: {
            printFunction(AS_FUNCTION(value));
            break;
        }
        case OBJ_NATIVE: {
            printf("<native fn>");
            break;
        }
        case OBJ_STRING: {
            printf("%s", AS_CSTRING(value));
            break;
        }
        case OBJ_UPVALUE: {
            printf("upvalue");
            break;
        }
        case OBJ_LIST: {
            ObjList *list = AS_LIST(value);
            printf("list[%d]: [", list->count);
            for (int i = 0; i < list->count; i++) {
                printValue(list->values.values[i]);
                if (i < list->count - 1) {
                    printf(", ");
                }
            }
            printf("]");
            break;
        }
        case OBJ_DICT: {
            ObjDict* dict = AS_DICT(value);
            printf("dict[%d]: {", dict->data.count);
            for (int i = 0; i < dict->data.capacity; i++) {
                Entry* entry = &dict->data.entries[i];
                if (entry->key.type != VAL_NIL && entry->key.type != VAL_EMPTY) {
                    printValue(entry->key);
                    printf(": ");
                    printValue(entry->value);
                    if (i < dict->data.capacity - 1) {
                        printf(", ");
                    }
                }
            }
            printf("}");
            break;
        }
        case OBJ_ENUM: {
            ObjEnum* sEnum = AS_ENUM(value);
            printf("enum %s: {", sEnum->name->chars);
            int first = 1;
            for (Entry* entry = sEnum->values.entries; entry < sEnum->values.entries + sEnum->values.capacity; entry++) {
                if (entry->key.type != VAL_NIL && entry->key.type != VAL_EMPTY) {
                    if (!first) {
                        printf(", ");
                    }
                    first = 0;
                    printValue(entry->key);
                    printf(": ");
                    printValue(entry->value);
                }
            }
            printf("}");
            break;
        }
        case OBJ_FILE: {
            ObjFile* sFile = AS_FILE(value);
            printf("file %p", (void*)sFile->file);
            if (sFile->closed) {
                printf(" (closed)");
            } else {
                printf(" (open)");
            }
            break;
        }
        case OBJ_ERROR:
            // shouldn't be printed directly anyway
            printf("<error>");
            break;
        default:
            return;
    }
}
