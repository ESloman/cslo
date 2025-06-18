/**
 * @file gc.c
 */

#include <stdlib.h>

#include "core/common.h"
#include "compiler/compiler.h"
#include "core/gc.h"
#include "core/memory.h"
#include "core/table.h"
#include "core/value.h"
#include "core/vm.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "core/debug.h"
#endif

#define GC_HEAP_GROW_FACTOR 2

/**
 * Method for gc processing.
 */
void collectGarbage() {

#ifdef DEBUG_LOG_GC
    printf("--> gc begin\n");
    size_t before = vm.bytesAllocated;
#endif

    markRoots();
    traceReferences();
    tableRemoveWhite(&vm.strings);
    sweep();

    vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;
    vm.markValue = !vm.markValue;

#ifdef DEBUG_LOG_GC
    printf("--> gc end\n");
    printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
         before - vm.bytesAllocated, before, vm.bytesAllocated,
         vm.nextGC);
#endif

}

/**
 * Method for marking roots.
 */
void markRoots() {

#ifdef DEBUG_LOG_GC
    printf("--> marking roots\n");
#endif

    for (const Value* slot = vm.stack; slot < vm.stackTop; slot++) {
        markValue(*slot);
    }

    markTable(&vm.globals);

    for (int f = 0; f < vm.frameCount; f++) {
        markObject((Obj*)vm.frames[f].closure);
    }

    for (ObjUpvalue* upvalue = vm.openUpvalues; upvalue != NULL; upvalue = upvalue->next) {
        markObject((Obj*)upvalue);
    }

    markCompilerRoots();
    markObject((Obj*)vm.initString);

#ifdef DEBUG_LOG_GC
    printf("--> finished marking roots\n");
#endif
}

/**
 * Method for marking a value.
 */
void markValue(Value value) {
    if (IS_OBJ(value)) {
        markObject(AS_OBJ(value));
    } else {
#ifdef DEBUG_LOG_GC
        printf("--> didn't mark as not an object: ");
        printValue(value);
        printf("\n");
#endif
    }
}

/**
 * Method for marking an object.
 */
void markObject(Obj* object) {
    if (object == NULL) {
        return;
    }

    if (object->mark == vm.markValue) {
        return;
    }

#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*)object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif

    object->mark = vm.markValue;

    /**
     * Optimisation as ObjStrings and ObjNatives have no outgoing references,
     * we don't need to process them further so don't need adding to the graystack.
     */
    if (object->type == OBJ_STRING || object->type == OBJ_NATIVE) {
        return;
    }

    if (vm.grayCapacity < vm.grayCount + 1) {
        vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
        vm.grayStack = (Obj**)realloc(vm.grayStack, sizeof(Obj*) * vm.grayCapacity);
        if (vm.grayStack == NULL) {
            exit(1);
        }
    }

    vm.grayStack[vm.grayCount++] = object;
}

/**
 * Method for marking a table.
 */
void markTable(Table* table) {
#ifdef DEBUG_LOG_GC
    printf("--> marking table %p\n", (void*)table);
#endif

    for (int i = 0; i < table->capacity; i++) {
        const Entry* entry = &table->entries[i];
        markValue(entry->key);
        markValue(entry->value);
    }

#ifdef DEBUG_LOG_GC
    printf("--> finished marking table %p\n", (void*)table);
#endif
}

/**
 * Method for marking a ValueArray.
 */
void markArray(ValueArray* array) {
#ifdef DEBUG_LOG_GC
    printf("--> marking array %p\n", (void*)array);
#endif
    for (int a = 0; a < array->count; a++) {
        markValue(array->values[a]);
    }
#ifdef DEBUG_LOG_GC
    printf("--> finished marking array %p\n", (void*)array);
#endif
}

/**
 * Method for marking compiler roots.
 */
