/**
 * @file table.h
 */

#ifndef cslo_table_h
#define cslo_table_h

#include "common.h"
#include "value.h"

/**
 * @struct Entry
 */
typedef struct Entry {
    ObjString* key;
    Value value;
} Entry;

/**
 * @struct Table
 */
typedef struct Table {
    int count;
    int capacity;
    Entry* entries;
} Table;

/**
 * Method for initialising a table.
 */
void initTable(Table* table);

/**
 * Method for freeing a table.
 */
void freeTable(Table* table);

/**
 * Method for inserting an entry into the table.
 */
bool tableSet(Table* table, ObjString* key, Value value);

/**
 * Method for deleting an entry from a table.
 */
bool tableDelete(Table* table, ObjString* key);

/**
 * Method for copying all of one table's entries into another.
 */
void tableAddAll(Table* from, Table* to);

/**
 * Method for copying
 */
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash);

/**
 * Method for retrieving a value from a table for a given key.
 */
bool tableGet(Table* table, ObjString* key, Value* value);

#endif
