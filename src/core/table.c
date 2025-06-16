/**
 * @file table.c
 */

#include <stdlib.h>
#include <string.h>

#include "core/memory.h"
#include"core/object.h"
#include "core/table.h"
#include "core/value.h"

#define TABLE_MAX_LOAD 0.75

/**
 * Method for initialising a table.
 */
void initTable(Table* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

/**
 * Method for freeing a table.
 *
 * Frees the array and then re-initialises the table.
 */
void freeTable(Table* table) {
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}

/**
 * Method for finding an entry in the table for a given key.
 */
static Entry* findEntry(Entry* entries, int capacity, Value key) {

    uint32_t index = hashValue(key) % capacity;
    Entry* tombstone = NULL;

    for (;;) {
        Entry* entry = &entries[index];
        if (IS_EMPTY(entry->key)) {
            if (IS_NIL(entry->value)) {
                // found an empty entry
                return tombstone != NULL ? tombstone : entry;
            } else {
                // found a tombstone
                if (tombstone == NULL) {
                    tombstone = entry;
                }
            }
        } else if (valuesEqual(key, entry->key)) {
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

/**
 * Method for retrieving a value from a table for a given key.
 */
bool tableGet(Table* table, Value key, Value* value) {
    if (table->count == 0) {
        return false;
    }

    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (IS_EMPTY(entry->key) || IS_NIL(entry->key)) {
        return false;
    }

    *value = entry->value;
    return true;
}

/**
 * Method for clearing the table.
 */
void tableClear(Table* table) {
    for (int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        entry->key = EMPTY_VAL;
        entry->value = NIL_VAL;
    }
    freeTable(table);
}

/**
 * Method for adjusting the capacity of a table.
 */
static void adjustCapacity(Table* table, int capacity) {
    Entry* entries = ALLOCATE(Entry, capacity);

    // initialise everything to null/nil
    for (int i = 0; i < capacity; i++) {
        entries[i].key = EMPTY_VAL;
        entries[i].value = NIL_VAL;
    }

    // reinsert any existing entries into the new table
    table->count = 0;
    for (int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        if (IS_EMPTY(entry->key)) {
            continue;
        }

        Entry* dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    FREE_ARRAY(Entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

/**
 * Method for inserting an entry into the table.
 */
bool tableSet(Table* table, Value key, Value value) {
    // make sure to initialise the table's array and that it's big enough
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }

    Entry* entry = findEntry(table->entries, table->capacity, key);
    bool isNewKey = IS_EMPTY(entry->key);
    if (isNewKey && IS_NIL(entry->value)) {
        table->count++;
    }

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

/**
 * Method for deleting an entry from a table.
 *
 * To delete an entry, and maintain any linking, we replace the
 * entry with a 'tombstone' value. This indicates to findEntry that there
 * used to be value there - so when searching for a key we keep going if
 * we encounter a tombstone.
 */
bool tableDelete(Table* table, Value key) {
    if (table->count == 0) {
        return false;
    }

    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (IS_EMPTY(entry->key)) {
        return false;
    }

    // insert a tombstone
    entry->key = EMPTY_VAL;
    entry->value = BOOL_VAL(true);
    return true;
}

/**
 * Method for copying all the entries of one table to another.
 */
void tableAddAll(Table* from, Table* to) {
    for (int i = 0; i < from->capacity; i++) {
        Entry* entry = &from->entries[i];
        if (!IS_EMPTY(entry->key)) {
            tableSet(to, entry->key, entry->value);
        }
    }
}

/**
 * Method for finding a particular string in the table.
 */
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash) {
    if (table->count == 0) {
        return NULL;
    }
    uint32_t index = hash % table->capacity;
    for (;;) {
        Entry* entry = &table->entries[index];
        if (IS_EMPTY(entry->key)) {
            return NULL;
        }

        ObjString* string = AS_STRING(entry->key);
        if (string->length == length && memcmp(string->chars, chars, length) == 0) {
            return string;
        }

        index = (index + 1) % table->capacity;
    }
}
