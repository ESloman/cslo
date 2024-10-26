/**
 * @file compiler.h
 */

#ifndef cslo_compiler_h
#define cslo_compiler_h

#include "object.h"
#include "vm.h"

/**
 * Method for compiling slo code into bytecode.
 */
bool compile(const char* source, Chunk* chunk);

#endif
