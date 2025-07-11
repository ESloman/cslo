/**
 * @file vm.h
 */

#ifndef cslo_vm_h
#define cslo_vm_h

#include "chunk.h"
#include "core/object.h"
#include "table.h"
#include "core/value.h"

#define FRAMES_MAX 256
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

/**
 * @struct CallFrame
 */
typedef struct CallFrame {
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
} CallFrame;

/**
 * @struct VM
 */
typedef struct VM {
    CallFrame frames[FRAMES_MAX];
    int frameCount;

    Value stack[STACK_MAX];
    Value* stackTop;
    Table globals;
    Table globalFinals;
    Table strings;
    ObjString* initString;

    ObjUpvalue* openUpvalues;

    ObjClass* containerClass;
    ObjClass* listClass;
    ObjClass* dictClass;
    ObjClass* stringClass;
    ObjClass* fileClass;

    size_t bytesAllocated;
    size_t nextGC;

    Obj* objects;

    bool markValue;
    int grayCount;
    int grayCapacity;
    Obj** grayStack;
} VM;

/**
 * @struct Interpretresult
 */
typedef enum InterpretResult {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

/**
 * Method for initialising the virtual machine on startup.
 */
void initVM();

/**
 * Method for freeing the virtual machine on shutdown.
 */
void freeVM();

/**
 * Method for executing a string of slo code.
 */
InterpretResult interpret(const char* source, const char* file);

/**
 * Method for pushing a value onto the stack.
 */
void push(Value value);

/**
 * Method for popping a value off of the stack.
 */
Value pop();

/**
 * Method for peeking at the stack.
 */
Value peek(int distance);

#endif
