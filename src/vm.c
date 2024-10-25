/**
 * @file vm.c
 */

#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "vm.h"

VM vm;

static void resetStack() {
    vm.stackTop = vm.stack;
}

/**
 * Implementation of method to initialise the virtual machine.
 */
void initVM() {
    resetStack();
}

/**
 * Implementation of method to free the virtual machine.
 */
void freeVM() {

}

/**
 * Pushes the given value onto the stack.
 * 
 * Stores the value in the top unused value of the array.
 * stackTop is a pointer to that position.
 * Increments the pointer to point to the next unused element in the array.
 */
void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
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
 * Method for actually running byte code.
 */
static InterpretResult run() {

#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_LONG_CONSTANT() (vm.chunk->constants.values[vm.chunk->code[READ_BYTE()] | (vm.chunk->code[READ_BYTE()] << 8) | (vm.chunk->code[READ_BYTE()] << 16)])
#define BINARY_OP(op) \
    do { \
        double b = pop(); \
        double a = pop(); \
        push(a op b); \
    } while (false)


    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
        printf("[ ");
        printValue(*slot);
        printf(" ]");
    }
    printf("\n");
    disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_CONSTANT_LONG: {
                Value constant = READ_LONG_CONSTANT();
                push(constant);
                break;
            }
            case OP_ADD: {
                BINARY_OP(+);
                break;
            }
            case OP_SUBTRACT: {
                BINARY_OP(-);
                break;
            }
            case OP_MULTIPLY: {
                BINARY_OP(*);
                break;
            }
            case OP_DIVIDE: {
                BINARY_OP(/);
                break;
            }
            case OP_NEGATE: {
                push(-pop());
                break;
            }
            case OP_RETURN: {
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;
            }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_LONG_CONSTANT
#undef BINARY_OP

}

/**
 * Method for executing a string of slo code.
 * 
 * Creates and initialises a chunk that the compiler method
 * will fill up with bytecode. The chunk is passed to the VM
 * and then it's executed in 'run()'.
 */
InterpretResult interpret(const char* source) {
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = run();

    freeChunk(&chunk);
    return result;
}