void markCompilerRoots() {
#ifdef DEBUG_LOG_GC
    printf("--> marking compiler roots\n");
#endif

    Compiler* compiler = current;
    while (compiler != NULL) {
        markObject((Obj*)compiler->function);
        compiler = compiler->enclosing;
    }

#ifdef DEBUG_LOG_GC
    printf("--> finished marking compiler roots\n");
#endif
}

/**
 * Method for tracing references.
 */
void traceReferences() {
#ifdef DEBUG_LOG_GC
    printf("--> tracing references\n");
#endif

    while (vm.grayCount > 0) {
        Obj* object = vm.grayStack[--vm.grayCount];
        blackenObject(object);
    }

#ifdef DEBUG_LOG_GC
    printf("--> finished tracing references\n");
#endif
}

/**
 * Method for blackening an object.
 */
void blackenObject(Obj* object) {
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void*)object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif

    switch (object->type) {
        case OBJ_BOUND_METHOD: {
            ObjBoundMethod* bound = (ObjBoundMethod*)object;
            markValue(bound->receiver);
            markObject((Obj*)bound->method);
            break;
        }
        case OBJ_CLASS: {
            ObjClass* sClass = (ObjClass*)object;
            markObject((Obj*)sClass->name);
            markTable(&sClass->methods);
            break;
        }
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            markObject((Obj*)closure->function);
            for (int c = 0; c < closure->upvalueCount; c++) {
                markObject((Obj*)closure->upvalues[c]);
            }
            break;
        }
        case OBJ_INSTANCE: {
            ObjInstance* instance = (ObjInstance*)object;
            markObject((Obj*)instance->sClass);
            markTable(&instance->fields);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            markObject((Obj*)function->name);
            markArray(&function->chunk.constants);
            break;
        }
        case OBJ_UPVALUE: {
            markValue(((ObjUpvalue*)object)->closed);
            break;
        }
        case OBJ_LIST: {
            ObjList* list = (ObjList*)object;
            markObject((Obj*)list->sClass);
            markArray(&list->values);
            break;
        }
        case OBJ_DICT: {
            ObjDict* dict = (ObjDict*)object;
            markObject((Obj*)dict->sClass);
            markTable(&dict->data);
            break;
        }
        case OBJ_ENUM: {
            ObjEnum* enumObj = (ObjEnum*)object;
            markObject((Obj*)enumObj->name);
            markTable(&enumObj->values);
            break;
        }
        case OBJ_MODULE: {
            ObjModule* module = (ObjModule*)object;
            markTable(&module->methods);
            break;
        }
        case OBJ_ERROR: {
            ObjError* error = (ObjError*)object;
            markObject((Obj*)error->message);
            break;
        }
        case OBJ_FILE:
            break;
        case OBJ_NATIVE:
            break;
        case OBJ_STRING:
            break;
        default:
            break;
    }
}

/**
 * Method for sweeping objects.
 */
void sweep() {
#ifdef DEBUG_LOG_GC
    printf("--> sweeping\n");
#endif

    Obj* previous = NULL;
    Obj* object = vm.objects;
    while (object != NULL){
        if(object->type == OBJ_NATIVE) {
            // don't touch natives
            previous = object;
            object = object->next;
            continue;
        }
        if (object->mark == vm.markValue) {
            previous = object;
            object = object->next;
        } else {
            Obj* unreached = object;
            object = object->next;
            if (previous != NULL) {
                previous->next = object;
            } else {
                vm.objects = object;
            }

#ifdef DEBUG_LOG_GC
            printf("--> sweeping: ");
            printObject(OBJ_VAL(unreached));
            printf(", type: %d", unreached->type);
            printf("\n");
#endif
            freeObject(unreached);
        }
    }

#ifdef DEBUG_LOG_GC
    printf("--> finished sweeping\n");
#endif
}

/**
 * Removes all white entries from the table.
 */
void tableRemoveWhite(Table* table) {
    for (int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        if (!IS_EMPTY(entry->key) && IS_OBJ(entry->key) && AS_OBJ(entry->key)->mark != vm.markValue) {
            tableDelete(table, entry->key);
        }
    }
}
