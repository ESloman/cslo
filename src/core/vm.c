/**
 * @file vm.c
 */

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "builtins/file_methods.h"
#include "builtins/type_methods.h"

#include "core/common.h"
#include "compiler/compiler.h"
#include "core/debug.h"
#include "core/errors.h"
#include "core/object.h"
#include "core/loader.h"
#include "core/memory.h"
#include "core/natives.h"
#include "core/vm.h"

#include "objects/collection_methods.h"
#include "objects/dict_methods.h"
#include "objects/file_methods.h"
#include "objects/list_methods.h"
#include "objects/string_methods.h"

VM vm;

/**
 * Method for resetting the stack.
 */
static void resetStack() {
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
    vm.openUpvalues = NULL;
}

/**
 * Method for reporting a runtime error.
 */
static void runtimeError(enum ErrorType errorType, const char* format, ...) {
    char message[512];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    // Get current frame for line info
    int line = -1;
    int column = -1;
    const char* file = "<script>";
    if (vm.frameCount > 0) {
        CallFrame* frame = &vm.frames[vm.frameCount - 1];
        ObjFunction* function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        line = getLine(function->chunk, instruction);
        column = getColumn(function->chunk, instruction);
        if (function->file && function->file->chars) {
            file = function->file->chars;
        }
    }

    char stacktrace[1024] = {0};
    size_t offset = 0;
    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->closure->function;
        int line = getLine(function->chunk, frame->ip - function->chunk.code - 1);
        int column = getColumn(function->chunk, frame->ip - function->chunk.code - 1);
        const char* funcName = function->name ? function->name->chars : "<script>";
        offset += snprintf(stacktrace + offset, sizeof(stacktrace) - offset,
                        "  at %s (%s:%d:%d)\n", funcName, "<script>", line, column);
    }

    Exception exc = {
        .type = errorType,
        .message = message,
        .line = line,
        .column = column,
        .file = file,
        .stacktrace = stacktrace
    };
    reportError(&exc);

    resetStack();
}

/**
 * Implementation of method to initialise the virtual machine.
 */
void initVM() {
    srand(time(NULL));
    resetStack();
    vm.objects = NULL;
    vm.bytesAllocated = 0;
    vm.nextGC = 1024 * 1024;
    vm.grayCount = 0;
    vm.grayCapacity = 0;
    vm.grayStack = NULL;
    vm.markValue = true;
    initTable(&vm.globals);
    initTable(&vm.strings);
    vm.initString = NULL;
    vm.initString = copyString("__init__", 8);

    registerBuiltInFileMethods(&vm.globals);
    registerBuiltInTypeMethods(&vm.globals);

    ObjString* containerName = copyString("container", 8);
    vm.containerClass = newClass(containerName, NULL);
    registerContainerMethods(vm.containerClass);

    // Create the list class and its methods.
    ObjString* listName = copyString("list", 4);
    vm.listClass = newClass(listName, vm.containerClass);
    registerListMethods(vm.listClass);

    // Create the dict class and its methods.
    ObjString* dictName = copyString("dict", 4);
    vm.dictClass = newClass(dictName, vm.containerClass);
    registerDictMethods(vm.dictClass);

    ObjString* stringName = copyString("string", 6);
    vm.stringClass = newClass(stringName, NULL);
    registerStringMethods(vm.stringClass);

    ObjString* fileName = copyString("fileCls", 7);
    vm.fileClass = newClass(fileName, NULL);
    registerFileMethods(vm.fileClass);

    defineNatives();
}

/**
 * Implementation of method to free the virtual machine.
 */
void freeVM() {
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    vm.initString = NULL;
    freeObjects();
}

/**
 * Pushes the given value onto the stack.
 *
 * Stores the value in the top unused value of the array.
 * stackTop is a pointer to that position.
 * Increments the pointer to point to the next unused element in the array.
 */
void push(Value value) {
    #ifdef DEBUG_LOGGING
    printf("Before push: stackTop=%ld\n", vm.stackTop - vm.stack);
    for (Value* v = vm.stack; v < vm.stackTop; v++) {
        printf("  [%ld] ", v - vm.stack);
        printValue(*v);
        printf("\n");
    }
    printf("Pushing to stack[%ld]: ", vm.stackTop - vm.stack);
    printValue(value);
    printf("\n");
    #endif
    *vm.stackTop = value;
    vm.stackTop++;
    #ifdef DEBUG_LOGGING
    printf("After push: stackTop=%ld\n", vm.stackTop - vm.stack);
    for (Value* v = vm.stack; v < vm.stackTop; v++) {
        printf("  [%ld] ", v - vm.stack);
        printValue(*v);
        printf("\n");
    }
    #endif
}

/**
 * Grabs the value on top of the stack.
 *
 * Decremenents the top of the stack pointer to the last used element in the array.
 * Returns the value stored at that position.
 */
Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

/**
 * Method for peeking at the stack.
 */
Value peek(int distance) {
  return vm.stackTop[-1 - distance];
}

/**
 * Method for actually executing a call.
 */
