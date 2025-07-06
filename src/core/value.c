/**
 * @file value.c
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "core/object.h"
#include "core/memory.h"
#include "core/value.h"

/**
 * Implemention of method to initialise a value array.
 *
 * Sets values to NULL and capacity/count to 0.
 */
void initValueArray(ValueArray* array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

/**
 * Method to grow a ValueArray.
 */
void growValueArray(ValueArray* array) {
    int oldCapacity = array->capacity;
    array->capacity = GROW_CAPACITY(oldCapacity);
    array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
}

/**
 * Method to shrink a ValueArray.
 */
void shrinkValueArray(ValueArray* array) {
    int newCapacity = array->capacity / 2;
    if (newCapacity < 8) {
        newCapacity = 8;
    }
    if (newCapacity < array->count) {
        newCapacity = array->count;
    }
    if (newCapacity < array->capacity) {
        array->values = GROW_ARRAY(Value, array->values, array->capacity, newCapacity);
        array->capacity = newCapacity;
    }
}

/**
 * Implementation of method to write a value to an array.
 *
 * If the array is out of capacity, grow the array to fit the new element.
 */
void writeValueArray(ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        growValueArray(array);
    }

    array->values[array->count] = value;
    array->count++;
}

/**
 * Implementation of method to free the array.
 */
void freeValueArray(ValueArray* array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

/**
 * Implementation of method to print a value.
 */
void printValue(Value value) {
    switch (value.type) {
        case VAL_BOOL:
            printf(AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NIL:
            printf("nil");
            break;
        case VAL_NUMBER:
            printf("%g", AS_NUMBER(value));
            break;
        case VAL_OBJ:
            printObject(value);
            break;
        case VAL_EMPTY:
            printf("<empty>");
            break;
        case VAL_ERROR:
            printf("<exception>");
        default:
            return;
    }
}

/**
 * Method for comparing two values for equality.
 *
 * First checks the types - if they're not the same then easy false.
 * Otherwise, get the raw values and compare.
 */
bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) {
        return false;
    }

    switch (a.type) {
        case VAL_BOOL:
            return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:
            return true;
        case VAL_NUMBER:
            return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ:
            if (OBJ_TYPE(a) != OBJ_TYPE(b)) {
                return false;
            }
            switch (OBJ_TYPE(a)) {
                case OBJ_STRING:
                    return AS_STRING(a)->length == AS_STRING(b)->length &&
                           memcmp(AS_STRING(a)->chars, AS_STRING(b)->chars, AS_STRING(a)->length) == 0;
                case OBJ_LIST:
                    ObjList* la = AS_LIST(a);
                    ObjList* lb = AS_LIST(b);
                    if (la->count != lb->count) return false;
                    for (int i = 0; i < la->count; i++) {
                        if (!valuesEqual(la->values.values[i], lb->values.values[i])) return false;
                    }
                    return true;
                default:
                    return AS_OBJ(a) == AS_OBJ(b);
            }
        case VAL_EMPTY:
            return true;
        default:
            return false;
    }
}

/**
 * Method for comparing values.
 */
int valueCompare(const void* a, const void* b) {
    const Value* va = (const Value*)a;
    const Value* vb = (const Value*)b;

    if (IS_NUMBER(*va) && IS_NUMBER(*vb)) {
        double diff = AS_NUMBER(*va) - AS_NUMBER(*vb);
        return (diff > 0) - (diff < 0);
    } else if (IS_STRING(*va) && IS_STRING(*vb)) {
        return strcmp(AS_CSTRING(*va), AS_CSTRING(*vb));
    } else {
        // Fallback: numbers < strings < others
        if (IS_NUMBER(*va)) return -1;
        if (IS_NUMBER(*vb)) return 1;
        if (IS_STRING(*va)) return -1;
        if (IS_STRING(*vb)) return 1;
        return 0;
    }
}

/**
 * Method for hashing a double.
 *
 * Taken from LUA's implementation for hashing a double.
 */
static uint32_t hashDouble(double value) {
    union BitCast {
        double value;
        uint32_t ints[2];
    };

    union BitCast cast;
    cast.value = (value) + 1.0;
    return cast.ints[0] + cast.ints[1];
}

/**
 * Method for hashing a value.
 */
uint32_t hashValue(Value value) {
    switch (value.type) {
        case VAL_BOOL:
            return AS_BOOL(value) ? 3 : 5;
        case VAL_NIL:
            return 7;
        case VAL_NUMBER:
            return hashDouble(AS_NUMBER(value));
        case VAL_OBJ:
            Obj* obj = value.as.obj;
            switch (obj->type) {
                case OBJ_STRING:
                    return AS_STRING(value)->hash;
                case OBJ_LIST:
                case OBJ_DICT:
                    return (uint32_t)(uintptr_t)obj; // Use pointer as hash for lists and dicts
                default:
                    return (uint32_t)(uintptr_t)obj; // Use pointer as hash for other objects
            }
        case VAL_EMPTY:
            return 0;
        default:
            printf("Unknown hash type: '%d'", value.type);
            return 1;
    }
}

/**
 * Method for getting a ValueType as a string.
 */
char* valueTypeToString(Value value) {
    switch (value.type) {
        case VAL_BOOL: return "bool";
        case VAL_NIL: return "nil";
        case VAL_NUMBER: return "number";
        case VAL_OBJ: {
            switch (OBJ_TYPE(value)) {
                case OBJ_STRING: return "string";
                case OBJ_LIST: return "list";
                case OBJ_CLASS: return "class";
                case OBJ_INSTANCE: return "instance";
                case OBJ_FUNCTION: return "function";
                case OBJ_NATIVE: return "native function";
                case OBJ_DICT: return "dict";
                case OBJ_ENUM: return "enum";
                case OBJ_FILE: return "file";
                case OBJ_MODULE: return "module";
                default: return "object";
            }
        }
        case VAL_EMPTY: return "empty";
        case VAL_ERROR: return "error";
        default: return "unknown";
    }
}

