/**
 * @file debug.c
 */

#include <stdio.h>

#include "core/debug.h"
#include "core/object.h"
#include "core/value.h"

/**
 * Implementation of method to disassemble a chunk.
 *
 * Prints out the given name and then loops over each of the instructions
 * in the chunk and calls 'disassembleInstruction'.
 * We let 'disassembleInstruction' increment offset as chunks can have different sizes.
 */
void disassembleChunk(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

/**
 * Method for printing a simple instruction.
 *
 * This just prints the given name and inrements the offset.
 */
static int simpleInstruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

/**
 * Method for printing a byte instruction.
 */
static int byteInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

/**
 * Method for printing a jump instruction.
 */
static int jumpInstruction(const char* name, int sign, Chunk* chunk, int offset) {
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset] + 2;
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 * sign * jump);
    return offset + 3;
}

/**
 * Method for printing a constant instruction.
 *
 * Prints the name, the constant, and the value and then increments the offest.
 */
static int constantInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}

/**
 * Method for printing an invoke instruction.
 */
static int invokeInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    uint8_t argCount = chunk->code[offset + 2];
    printf("%-16s (%d args) %4d '", name, argCount, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 3;
}

/**
 * Disassembles an instruction in a chunk with the given offset.
 *
 * This will call the necessary method for handling that instruction.
 */
int disassembleInstruction(Chunk* chunk, int offset) {
    printf("%04d ", offset);
    int line = getLine(*chunk, offset);

    if (offset > 0 && line == getLine(*chunk, offset - 1)) {
        printf("   | ");
    } else {
        printf("%4d ", line);
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_NIL:
            return simpleInstruction("OP_NIL", offset);
        case OP_TRUE:
            return simpleInstruction("OP_TRUE", offset);
        case OP_FALSE:
            return simpleInstruction("OP_FALSE", offset);
        case OP_POP:
            return simpleInstruction("OP_POP", offset);
        case OP_GET_LOCAL:
            return byteInstruction("OP_GET_LOCAL", chunk, offset);
        case OP_SET_LOCAL:
            return byteInstruction("OP_SET_LOCAL", chunk, offset);
        case OP_GET_GLOBAL:
            return constantInstruction("OP_GET_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL:
            return constantInstruction("OP_SET_GLOBAL", chunk, offset);
        case OP_GET_UPVALUE:
            return byteInstruction("OP_GET_UPVALUE", chunk, offset);
        case OP_SET_UPVALUE:
            return byteInstruction("OP_SET_UPVALUE", chunk, offset);
        case OP_DEFINE_GLOBAL:
            return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_DEFINE_FINAL_GLOBAL:
            return constantInstruction("OP_DEFINE_FINAL_GLOBAL", chunk, offset);
        case OP_EQUAL:
            return simpleInstruction("OP_EQUAL", offset);
        case OP_NOT_EQUAL:
            return simpleInstruction("OP_NOT_EQUAL", offset);
        case OP_GREATER:
            return simpleInstruction("OP_GREATER", offset);
        case OP_GREATER_EQUAL:
            return simpleInstruction("OP_GREATER_EQUAL", offset);
        case OP_LESS:
            return simpleInstruction("OP_LESS", offset);
        case OP_LESS_EQUAL:
            return simpleInstruction("OP_LESS_EQUAL", offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);
        case OP_NOT:
            return simpleInstruction("OP_NOT", offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_MODULO:
            return simpleInstruction("OP_MODULO", offset);
        case OP_POW:
            return simpleInstruction("OP_POW", offset);
        case OP_DUP:
            return simpleInstruction("OP_DUP", offset);
        case OP_DUP2:
            return simpleInstruction("OP_DUP2", offset);
        case OP_JUMP:
            return jumpInstruction("OP_JUMP", 1, chunk, offset);
        case OP_JUMP_IF_FALSE:
            return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
        case OP_JUMP_IF_TRUE:
            return jumpInstruction("OP_JUMP_IF_TRUE", 1, chunk, offset);
        case OP_LOOP:
            return jumpInstruction("OP_LOOP", -1, chunk, offset);
        case OP_CALL:
            return byteInstruction("OP_CALL", chunk, offset);
        case OP_GET_PROPERTY:
            return constantInstruction("OP_GET_PROPERTY", chunk, offset);
        case OP_SET_PROPERTY:
            return constantInstruction("OP_SET_PROPERTY", chunk, offset);
        case OP_GET_SUPER: {
            return constantInstruction("OP_GET_SUPER", chunk, offset);
        }
        case OP_CLOSURE: {
            offset++;
            uint8_t constant = chunk->code[offset++];
            printf("%-16s %4d ", "OP_CLOSURE", constant);
            printValue(chunk->constants.values[constant]);
            printf("\n");

            ObjFunction* function = AS_FUNCTION(chunk->constants.values[constant]);
            for (int j = 0; j < function->upvalueCount; j++) {
                int isLocal = chunk->code[offset++];
                int index = chunk->code[offset++];
                printf("%04d      |                     %s %d\n", offset - 2, isLocal ? "local" : "upvalue", index);
            }
            return offset;
        }
        case OP_INVOKE: {
            return invokeInstruction("OP_INVOKE", chunk, offset);
        }
        case OP_SUPER_INVOKE: {
            return invokeInstruction("OP_SUPER_INSTRUCTION", chunk, offset);
        }
        case OP_CLOSE_UPVALUE: {
            return simpleInstruction("OP_CLOSE_UPVALUE", offset);
        }
        case OP_CLASS: {
            return constantInstruction("OP_CLASS", chunk, offset);
        }
        case OP_INHERIT: {
            return simpleInstruction("OP_INHERIT", offset);
        }
        case OP_METHOD: {
            return constantInstruction("OP_METHOD", chunk, offset);
        }
        case OP_LIST: {
            return simpleInstruction("OP_LIST", offset);
        }
        case OP_GET_INDEX: {
            return simpleInstruction("OP_GET_INDEX", offset);
        }
        case OP_SET_INDEX: {
            return simpleInstruction("OP_SET_INDEX", offset);
        }
        case OP_SLICE: {
            return simpleInstruction("OP_SLICE", offset);
        }
        case OP_HAS: {
            return simpleInstruction("OP_HAS", offset);
        }
        case OP_HAS_NOT: {
            return simpleInstruction("OP_HAS_NOT", offset);
        }
        case OP_LEN: {
            return simpleInstruction("OP_LEN", offset);
        }
        case OP_DICT: {
            return simpleInstruction("OP_DICT", offset);
        }
        case OP_ENUM: {
            return simpleInstruction("OP_ENUM", offset);
        }
        case OP_IMPORT: {
            return constantInstruction("OP_IMPORT", chunk, offset);
        }
        case OP_IMPORT_AS: {
            return constantInstruction("OP_IMPORT_AS", chunk, offset);
        }
        case OP_INTERPOLATE: {
            return simpleInstruction("OP_INTERPOLATE", offset);
        }
        case OP_ASSERT: {
            return simpleInstruction("OP_ASSERT", offset);
        }
        case OP_RETURN: {
            return simpleInstruction("OP_RETURN", offset);
        }
        default: {
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
        }
    }
}
