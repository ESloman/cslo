/**
 * @file debug.h
 */

#ifndef cslo_debug_h
#define cslo_debug_h

#include "chunk.h"

/**
 * Method for disassembling a given chunk.
 */
void disassembleChunk(Chunk* chunk, const char* name);

/**
 * Method for disassembling a given instruction.
 */
int disassembleInstruction(Chunk* chunk, int offset);

#endif