/**
 * @brief Works out if a given value is falsey.
 * @param Value value
 *
 * A value is false if:
 *   - it is nil
 *   - it's a boolean 'false'
 *   - it's a number that's 0
 *   - an empty list or dict
 *   - an empty string
 */
bool isFalsey(Value value) {
    if (IS_BOOL(value)) {
        return !AS_BOOL(value);
    } else if (IS_NIL(value)) {
        return true;
    } else if (IS_NUMBER(value)) {
        double num = AS_NUMBER(value);
        return num == 0 || num == 0.0;
    } else if (IS_STRING(value)) {
        return AS_STRING(value)->length == 0;
    } else if (IS_LIST(value)) {
        return AS_LIST(value)->count == 0;
    } else if (IS_DICT(value)) {
        return AS_DICT(value)->data.count == 0;
    } else {
        // everything else is truthy by default for now
        return false;
    }
}

/**
 * @brief converts a given value to a string.
 */
Value valueToString(Value value) {
    if (IS_STRING(value)) {
        return value;
    } else if (IS_NIL(value)) {
        return OBJ_VAL(copyString("nil", 3));
    } else if (IS_BOOL(value)) {
        return OBJ_VAL(copyString(AS_BOOL(value) ? "true" : "false", AS_BOOL(value) ? 4 : 5));
    } else if (IS_NUMBER(value)) {
        char buffer[32];
        int len = snprintf(buffer, sizeof(buffer), "%.14g", AS_NUMBER(value));
        return OBJ_VAL(copyString(buffer, len));
    } else if (IS_LIST(value)) {
        ObjList* list = AS_LIST(value);
        if (list->count == 0) {
            return OBJ_VAL(copyString("[]", 2));
        }
        // Estimate buffer size: assume each element is up to 32 chars, plus commas and brackets
        int bufSize = 2 + (list->count - 1) * 2 + list->count * 32;
        char* buffer = ALLOCATE(char, bufSize);
        int offset = 0;
        buffer[offset++] = '[';
        for (int i = 0; i < list->count; i++) {
            Value itemStr = valueToString(list->values.values[i]);
            if (IS_ERROR(itemStr)) {
                FREE_ARRAY(char, buffer, bufSize);
                return itemStr;
            }
            int len = AS_STRING(itemStr)->length;
            if (offset + len + 3 > bufSize) {
                // Grow buffer if needed
                int newBufSize = bufSize * 2 + len + 3;
                char* newBuffer = ALLOCATE(char, newBufSize);
                memcpy(newBuffer, buffer, offset);
                FREE_ARRAY(char, buffer, bufSize);
                buffer = newBuffer;
                bufSize = newBufSize;
            }
            memcpy(buffer + offset, AS_STRING(itemStr)->chars, len);
            offset += len;
            if (i < list->count - 1) {
                buffer[offset++] = ',';
                buffer[offset++] = ' ';
            }
        }
        buffer[offset++] = ']';
        buffer[offset] = '\0';
        Value result = OBJ_VAL(copyString(buffer, offset));
        FREE_ARRAY(char, buffer, bufSize);
        return result;
    } else if (IS_DICT(value)) {
        ObjDict* dict = AS_DICT(value);
        Table* table = &dict->data;
        int count = 0;
        // Count actual entries (non-empty)
        for (int i = 0; i < table->capacity; i++) {
            if (table->entries[i].key.type != VAL_EMPTY &&
                table->entries[i].key.type != VAL_NIL) {
                count++;
            }
        }
        if (count == 0) {
            return OBJ_VAL(copyString("{}", 2));
        }
        int bufSize = 2 + (count - 1) * 4 + count * 64;
        char* buffer = ALLOCATE(char, bufSize);
        int offset = 0;
        buffer[offset++] = '{';
        int seen = 0;
        for (int i = 0; i < table->capacity; i++) {
            Value key = table->entries[i].key;
            Value val = table->entries[i].value;
            if (key.type == VAL_EMPTY || key.type == VAL_NIL) continue;

            Value keyStr = valueToString(key);
            Value valStr = valueToString(val);
            if (IS_ERROR(keyStr) || IS_ERROR(valStr)) {
                FREE_ARRAY(char, buffer, bufSize);
                return IS_ERROR(keyStr) ? keyStr : valStr;
            }
            int keyLen = AS_STRING(keyStr)->length;
            int valLen = AS_STRING(valStr)->length;
            if (offset + keyLen + valLen + 5 > bufSize) {
                int newBufSize = bufSize * 2 + keyLen + valLen + 5;
                char* newBuffer = ALLOCATE(char, newBufSize);
                memcpy(newBuffer, buffer, offset);
                FREE_ARRAY(char, buffer, bufSize);
                buffer = newBuffer;
                bufSize = newBufSize;
            }
            memcpy(buffer + offset, AS_STRING(keyStr)->chars, keyLen);
            offset += keyLen;
            buffer[offset++] = ':';
            buffer[offset++] = ' ';
            memcpy(buffer + offset, AS_STRING(valStr)->chars, valLen);
            offset += valLen;
            seen++;
            if (seen < count) {
                buffer[offset++] = ',';
                buffer[offset++] = ' ';
            }
        }
        buffer[offset++] = '}';
        buffer[offset] = '\0';
        Value result = OBJ_VAL(copyString(buffer, offset));
        FREE_ARRAY(char, buffer, bufSize);
        return result;
    } else {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "<%s>", valueTypeToString(value));
        return OBJ_VAL(copyString(buffer, strlen(buffer)));
    }
}
