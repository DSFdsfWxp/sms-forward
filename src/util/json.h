
#pragma once

#include <stdbool.h>
#include "api/json.h"

/**
 * @brief Check whether a JSON item has the expected type.
 * @param item  JSON node (may be NULL).
 * @param type  Expected type from json_type_e (JSON_STRING, JSON_NUMBER, etc.).
 * @return true if item is non-NULL and item->type == type.
 */
bool json_item_check_type(const json_t* item, int type);

/**
 * @brief Navigate to a nested JSON node using dot-separated path.
 *        Example: "push.wxpusher.mode" walks root->"push"->"wxpusher"->"mode".
 * @param root  Root JSON object.
 * @param path  Dot-separated path string.
 * @return Pointer to the target node, or NULL on missing path.
 */
json_t* json_get_by_path(json_t* root, const char* path);

/**
 * @brief Extract a json string array into a NULL-terminated C array.
 *        Only items of type JSON_STRING are included; non-string items
 *        are silently skipped.
 * @param arr  JSON array node.
 * @return malloc'd NULL-terminated char** array, or NULL on empty/invalid input.
 *         Each string is strdup'd; caller must free all strings and the array.
 */
char** json_load_str_array(json_t* arr);

/**
 * @brief Extract a json number array into a length-prefixed int array.
 *        Only items of type JSON_NUMBER are included.
 * @param arr  JSON array node.
 * @return malloc'd int array where [0]=count, [1..count]=values,
 *         or NULL on empty/invalid input. Caller must free with free().
 */
int* json_load_int_array(json_t* arr);
