/**
 * @file codegen.h
 *
 * File handles bytecode emission and generation.
 */

#ifndef cslo_codegen_h
#define cslo_codegen_h

#include <stdint.h>

#include "core/chunk.h"
#include "core/value.h"

#endif

/**
 * @file codegen.c
 *
 * File handles bytecode emission and generation.
 */


 /**
 * Method for 'emitting' a byte.
 *
 * This writes a given bit of bytecode to the current chunk.
 *
 * This takes a line number as a parameter because it's plausible
 * that some bytes being emitted aren't necessarily always going to be
 * from 'parser.previous.line'.
 */
void emitByte(uint8_t byte, int line);

/**
 * Method for emitting two bytes in sequence for convenience.
 */
void emitBytes(uint8_t byte1, uint8_t byte2);

/**
 * Method for emitting a loop instruction.
 */
void emitLoop(int loopStart);

/**
 * Method for emitting a jump instruction.
 */
int emitJump(uint8_t instruction);

/**
 * Emits an OP_RETURN to the chunk.
 */
void emitReturn();

/**
 * makeConstant
 */
uint8_t makeConstant(Value value);

/**
 * Method for writing a constant to the chunk.
 */
void emitConstant(Value value);

/**
 * Method for returning the current compiling chunk.
 */
Chunk* currentChunk();
