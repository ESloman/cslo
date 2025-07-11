/**
 * @file memory.c
 */

#include <stdlib.h>

#include "core/gc.h"
#include "core/memory.h"
#include "core/object.h"
#include "core/vm.h"

/**
 * Implementation of reallocate function.
 */
void *reallocate(void *pointer, size_t oldSize, size_t newSize) {
    vm.bytesAllocated += newSize - oldSize;

    if (newSize > oldSize) {
        if (vm.bytesAllocated > vm.nextGC) {
            collectGarbage();
        }
    }

    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);
    if (result == NULL) exit(1);
    return result;
}

/**
 * Method for freeing an object.
 *
 * Switches based on type and ensures we free all aspects of the Obj.
 */
void freeObject(Obj* object) {
    switch (object->type) {
        case OBJ_BOUND_METHOD: {
            FREE(ObjBoundMethod, object);
            break;
        }
        case OBJ_CLASS: {
            ObjClass* sClass = (ObjClass*)object;
            freeTable(&sClass->methods);
            FREE(ObjClass, object);
            break;
        }
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalueCount);
            FREE(OBJ_CLOSURE, object);
            break;
        }
        case OBJ_INSTANCE: {
            ObjInstance* instance = (ObjInstance*)object;
            freeTable(&instance->fields);
            FREE(ObjInstance, object);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            freeChunk(&function->chunk);
            FREE(ObjFunction, object);
            break;
        }
        case OBJ_NATIVE: {
            FREE(ObjNative, object);
            break;
        }
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(ObjString, object);
            break;
        }
        case OBJ_UPVALUE: {
            FREE(ObjUpvalue, object);
            break;
        }
        case OBJ_LIST: {
            ObjList* list = (ObjList*)object;
            FREE_ARRAY(Value, list->values.values, list->values.capacity);
            FREE(ObjList, object);
            break;
        }
        case OBJ_DICT: {
            ObjDict* dict = (ObjDict*)object;
            freeTable(&dict->data);
            FREE(ObjDict, object);
            break;
        }
        case OBJ_MODULE: {
            ObjModule* module = (ObjModule*)object;
            freeTable(&module->methods);
            FREE(ObjModule, object);
            break;
        }
        case OBJ_ENUM: {
            ObjEnum* sEnum = (ObjEnum*)object;
            freeTable(&sEnum->values);
            FREE(ObjEnum, object);
            break;
        }
        case OBJ_FILE: {
            ObjFile* sFile = (ObjFile*)object;
            if (sFile->file && !sFile->closed) {
                fclose(sFile->file);
                sFile->closed = true;
            }
            FREE(ObjFile, object);
            break;
        }
        case OBJ_ERROR: {
            FREE(ObjError, object);
            break;
        }
        default:
            return;
    }
}

/**
 * Method for freeing all the objects.
 *
 * This walks the linked list of Objs and calls freeObject for each.
 */
void freeObjects() {
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
  }
}
