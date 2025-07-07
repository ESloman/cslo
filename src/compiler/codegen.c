/**
 * @file codegen.c
 *
 * File handles bytecode emission and generation.
 */

#include <stdint.h>

#include "compiler/codegen.h"
#include "compiler/compiler.h"
#include "parser/parser.h"
#include "compiler/scanner.h"
#include "core/chunk.h"
#include "core/value.h"

 /**
 * Method for 'emitting' a byte.
 *
 * This writes a given bit of bytecode to the current chunk.
 *
 * This takes a line number as a parameter because it's plausible
 * that some bytes being emitted aren't necessarily always going to be
 * from 'parser.previous.line'.
 */
void emitByte(uint8_t byte, int line) {
    writeChunk(currentChunk(), byte, line);
}

/**
 * Method for emitting two bytes in sequence for convenience.
 */
void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1, parser.previous.line);
    emitByte(byte2, parser.previous.line);
}

/**
 * Method for emitting a loop instruction.
 */
void emitLoop(int loopStart) {
    emitByte(OP_LOOP, parser.previous.line);

    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX) {
        error("Loop body too large.");
    }

    emitByte((offset >> 8) & 0xff, parser.previous.line);
    emitByte(offset & 0xff, parser.previous.line);
}

/**
 * Method for emitting a jump instruction.
 */
int emitJump(uint8_t instruction) {
    int line = parser.previous.line;
    emitByte(instruction, line);
    emitByte(0xff, line);
    emitByte(0xff, line);
    return currentChunk()->count - 2;
}

/**
 * Emits an OP_RETURN to the chunk.
 */
void emitReturn() {
    if (current->type == TYPE_INITIALISER) {
        emitBytes(OP_GET_LOCAL, 0);
    } else {
        emitByte(OP_NIL, parser.previous.line);
    }
    emitByte(OP_RETURN, parser.previous.line);
}

/**
 * makeConstant
 */
uint8_t makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

/**
 * Method for writing a constant to the chunk.
 */
void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

/**
 * Method for returning the current compiling chunk.
 */
Chunk* currentChunk() {
    return &current->function->chunk;
}
