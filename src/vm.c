/**
 * @file vm.c
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include "vm.h"

VM vm;

/**
 * Method for resetting the stack.
 */
static void resetStack() {
    vm.stackTop = vm.stack;
}

/**
 * Method for reporting a runtime error.
 */
static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    int instruction = (int)(vm.ip - vm.chunk->code - 1);
    int line = getLine(vm.chunk, instruction);
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
}



/**
 * Implementation of method to initialise the virtual machine.
 */
void initVM() {
    resetStack();
    vm.objects = NULL;
    initTable(&vm.strings);
}

/**
 * Implementation of method to free the virtual machine.
 */
void freeVM() {
    freeTable(&vm.strings);
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
 * Method for peeking at the stack.
 */
static Value peek(int distance) {
  return vm.stackTop[-1 - distance];
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
    ObjString* b = AS_STRING(pop());
    ObjString* a = AS_STRING(pop());

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length);
    push(OBJ_VAL(result));
}

/**
 * Method for actually running byte code.
 */
static InterpretResult run() {

#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_LONG_CONSTANT() (vm.chunk->constants.values[vm.chunk->code[READ_BYTE()] | (vm.chunk->code[READ_BYTE()] << 8) | (vm.chunk->code[READ_BYTE()] << 16)])
#define BINARY_OP(valueType, op) \
    do { \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers."); \
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
                if (valuesEqual(peek(0), peek(1))) {
                    push(BOOL_VAL(true));
                    break;
                }
                BINARY_OP(BOOL_VAL, >);
                break;
            }
            case OP_LESS: {
                BINARY_OP(BOOL_VAL, <);
                break;
            }
            case OP_LESS_EQUAL: {
                if (valuesEqual(peek(0), peek(1))) {
                    push(BOOL_VAL(true));
                    break;
                }
                BINARY_OP(BOOL_VAL, <);
                break;
            }
            case OP_ADD: {
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else {
                    runtimeError("Invalid type.");
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
            case OP_NOT: {
                push(BOOL_VAL(isFalsey(pop())));
                break;
            }
            case OP_NEGATE: {
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
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
