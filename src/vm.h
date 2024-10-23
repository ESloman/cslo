/**
 * @file vm.h
 */

#ifndef cslo_vm_h
#define cslo_vm_h

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

/**
 * @struct VM
 */
typedef struct {
    Chunk* chunk;
    uint8_t* ip;
    Value stack[STACK_MAX];
    Value* stackTop;
} VM;

/**
 * @struct Interpretresult
 */
typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

/**
 * Method for initialising the virtual machine on startup.
 */
void initVM();

/**
 * Method for freeing the virtual machine on shutdown.
 */
void freeVM();

/**
 * Method for executing a given chunk.
 */
InterpretResult interpret(Chunk* chunk);

/**
 * Method for pushing a value onto the stack.
 */
void push(Value value);

/**
 * Method for popping a value off of the stack.
 */
Value pop();

static InterpretResult run();

#endif
