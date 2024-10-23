/**
 * @file main.c
 */

#include "common.h"

#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(int argc, const char* argv[]) {
    initVM();

    Chunk chunk;
    initChunk(&chunk);

    writeConstant(&chunk, 1.2, 1);

    writeConstant(&chunk, 3.4, 2);
    writeChunk(&chunk, OP_ADD, 2);
    writeConstant(&chunk, 5.6, 2);
    writeChunk(&chunk, OP_DIVIDE, 2);

    writeChunk(&chunk, OP_NEGATE, 2);
    writeChunk(&chunk, OP_RETURN, 3);

    disassembleChunk(&chunk, "test chunk");
    interpret(&chunk);

    freeChunk(&chunk);

    freeVM();

    return 0;
}