static bool call(ObjClosure* closure, int argCount) {
    if(argCount != closure->function->arity) {
        runtimeError(ERROR_TYPE, "Expected %d arguments but got %d.", closure->function->arity, argCount);
        return false;
    }

    if (vm.frameCount == FRAMES_MAX) {
        runtimeError(ERROR_RUNTIME, "Stack overflow.");
        return false;
    }

    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = vm.stackTop - argCount - 1;
    return true;
}

/**
 * Method for executing a call.
 */
static bool callValue(Value callee, int argCount, uint8_t* _ip) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_BOUND_METHOD: {
                ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
                vm.stackTop[-argCount - 1] = bound->receiver;
                return call(bound->method, argCount);
            }
            case OBJ_CLASS: {
                ObjClass* sClass = AS_CLASS(callee);
                vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(sClass));
                Value initialiser;
                if (tableGet(&sClass->methods, OBJ_VAL(vm.initString), &initialiser)) {
                    return call(AS_CLOSURE(initialiser), argCount);
                } else if (argCount != 0) {
                    CallFrame* frame = &vm.frames[vm.frameCount - 1];
                    frame->ip = _ip;
                    runtimeError(ERROR_TYPE, "Expected 0 arguments but got %d.", argCount);
                    return false;
                }
                return true;
            }
            case OBJ_CLOSURE: {
                return call(AS_CLOSURE(callee), argCount);
            }
            case OBJ_NATIVE: {
                NativeFn native = AS_NATIVE(callee);
                #ifdef DEBUG_LOGGING
                printf("Stack before native call: ");
                for (int i = 0; i < argCount + 1; i++) {
                    printValue(vm.stackTop[-argCount - 1 + i]);
                    printf(" ");
                }
                printf("\n");
                #endif
                Value result = native(argCount, vm.stackTop - argCount);
                if (IS_ERROR(result)) {
                    // todo: handle native errors
                    runtimeError(ERROR_RUNTIME, "Native function returned an error.");
                    return false;
                }
                vm.stackTop -= argCount + 1;
                push(result);
                return true;
            }
            default:
                break;
        }
    }
    runtimeError(ERROR_TYPE, "Can only call functions and classes.");
    return false;
}

/**
 * Method for invoking a method directly from an instance.
 */
static bool invokeFromClass(ObjClass* sClass, ObjString* name, int argCount) {
    Value method;
    if (!tableGet(&sClass->methods, OBJ_VAL(name), &method)) {
        runtimeError(ERROR_ATTRIBUTE, "Undefined property '%s'.", name->chars);
        return false;
    }

    return call(AS_CLOSURE(method), argCount);
}

/**
 * Method for invoking a method on a container.
 */
static Value getContainerMethod(Value receiver, ObjString* name) {
    // check parent container first
    Value method;
    #ifdef DEBUG_LOGGING
    printf("All container methods:\n");
    for (int i = 0; i < vm.containerClass->methods.count; i++) {
        Entry* entry = &vm.containerClass->methods.entries[i];
        if (!IS_EMPTY(entry->key)) {
            printf("  %s\n", AS_STRING(entry->key)->chars);
        }
    }
    #endif
    if (tableGet(&vm.containerClass->methods, OBJ_VAL(name), &method)) {
        return method;
    }
    Table* methods;
    if (IS_DICT(receiver)) {
        methods = &AS_DICT(receiver)->sClass->methods;
    } else if (IS_LIST(receiver)) {
        methods = &AS_LIST(receiver)->sClass->methods;
    } else {
        runtimeError(ERROR_TYPE, "Only containers have methods.");
        return NIL_VAL;
    }
    #ifdef DEBUG_LOGGING
    printf("All methods:\n");
    for (int i = 0; i < methods->count; i++) {
        Entry* entry = &methods->entries[i];
        if (!IS_EMPTY(entry->key)) {
            printf("  %s\n", AS_STRING(entry->key)->chars);
        }
    }
    #endif
    return tableGet(methods, OBJ_VAL(name), &method) ? method : NIL_VAL;

}

/**
 * Method for invoking a method.
 */
