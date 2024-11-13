/** @file memory.h
 * 
 */

#ifndef cslo_memory_h
#define cslo_memory_h

#include "common.h"
#include "object.h"

/**
 * Macro for allocating an array with given type and count.
 */
#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

/**
 * Macro for freeing. 'rellocates' the size to 0.
 */
#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

/**
 * Macro for growing the capacity.
 * If less than 8 - return 8, otherwise double it.
 */
#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

/**
 * This macros abstracts calls to reallocate when creating/growing an array.
 * 
 * It gets the size of the type and gets the actual
 * old/new size based on type size. It'll cast the returned
 * pointer to the right type too.
 */
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))

/**
 * This macro abstracts calls to reallocate when freeing an array.
 * 
 * It calculates the actual oldSize using the size of the type
 * and calls reallocate with a newSize of 0.
 */
#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * (oldCount), 0)


/**
 * Method for dynamic memory management in cslo.
 */
void* reallocate(void* pointer, size_t oldSize, size_t newSize);

/**
 * Method for freeing an object.
 * 
 * Switches based on type and ensures we free all aspects of the Obj.
 */
void freeObject(Obj* object);

/**
 * Method for freeing objects we defined.
 */
void freeObjects();

#endif
