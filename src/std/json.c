/**
 * @file json.c
 * @brief Implementation of JSON handling utilities.
 */

#include <string.h>

#include "builtins/util.h"
#include "core/object.h"
#include "core/value.h"
#include "std/json.h"

#include "cJSON.h"

// forward declarations of native functions
static Value loadsJsonNative(int argCount, Value* args);

/**
 * @brief Gets the json module with all its functions.
 * @return A pointer to the ObjModule containing json functions.
 */
ObjModule* getJsonModule() {
    ObjModule* module = newModule();
    defineBuiltIn(&module->methods, "loads", loadsJsonNative);
    return module;
}

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

static Value loadsJson(const char* jsonString) {
    cJSON* json = cJSON_Parse(jsonString);
    if (!json) {
        return ERROR_VAL_PTR("Invalid JSON string.");
    }

    Value result = cjsonToValue(json);
    cJSON_Delete(json);
    return result;
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
    return loadsJson(AS_CSTRING(args[0]));
}
