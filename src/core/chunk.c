/** @file chunk.c
 *
 */

#include <stdlib.h>

#include "core/chunk.h"
#include "core/memory.h"
#include "core/vm.h"

/**
 * Implemention of method to initialise a new chunk.
 */
void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;

    chunk->lineCount = 0;
    chunk->lineCapacity = 0;
    chunk->lines = NULL;
    initValueArray(&chunk->constants);
}

/**
 * Implementation of method to free a given chunk.
 */
void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(LineStart, chunk->lines, chunk->lineCapacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

/**
 * Implementation of a method to write a new chunk.
 *
 * If the byte will take the count over capacity,
 * we grow the Chunk so we can fit the new byte in.
 */
void writeChunk(Chunk* chunk, uint8_t byte, int line) {
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->count++;

    if (chunk->lineCount > 0 && chunk->lines[chunk->lineCount - 1].line == line) {
        return;
    }

    if (chunk->lineCapacity < chunk->lineCount + 1) {
        int oldCapacity = chunk->lineCapacity;
        chunk->lineCapacity = GROW_CAPACITY(oldCapacity);
        chunk->lines = GROW_ARRAY(LineStart, chunk->lines, oldCapacity, chunk->lineCapacity);
    }

    LineStart* lineStart = &chunk->lines[chunk->lineCount++];
    lineStart->offset = chunk->count - 1;
    lineStart->line = line;
}

/**
 * Implementation of method to write a constant value to a chunk.
 */
int addConstant(Chunk* chunk, Value value) {
    push(value);
    writeValueArray(&chunk->constants, value);
    pop();
    return chunk->constants.count - 1;
}

int getLine(Chunk chunk, size_t instruction) {
    int start = 0;
    int end = chunk.lineCount;
    int _instruction = (int) instruction;

    for (;;) {
        int mid = (start + end) / 2;
        LineStart* line = &chunk.lines[mid];
        if (_instruction < line->offset) {
            end = mid - 1;
        } else if (mid == chunk.lineCount - 1 || _instruction < chunk.lines[mid + 1].offset) {
            return line->line;
        } else {
        start = mid + 1;
        }
    }
}

int getColumn(Chunk chunk, size_t instruction) {
    int line = getLine(chunk, instruction);
    for (int i = 0; i < chunk.lineCount; i++) {
        if (chunk.lines[i].line == line) {
            return instruction - chunk.lines[i].offset;
        }
    }
    return -1; // Not found
}
