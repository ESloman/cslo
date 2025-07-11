/**
 * @file chunk.h
 */

#ifndef cslo_chunk_h
#define cslo_chunk_h

#include "core/common.h"
#include "core/opcodes.h"
#include "core/value.h"

/**
* @struct LineStart
*/
typedef struct LineStart {
  int offset;
  int line;
} LineStart;

/** @struct Chunk
*  This defines a chunk of code.
*
* count is the number of elements in the array
* capacity is the maxinum number of elements in the array
* code is the code bytes
*
*/
typedef struct Chunk {
    int count;
    int capacity;
    uint8_t* code;
    ValueArray constants;
    int lineCount;
    int lineCapacity;
    LineStart* lines;
} Chunk;


/**
* Method for initialising a new chunk
*/
void initChunk(Chunk* chunk);

/**
* Method for freeing a chunk.
*/
void freeChunk(Chunk* chunk);

/**
* Method for writing a byte to a chunk.
*/
void writeChunk(Chunk* chunk, uint8_t byte, int line);

/**
* Method for adding a new constant value to a given chunk.
*/
int addConstant(Chunk* chunk, Value value);

/**
 * Method for getting the line of a given instruction.
 */
int getLine(Chunk chunk, size_t instruction);

/**
 * Method for getting the column of a given instruction.
 */
int getColumn(Chunk chunk, size_t instruction);

 #endif