static bool invoke(ObjString* name, int argCount, uint8_t* ip) {
    Value receiver = peek(argCount);

    if (IS_INSTANCE(receiver)) {
        // implementation if receiver is an instance of a class
        ObjInstance* instance = AS_INSTANCE(receiver);
        Value value;
        if (tableGet(&instance->fields, OBJ_VAL(name), &value)) {
            vm.stackTop[-argCount - 1] = value;
            return callValue(value, argCount, ip);
        }
        return invokeFromClass(instance->sClass, name, argCount);
    } else if (IS_STRING(receiver)) {
        Value method;
        if (tableGet(&vm.stringClass->methods, OBJ_VAL(name), &method)) {
            if (IS_NATIVE(method)) {
                NativeFn native = AS_NATIVE(method);
                Value result = native(argCount + 1, vm.stackTop - argCount - 1);
                if (IS_ERROR(result)) {
                    // todo: add native error handling
                    runtimeError(ERROR_RUNTIME, "Native function returned an error.");
                    return false;
                }
                vm.stackTop -= argCount + 1;
                push(result);
                return true;
            } else if (IS_CLOSURE(method)) {
                // If you support closures/methods on containers, handle here
                return call(AS_CLOSURE(method), argCount);
            } else {
                runtimeError(ERROR_TYPE, "Method '%s' is not callable.", name->chars);
                return false;
            }
        } else {
            runtimeError(ERROR_ATTRIBUTE, "Undefined method '%s' for string.", name->chars);
            return false;
        }
    } else if (IS_CONTAINER(receiver)) {
        Value method = getContainerMethod(receiver, name);
        if (IS_NIL(method)) {
            runtimeError(ERROR_ATTRIBUTE, "Undefined method '%s'.", name->chars);
            return false;
        }
        // For native methods, pass the container as the first argument
        if (IS_NATIVE(method)) {
            NativeFn native = AS_NATIVE(method);
            Value result = native(argCount + 1, vm.stackTop - argCount - 1);
            if (IS_ERROR(result)) {
                runtimeError(ERROR_RUNTIME, "Native function returned an error.");
                return false;
            }
            vm.stackTop -= argCount + 1;
            push(result);
            return true;
        } else if (IS_CLOSURE(method)) {
            // If you support closures/methods on containers, handle here
            return call(AS_CLOSURE(method), argCount);
        } else {
            runtimeError(ERROR_TYPE, "Method '%s' is not callable.", name->chars);
            return false;
        }
    } else if (IS_MODULE(receiver)) {
        ObjModule* module = AS_MODULE(receiver);
        Value method;
        if (tableGet(&module->methods, OBJ_VAL(name), &method)) {
            vm.stackTop[-argCount - 1] = method;
            return callValue(method, argCount, ip);
        } else {
            runtimeError(ERROR_ATTRIBUTE, "Undefined method '%s' in module.", name->chars);
            return false;
        }
    } else if (IS_FILE(receiver)) {
        Value method;
        if (tableGet(&vm.fileClass->methods, OBJ_VAL(name), &method)) {
            if (IS_NATIVE(method)) {
                NativeFn native = AS_NATIVE(method);
                Value result = native(argCount + 1, vm.stackTop - argCount - 1);
                if (IS_ERROR(result)) {
                    // todo: add native error handling
                    runtimeError(ERROR_RUNTIME, "Native function returned an error.");
                    return false;
                }
                vm.stackTop -= argCount + 1;
                push(result);
                return true;
            } else if (IS_CLOSURE(method)) {
                // If you support closures/methods on files, handle here
                return call(AS_CLOSURE(method), argCount);
            } else {
                runtimeError(ERROR_TYPE, "Method '%s' is not callable.", name->chars);
                return false;
            }
        } else {
            runtimeError(ERROR_ATTRIBUTE, "Undefined method '%s' for file.", name->chars);
            return false;
        }
    } else {
        runtimeError(ERROR_TYPE, "Only instances and lists have methods.");
        return false;
    }
}

/**
 * Method for binding a method.
 */
static bool bindMethod(ObjClass* sClass, ObjString* name) {
    Value method;
    if (!tableGet(&sClass->methods, OBJ_VAL(name), &method)) {
        return false;
    }

    ObjBoundMethod* bound = newBoundMethod(peek(0), AS_CLOSURE(method));
    pop();
    push(OBJ_VAL(bound));
    return true;
}

/**
 * Method for capturing an upvalue.
 */
static ObjUpvalue* captureUpvalue(Value* local) {
    ObjUpvalue* prevUpvalue = NULL;
    ObjUpvalue* upvalue = vm.openUpvalues;
    while (upvalue != NULL && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue* createdUpvalue = newUpvalue(local);
    if (prevUpvalue == NULL) {
        vm.openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }
    return createdUpvalue;
}

/**
 * Method for closing over values.
 */
static void closeUpvalues(Value* last) {
    while (vm.openUpvalues != NULL && vm.openUpvalues->location >= last) {
        ObjUpvalue* upvalue = vm.openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.openUpvalues = upvalue->next;
    }
}

/**
 * Method for defining a method.
 */
static void defineMethod(ObjString* name) {
    Value method = peek(0);
    ObjClass* sClass = AS_CLASS(peek(1));
    tableSet(&sClass->methods, OBJ_VAL(name), method);
    pop();
}

/**
 * Works out if a given value is falsey.
 *
 * A value is false if:
 *   - it is nil
 *   - it's a boolean 'false'
 *   - it's a number that's 0
 */
static bool isFalsey(Value value) {
    if (IS_BOOL(value)) {
        return !AS_BOOL(value);
    } else if (IS_NIL(value)) {
        return true;
    } else if (IS_NUMBER(value)) {
        double num = AS_NUMBER(value);
        return num == 0 || num == 0.0;
    } else {
        // should be unreachable
        return false;
    }
}

/**
 *  Method for concatenating two strings.
 *
 * Simply adds the lengths, allocates enough memory,
 * and copies the string arrays into the new location.
 *
 * Then it creates a new ObjString using the new string.
 */
static void concatenate() {
    ObjString* b = AS_STRING(peek(0));
    ObjString* a = AS_STRING(peek(1));

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length);
    pop();
    pop();
    push(OBJ_VAL(result));
}

