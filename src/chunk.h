/**
 * @file chunk.h
 */

#ifndef cslo_chunk_h
#define cslo_chunk_h

#include "common.h"
#include "value.h"



/** @enum OpCode
 *  This defines all of the valid OpCode for our program.
 *
 */
typedef enum {
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_RETURN,
} OpCode;

/**
 * @struct LineStart
 */
typedef struct {
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
typedef struct {
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
 * Method for writing a new constant value to a given chunk.
 */
void writeConstant(Chunk* chunk, Value value, int line);

int getLine(Chunk* chunk, int instruction);

#endif