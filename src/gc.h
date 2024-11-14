/**
 * @file gc.h
 */

#include "table.h"
#include "value.h"

/**
 * Method for gc processing.
 */
void collectGarbage();

/**
 * Method for marking roots.
 */
void markRoots();

/**
 * Method for marking a value.
 */
void markValue(Value slot);

/**
 * Method for marking an object.
 */
void markObject(Obj* object);

/**
 * Method for marking a table.
 */
void markTable(Table* table);

/**
 * Method for marking compiler roots.
 */
void markCompilerRoots();

/**
 * Method for marking a ValueArray.
 */
void markArray(ValueArray* array);

/**
 * Method for tracing references.
 */
void traceReferences();


/**
 * Method for blackening an object.
 */
void blackenObject(Obj* object);

/**
 * Method for sweeping unreachable objects.
 */
void sweep();

/**
 * Method for removing white objects from the table.
 */
void tableRemoveWhite(Table* table);
