/**
 * @file json.c
 * @brief Implementation of JSON handling utilities.
 */

#include <stdlib.h>
#include <string.h>

#include "builtins/util.h"
#include "core/object.h"
#include "core/value.h"
#include "std/json.h"

#include "cJSON.h"

// forward declarations of native functions
static Value loadsJsonNative(int argCount, Value* args);
static Value dumpsJsonNative(int argCount, Value* args);

/**
 * @brief Gets the json module with all its functions.
 * @return A pointer to the ObjModule containing json functions.
 */
ObjModule* getJsonModule() {
    ObjModule* module = newModule();
    defineBuiltIn(&module->methods, "loads", loadsJsonNative);
    defineBuiltIn(&module->methods, "dumps", dumpsJsonNative);
    return module;
}

/**
 * @brief Converts a cJSON object to a Value.
 * @param json The cJSON object to convert.
 * @return A Value representing the JSON object, or an error value if conversion fails.
 */
static Value cjsonToValue(cJSON* json) {
    if (cJSON_IsObject(json)) {
        Value result = OBJ_VAL(newDict());
        cJSON* child = json->child;
        while (child) {
            Value key = OBJ_VAL(copyString(child->string, strlen(child->string)));
            Value value = cjsonToValue(child);
            if (IS_ERROR(value)) return value;
            tableSet(&AS_DICT(result)->data, key, value);
            child = child->next;
        }
        return result;
    } else if (cJSON_IsArray(json)) {
        Value result = OBJ_VAL(newList());
        cJSON* child = json->child;
        while (child) {
            Value value = cjsonToValue(child);
            if (IS_ERROR(value)) return value;
            ObjList* list = AS_LIST(result);
            if (list->values.capacity <= list->values.capacity + 1) {
                growValueArray(&list->values);
            }
            // add the value to the list
            list->values.values[list->count++] = value;
            list->values.count = list->count;

            child = child->next;
        }
        return result;
    } else if (cJSON_IsString(json)) {
        return OBJ_VAL(copyString(json->valuestring, strlen(json->valuestring)));
    } else if (cJSON_IsNumber(json)) {
        return NUMBER_VAL(json->valuedouble);
    } else if (cJSON_IsBool(json)) {
        return BOOL_VAL(cJSON_IsTrue(json));
    } else if (cJSON_IsNull(json)) {
        return NIL_VAL;
    }
    return NIL_VAL;
}

static cJSON* valueToCJson(Value value) {
    if (IS_DICT(value)) {
        cJSON* json = cJSON_CreateObject();
        ObjDict* dict = AS_DICT(value);
        for (int i = 0; i < dict->data.capacity; i++) {
            Value key = dict->data.entries[i].key;
            Value val = dict->data.entries[i].value;
            if (IS_NIL(key)) {
                // skip tombstones / empties
                continue;
            }
            if (!IS_STRING(key)) {
                continue;
            }
            cJSON_AddItemToObject(json, AS_CSTRING(key), valueToCJson(val));
        }
        return json;
    } else if (IS_LIST(value)) {
        cJSON* json = cJSON_CreateArray();
        ObjList* list = AS_LIST(value);
        for (int i = 0; i < list->values.count; i++) {
            cJSON_AddItemToArray(json, valueToCJson(list->values.values[i]));
        }
        return json;
    } else if (IS_STRING(value)) {
        return cJSON_CreateString(AS_CSTRING(value));
    } else if (IS_NUMBER(value)) {
        return cJSON_CreateNumber(AS_NUMBER(value));
    } else if (IS_BOOL(value)) {
        return cJSON_CreateBool(AS_BOOL(value));
    } else if (IS_NIL(value)) {
        return cJSON_CreateNull();
    }
    return cJSON_CreateNull();
}

/**
 * @brief Loads a JSON string into a Value.
 * @param jsonString The JSON string to parse.
 * @return A Value representing the parsed JSON object or array, or an error value.
 */
static Value loadsJsonNative(int argCount, Value* args) {
    if (argCount != 1 || !IS_STRING(args[0])) {
        return ERROR_VAL_PTR("loads() expects a single string argument.");
    }

    cJSON* json = cJSON_Parse(AS_CSTRING(args[0]));
    if (!json) {
        return ERROR_VAL_PTR("Invalid JSON string.");
    }

    Value result = cjsonToValue(json);
    cJSON_Delete(json);

    return result;
}

/**
 *
 */
static Value dumpsJsonNative(int argCount, Value* args) {
    if (argCount != 1) {
        return ERROR_VAL_PTR("dumps() expects a single argument.");
    }

    cJSON* json = valueToCJson(args[0]);
    if (!json) {
        return ERROR_VAL_PTR("Invalid value for JSON serialization.");
    }

    char* jsonString = cJSON_Print(json);
    cJSON_Delete(json);
    if (!jsonString) {
        return ERROR_VAL_PTR("Failed to serialize JSON.");
    }
    Value result = OBJ_VAL(copyString(jsonString, strlen(jsonString)));
    free(jsonString);
    return result;
}