/**
 * Method for actually running byte code.
 */
static InterpretResult run() {
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
    register uint8_t* ip = frame->ip;

#define READ_BYTE() (*ip++)

#define READ_CONSTANT() \
    (frame->closure->function->chunk.constants.values[READ_BYTE()])

#define READ_SHORT() \
    (ip += 2, \
    (uint16_t)((ip[-2] << 8) | ip[-1]))

#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueType, op) \
    do { \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            frame->ip = ip; \
            runtimeError(ERROR_TYPE, "Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        double b = AS_NUMBER(pop()); \
        double a = AS_NUMBER(pop()); \
        push(valueType(a op b)); \
    } while (false)


    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
        printf("[ ");
        printValue(*slot);
        printf(" ]");
    }
    printf("  <-- stack size: %ld\n", (long)(vm.stackTop - vm.stack));
    disassembleInstruction(&frame->closure->function->chunk, (int)(frame->ip - frame->closure->function->chunk.code));
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_NIL: {
                push(NIL_VAL);
                break;
            }
            case OP_TRUE: {
                push(BOOL_VAL(true));
                break;
            }
            case OP_FALSE: {
                push(BOOL_VAL(false));
                break;
            }
            case OP_POP: {
                pop();
                #ifdef DEBUG_LOGGING
                printf("DEBUG: Stack after OP_POP: ");
                for (int i = 0; i < vm.stackTop - vm.stack; i++) {
                    printValue(vm.stack[i]);
                    printf(" ");
                }
                printf("\n");
                #endif
                break;
            }
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                #ifdef DEBUG_LOGGING
                printf("OP_GET_LOCAL: slot: %d, value: ", slot);
                printValue(frame->slots[slot]);
                printf("\n");
                printf("DEBUG: Stack before OP_GET_LOCAL: ");
                for (int i = 0; i < vm.stackTop - vm.stack; i++) {
                    printValue(vm.stack[i]);
                    printf(" ");
                }
                printf("\n");
                printf("All slots: ");
                for (Value* slot = frame->slots; slot < vm.stackTop; slot++) {
                    printValue(*slot);
                    printf(" ");
                }
                printf("\n");
                #endif
                push(frame->slots[slot]);
                #ifdef DEBUG_LOGGING
                printf("DEBUG: Stack after OP_GET_LOCAL: ");
                for (int i = 0; i < vm.stackTop - vm.stack; i++) {
                    printValue(vm.stack[i]);
                    printf(" ");
                }
                printf("\n");
                printf("All slots: ");
                for (Value* slot = frame->slots; slot < vm.stackTop; slot++) {
                    printValue(*slot);
                    printf(" ");
                }
                printf("\n");
                #endif
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                #ifdef DEBUG_LOGGING
                printf("OP_SET_LOCAL: slot: %d, value: ", slot);
                printValue(peek(0));
                printf("\n");
                printf("DEBUG: Stack before OP_SET_LOCAL: ");
                for (int i = 0; i < vm.stackTop - vm.stack; i++) {
                    printValue(vm.stack[i]);
                    printf(" ");
                }
                printf("\n");
                printf("All slots: ");
                for (Value* slot = frame->slots; slot < vm.stackTop; slot++) {
                    printValue(*slot);
                    printf(" ");
                }
                printf("\n");
                #endif
                frame->slots[slot] = peek(0);
                #ifdef DEBUG_LOGGING
                printf("DEBUG: Stack after OP_SET_LOCAL: ");
                for (int i = 0; i < vm.stackTop - vm.stack; i++) {
                    printValue(vm.stack[i]);
                    printf(" ");
                }
                printf("\n");
                printf("All slots: ");
                for (Value* slot = frame->slots; slot < vm.stackTop; slot++) {
                    printValue(*slot);
                    printf(" ");
                }
                printf("\n");
                #endif
                break;
            }
            case OP_GET_GLOBAL: {
                #ifdef DEBUG_LOGGING
                // Debug: Print stack and slots before the call
                printf("== Before OP_GET_GLOBAL ==\n");
                printf("StackTop: %ld\n", vm.stackTop - vm.stack);
                for (Value* v = vm.stack; v < vm.stackTop; v++) {
                    printf("  [%ld] ", v - vm.stack);
                    printValue(*v);
                    printf("\n");
                }
                printf("All slots: ");
                for (Value* slot = frame->slots; slot < vm.stackTop; slot++) {
                    printValue(*slot);
                    printf(" ");
                }
                printf("\n");
                #endif
                ObjString* name = READ_STRING();
                Value value;
                if (!tableGet(&vm.globals, OBJ_VAL(name), &value)) {
                    frame->ip = ip;
                    runtimeError(ERROR_NAME, "Undefined variable '%s'", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                #ifdef DEBUG_LOGGING
                // Debug: Print stack and slots before the call
                printf("== After OP_GET_GLOBAL ==\n");
                printf("StackTop: %ld\n", vm.stackTop - vm.stack);
                for (Value* v = vm.stack; v < vm.stackTop; v++) {
                    printf("  [%ld] ", v - vm.stack);
                    printValue(*v);
                    printf("\n");
                }
                printf("All slots: ");
                for (Value* slot = frame->slots; slot < vm.stackTop; slot++) {
                    printValue(*slot);
                    printf(" ");
                }
                printf("\n");
                #endif
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString* name = READ_STRING();
                if (tableSet(&vm.globals, OBJ_VAL(name), peek(0))) {
                    tableDelete(&vm.globals, OBJ_VAL(name));
                    frame->ip = ip;
                    runtimeError(ERROR_NAME, "Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjString* name = READ_STRING();
                tableSet(&vm.globals, OBJ_VAL(name), peek(0));
                pop();
                break;
            }
            case OP_GET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                push(*frame->closure->upvalues[slot]->location);
                break;
            }
            case OP_SET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek(0);
                break;
            }
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_NOT_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(!valuesEqual(a, b)));
                break;
            }
            case OP_GREATER: {
                BINARY_OP(BOOL_VAL, >);
                break;
            }
            case OP_GREATER_EQUAL: {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                    runtimeError(ERROR_TYPE, "Operands must be numbers.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(BOOL_VAL(a >= b));
                break;
            }
            case OP_LESS: {
                #ifdef DEBUG_LOGGING
                // Debug: Print stack and slots before the call
                printf("== Before OP_LESS ==\n");
                printf("StackTop: %ld\n", vm.stackTop - vm.stack);
                for (Value* v = vm.stack; v < vm.stackTop; v++) {
                    printf("  [%ld] ", v - vm.stack);
                    printValue(*v);
                    printf("\n");
                }
                printf("All slots: ");
                for (Value* slot = frame->slots; slot < vm.stackTop; slot++) {
                    printValue(*slot);
                    printf(" ");
                }
                printf("\n");
                #endif
                BINARY_OP(BOOL_VAL, <);
                #ifdef DEBUG_LOGGING
                // Debug: Print stack and slots before the call
                printf("== After OP_LESS ==\n");
                printf("StackTop: %ld\n", vm.stackTop - vm.stack);
                for (Value* v = vm.stack; v < vm.stackTop; v++) {
                    printf("  [%ld] ", v - vm.stack);
                    printValue(*v);
                    printf("\n");
                }
                printf("All slots: ");
                for (Value* slot = frame->slots; slot < vm.stackTop; slot++) {
                    printValue(*slot);
                    printf(" ");
                }
                printf("\n");
                #endif
                break;
            }
            case OP_LESS_EQUAL: {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                    runtimeError(ERROR_TYPE, "Operands must be numbers.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(BOOL_VAL(a <= b));
                break;
            }
            case OP_ADD: {
                #ifdef DEBUG_LOGGING
                printf("DEBUG: Stack before OP_ADD: ");
                for (int i = 0; i < vm.stackTop - vm.stack; i++) {
                    printValue(vm.stack[i]);
                    printf(" ");
                }
                printf("\n");
                #endif
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else if (IS_LIST(peek(0)) && IS_LIST(peek(1))) {
                    ObjList* b = AS_LIST(pop());
                    ObjList* a = AS_LIST(pop());
                    ObjList* result = newList();

                    // Ensure enough capacity
                    while (result->values.capacity < a->count + b->count) {
                        growValueArray(&result->values);
                    }

                    // Copy elements from a
                    for (int i = 0; i < a->count; i++) {
                        result->values.values[result->count++] = a->values.values[i];
                    }
                    // Copy elements from b
                    for (int i = 0; i < b->count; i++) {
                        result->values.values[result->count++] = b->values.values[i];
                    }
                    result->values.count = result->count;

                    push(OBJ_VAL(result));
                } else {
                    frame->ip = ip;
                    if (peek(0).type != peek(1).type) {
                        runtimeError(
                            ERROR_TYPE,
                            "Mismatched types: %s and %s.",
                            valueTypeToString(peek(0)),
                            valueTypeToString(peek(1))
                        );
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    runtimeError(ERROR_TYPE, "Addition not support for %s.", valueTypeToString(peek(0)));
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT: {
                BINARY_OP(NUMBER_VAL, -);
                break;
            }
            case OP_MULTIPLY: {
                BINARY_OP(NUMBER_VAL, *);
                break;
            }
            case OP_DIVIDE: {
                BINARY_OP(NUMBER_VAL, /);
                break;
            }
            case OP_MODULO: {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                    frame->ip = ip;
                    runtimeError(ERROR_TYPE, "Operands must be numbers.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(remainder(a, b)));
                break;
            }
            case OP_POW: {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                    frame->ip = ip;
                    runtimeError(ERROR_TYPE, "Operands must be numbers.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(pow(a, b)));
                break;
            }
            case OP_NOT: {
                push(BOOL_VAL(isFalsey(pop())));
                break;
            }
            case OP_NEGATE: {
                if (!IS_NUMBER(peek(0))) {
                    frame->ip = ip;
                    runtimeError(ERROR_TYPE, "Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            }
            case OP_DUP: {
                // duplicates the top value on the stack
                Value value = peek(0);
                push(value);
                break;
            }
            case OP_DUP2: {
                // duplicates the top two values on the stack
                Value value1 = peek(0);
                Value value2 = peek(1);
                push(value2);
                push(value1);
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (isFalsey(peek(0))) {
                    ip += offset;
                }
                break;
            }
            case OP_JUMP_IF_TRUE: {
                uint16_t offset = READ_SHORT();
                if (!isFalsey(peek(0))) {
                    ip += offset;
                }
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                ip -= offset;
                break;
            }
            case OP_CALL: {
                int argCount = READ_BYTE();
                frame->ip = ip;

                #ifdef DEBUG_LOGGING
                // Debug: Print stack and slots before the call
                printf("== Before OP_CALL ==\n");
                printf("StackTop: %ld\n", vm.stackTop - vm.stack);
                for (Value* v = vm.stack; v < vm.stackTop; v++) {
                    printf("  [%ld] ", v - vm.stack);
                    printValue(*v);
                    printf("\n");
                }
                printf("All slots: ");
                for (Value* slot = frame->slots; slot < vm.stackTop; slot++) {
                    printValue(*slot);
                    printf(" ");
                }
                printf("\n");
                #endif
                if (!callValue(peek(argCount), argCount, ip)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                #ifdef DEBUG_LOGGING
                // Debug: Print stack and slots after the call
                printf("== After OP_CALL ==\n");
                printf("StackTop: %ld\n", vm.stackTop - vm.stack);
                for (Value* v = vm.stack; v < vm.stackTop; v++) {
                    printf("  [%ld] ", v - vm.stack);
                    printValue(*v);
                    printf("\n");
                }
                printf("All slots: ");
                for (Value* slot = frame->slots; slot < vm.stackTop; slot++) {
                    printValue(*slot);
                    printf(" ");
                }
                printf("\n");
                #endif
                frame = &vm.frames[vm.frameCount - 1];
                ip = frame->ip;
                break;
            }
            case OP_CLOSURE: {
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure* closure = newClosure(function);
                push(OBJ_VAL(closure));
                for (int i = 0; i < closure->upvalueCount; i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (isLocal) {
                        closure->upvalues[i] = captureUpvalue(frame->slots + index);
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                break;
            }
            case OP_CLOSE_UPVALUE: {
                closeUpvalues(vm.stackTop - 1);
                pop();
                break;
            }
            case OP_CLASS: {
                push(OBJ_VAL(newClass(READ_STRING(), NULL)));
                break;
            }
            case OP_GET_PROPERTY: {
                if (!IS_INSTANCE(peek(0)) && !IS_ENUM(peek(0))) {
                    frame->ip = ip;
                    runtimeError(ERROR_ATTRIBUTE, "Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (IS_INSTANCE(peek(0))) {
                    ObjInstance* instance = AS_INSTANCE(peek(0));
                    ObjString* name = READ_STRING();
                    Value value;
                    if (tableGet(&instance->fields, OBJ_VAL(name), &value)) {
                        pop();
                        push(value);
                        break;
                    }

                    if (!bindMethod(instance->sClass, name)) {
                        frame->ip = ip;
                        runtimeError(ERROR_ATTRIBUTE, "Undefined property '%s'.", name->chars);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                } else if (IS_ENUM(peek(0))) {
                    ObjEnum* sEnum = AS_ENUM(peek(0));
                    ObjString* name = READ_STRING();
                    Value value;
                    if (tableGet(&sEnum->values, OBJ_VAL(name), &value)) {
                        pop();
                        push(value);
                        break;
                    }

                    frame->ip = ip;
                    runtimeError(ERROR_ATTRIBUTE, "Undefined property '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SET_PROPERTY: {
                if (!IS_INSTANCE(peek(1))) {
                    frame->ip = ip;
                    runtimeError(ERROR_ATTRIBUTE, "Only instances have fields.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjInstance* instance = AS_INSTANCE(peek(1));
                tableSet(&instance->fields, OBJ_VAL(READ_STRING()), peek(0));
                Value value = pop();
                pop();
                push(value);
                break;
            }
            case OP_INHERIT: {
                Value super = peek(1);
                if (!IS_CLASS(super)) {
                    frame->ip = ip;
                    runtimeError(ERROR_TYPE, "Superclass must be a class.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjClass* subClass = AS_CLASS(peek(0));
                tableAddAll(&AS_CLASS(super)->methods, &subClass->methods);
                pop(); // pops the subclass
                break;
            }
            case OP_GET_SUPER: {
                ObjString* name = READ_STRING();
                ObjClass* superclass = AS_CLASS(pop());

                if (!bindMethod(superclass, name)) {
                    frame->ip = ip;
                    return INTERPRET_RUNTIME_ERROR;
                }

                break;
            }
            case OP_METHOD: {
                defineMethod(READ_STRING());
                break;
            }
            case OP_INVOKE: {
                ObjString* method = READ_STRING();
                int argCount = READ_BYTE();
                frame->ip = ip;
                if (!invoke(method, argCount, ip)) {
                    frame->ip = ip;
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                ip = frame->ip;
                break;
            }
            case OP_SUPER_INVOKE: {
                ObjString* method = READ_STRING();
                int argCount = READ_BYTE();
                ObjClass* superclass = AS_CLASS(pop());
                frame->ip = ip;
                if (!invokeFromClass(superclass, method, argCount)) {
                    frame->ip = ip;
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                ip = frame->ip;
                break;
            }
            case OP_LIST: {
                int count = READ_SHORT();
                ObjList* list = newList();
                // Pop values into a temporary array
                Value* temp = ALLOCATE(Value, count);
                for (int i = count - 1; i >= 0; i--) {
                    temp[i] = pop();
                }
                while (list->values.capacity < count) {
                    growValueArray(&list->values);
                }
                for (int i = 0; i < count; i++) {
                    list->values.values[i] = temp[i];
                    list->count++;
                }
                list->values.count = list->count;
                FREE_ARRAY(Value, temp, count);
                push(OBJ_VAL(list));
                break;
            }
            case OP_GET_INDEX: {
                Value index = pop();
                Value indexable = pop();
                #ifdef DEBUG_LOGGING
                printf("DEBUG: OP_GET_INDEX types: index=%d, value=%d\n", index.type, indexable.type);
                #endif
                if (IS_LIST(indexable)) {
                    ObjList* list = AS_LIST(indexable);
                    if (!IS_NUMBER(index)) {
                        frame->ip = ip;
                        runtimeError(ERROR_TYPE, "Index must be a number.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    int idx = (int)AS_NUMBER(index);
                    if (idx < 0) {
                        idx += list->count; // allow negative indexing
                    }
                    if (idx < 0 || idx >= list->count) {
                        frame->ip = ip;
                        runtimeError(ERROR_INDEX, "Index out of bounds.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    push(list->values.values[idx]);
                } else if (IS_DICT(indexable)) {
                    ObjDict* dict = AS_DICT(indexable);
                    Value value;
                    if (tableGet(&dict->data, index, &value)) {
                        push(value);
                    } else {
                        frame->ip = ip;
                        runtimeError(ERROR_INDEX, "Key not found in dictionary.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                } else {
                    frame->ip = ip;
                    runtimeError(ERROR_TYPE, "Expected a list.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SET_INDEX: {
                Value value = pop();
                Value index = pop();
                Value indexable = pop();
                #ifdef DEBUG_LOGGING
                printf("DEBUG: OP_SET_INDEX types: list=%d, index=%d, value=%d\n", indexable.type, index.type, value.type);
                #endif
                if (IS_LIST(indexable)) {
                    ObjList* list = AS_LIST(indexable);
                    if (!IS_NUMBER(index)) {
                        frame->ip = ip;
                        runtimeError(ERROR_TYPE, "Index must be a number.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    int idx = (int)AS_NUMBER(index);
                    if (idx < 0) {
                        idx += list->count; // allow negative indexing
                    }
                    if (idx < 0 || idx >= list->count) {
                        frame->ip = ip;
                        runtimeError(ERROR_INDEX, "Index out of bounds.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    list->values.values[idx] = value;
                    push(value);
                } else if (IS_DICT(indexable)) {
                    ObjDict* dict = AS_DICT(indexable);
                    tableSet(&dict->data, index, value);
                    push(value);
                } else {
                    frame->ip = ip;
                    runtimeError(ERROR_TYPE, "Expected a list or dictionary.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SLICE: {
                Value end = pop();
                Value start = pop();
                Value listValue = pop();
                if (!IS_LIST(listValue)) {
                    frame->ip = ip;
                    runtimeError(ERROR_TYPE, "Expected a list.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjList* list = AS_LIST(listValue);

                int iStart = IS_NIL(start) ? 0 : (int)AS_NUMBER(start);
                int iEnd = IS_NIL(end) ? list->count : (int)AS_NUMBER(end);

                // Handle negative indices
                if (iStart < 0) {
                    iStart += list->count;
                }
                if (iEnd < 0) {
                    iEnd += list->count;
                }
                if (iStart < 0) {
                    iStart = 0;
                }
                if (iEnd > list->count) {
                    iEnd = list->count;
                }
                if (iEnd < iStart) {
                    iEnd = iStart;
                }

                ObjList* result = newList();
                for (int i = iStart; i < iEnd; i++) {
                    // Copy each value
                    if (result->count + 1 > result->values.capacity) {
                        growValueArray(&result->values);
                    }
                    result->values.values[result->count++] = list->values.values[i];
                    result->values.count = result->count;
                }
                push(OBJ_VAL(result));
                break;
            }
            case OP_HAS: {
                Value value = pop();
                Value container = pop();
                if (IS_LIST(container)) {
                    ObjList* list = AS_LIST(container);
                    bool found = false;
                    for (int i = 0; i < list->count; i++) {
                        if (valuesEqual(list->values.values[i], value)) {
                            found = true;
                            break;
                        }
                    }
                    push(BOOL_VAL(found));
                } else if (IS_STRING(container)) {
                    ObjString* str = AS_STRING(container);
                    if (IS_STRING(value)) {
                        ObjString* valStr = AS_STRING(value);
                        push(BOOL_VAL(strstr(str->chars, valStr->chars) != NULL));
                    } else {
                        frame->ip = ip;
                        runtimeError(ERROR_TYPE, "Can only check for strings in strings.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                } else if (IS_DICT(container)) {
                    ObjDict* dict = AS_DICT(container);
                    Value val;
                    if (tableGet(&dict->data, value, &val)) {
                        push(BOOL_VAL(true));
                    } else {
                        push(BOOL_VAL(false));
                    }
                } else {
                    frame->ip = ip;
                    runtimeError(ERROR_TYPE, "'has' not supported for this type.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_HAS_NOT: {
                Value value = pop();
                Value container = pop();
                if (IS_LIST(container)) {
                    ObjList* list = AS_LIST(container);
                    bool found = false;
                    for (int i = 0; i < list->count; i++) {
                        if (valuesEqual(list->values.values[i], value)) {
                            found = true;
                            break;
                        }
                    }
                    push(BOOL_VAL(!found));
                } else if (IS_STRING(container)) {
                    ObjString* str = AS_STRING(container);
                    if (IS_STRING(value)) {
                        ObjString* valStr = AS_STRING(value);
                        push(BOOL_VAL(!(strstr(str->chars, valStr->chars) != NULL)));
                    } else {
                        frame->ip = ip;
                        runtimeError(ERROR_TYPE, "Can only check for strings in strings.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                } else if (IS_DICT(container)) {
                    ObjDict* dict = AS_DICT(container);
                    Value val;
                    if (tableGet(&dict->data, value, &val)) {
                        push(BOOL_VAL(false));
                    } else {
                        push(BOOL_VAL(true));
                    }
                } else {
                    frame->ip = ip;
                    runtimeError(ERROR_TYPE, "'has' not supported for this type.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_LEN: {
                #ifdef DEBUG_LOGGING
                printf("DEBUG: Value before OP_LEN: ");
                printValue(peek(0));
                printf("\n");
                #endif
                Value container = pop();
                if (IS_LIST(container)) {
                    ObjList* list = AS_LIST(container);
                    push(NUMBER_VAL((double)list->count));
                } else if (IS_STRING(container)) {
                    ObjString* str = AS_STRING(container);
                    push(NUMBER_VAL((double)str->length));
                } else if (IS_DICT(container)) {
                    ObjDict* dict = AS_DICT(container);
                    push(NUMBER_VAL((double)dict->data.count));
                } else {
                    frame->ip = ip;
                    runtimeError(ERROR_TYPE, "Length only supported for lists and strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_DICT: {
                int count = READ_SHORT();
                ObjDict* dict = newDict();
                Value* keys = ALLOCATE(Value, count);
                Value* values = ALLOCATE(Value, count);
                for (int i = count - 1; i >= 0; i--) {
                    values[i] = pop();
                    keys[i] = pop();
                }
                for (int i = 0; i < count; i++) {
                    tableSet(&dict->data, keys[i], values[i]);
                }
                FREE_ARRAY(Value, keys, count);
                FREE_ARRAY(Value, values, count);
                push(OBJ_VAL(dict));
                break;
            }
            case OP_ENUM: {
                #ifdef DEBUG_LOGGING
                printf("DEBUG: OP_ENUM - reading enum with name: ");
                #endif
                int count = READ_BYTE();
                ObjEnum* sEnum = newEnum(READ_STRING());
                Value* keys = ALLOCATE(Value, count);
                Value* values = ALLOCATE(Value, count);
                for (int i = count - 1; i >= 0; i--) {
                    values[i] = pop();
                    keys[i] = pop();
                }
                for (int i = 0; i < count; i++) {
                    tableSet(&sEnum->values, keys[i], values[i]);
                }
                push(OBJ_VAL(sEnum));
                break;
            }
            case OP_IMPORT: {
                ObjString* moduleName = READ_STRING();
                bool result = loadModule(moduleName->chars);
                if (!result) {
                    frame->ip = ip;
                    runtimeError(ERROR_IMPORT, "Failed to import module '%s'.", moduleName->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_RETURN: {
                Value result = pop();
                closeUpvalues(frame->slots);
                vm.frameCount--;

                if (vm.frameCount == 0) {
                    // this means we've finished executing
                    // the entry script - we're done!
                    pop();
                    return INTERPRET_OK;
                }

                vm.stackTop = frame->slots;
                push(result);
                frame = &vm.frames[vm.frameCount - 1];
                ip = frame->ip;
                break;
            }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_SHORT
#undef READ_STRING
#undef BINARY_OP

}

/**
 * Method for executing a string of slo code.
 *
 * Creates and initialises a chunk that the compiler method
 * will fill up with bytecode. The chunk is passed to the VM
 * and then it's executed in 'run()'.
 */
InterpretResult interpret(const char* source, const char* file) {
    ObjFunction* function = compile(source, file);
    if (function == NULL) {
        return INTERPRET_COMPILE_ERROR;
    }

    push(OBJ_VAL(function));
    ObjClosure* closure = newClosure(function);
    pop();
    push(OBJ_VAL(closure));
    call(closure, 0);

    return run();
}
