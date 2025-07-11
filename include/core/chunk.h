/**
 * @file chunk.h
 */

#ifndef cslo_chunk_h
#define cslo_chunk_h

#include "core/common.h"
#include "core/value.h"

/** @enum OpCode
*  This defines all of the valid OpCode for our program.
*
*/
typedef enum OpCode {
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_DEFINE_GLOBAL,
    OP_DEFINE_FINAL_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_CLOSE_UPVALUE,
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_GREATER,
    OP_GREATER_EQUAL,
    OP_LESS,
    OP_LESS_EQUAL,
    OP_NEGATE,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_MODULO,
    OP_POW,
    OP_NOT,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_JUMP_IF_TRUE,
    OP_LOOP,
    OP_CALL,
    OP_INVOKE,
    OP_SUPER_INVOKE,
    OP_CLOSURE,
    OP_RETURN,
    OP_CLASS,
    OP_METHOD,
    OP_INHERIT,
    OP_GET_SUPER,
    OP_SET_PROPERTY,
    OP_GET_PROPERTY,
    OP_DUP,
    OP_DUP2,
    OP_LIST,
    OP_GET_INDEX,
    OP_SET_INDEX,
    OP_SLICE,
    OP_HAS,
    OP_HAS_NOT,
    OP_LEN,
    OP_DICT,
    OP_ENUM,
    OP_IMPORT,
    OP_IMPORT_AS,
    OP_INTERPOLATE,
    OP_ASSERT,
} OpCode;

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
