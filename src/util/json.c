
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"

bool json_item_check_type(const json_t* item, int type) {
  return item && item->type == type;
}

json_t* json_get_by_path(json_t* root, const char* path) {
  if (!root || !path)
    return NULL;

  char path_copy[256];
  strncpy(path_copy, path, sizeof(path_copy) - 1);
  path_copy[sizeof(path_copy) - 1] = '\0';

  json_t* current_node = root;
  char* token = strtok(path_copy, ".");

  while (token != NULL) {
    if (current_node->type != JSON_OBJECT) {
      return NULL;
    }

    current_node = json_get_object_item(current_node, token);
    if (!current_node) {
      return NULL;
    }

    token = strtok(NULL, ".");
  }

  return current_node;
}

char** json_load_str_array(json_t* arr) {
  if (!arr || arr->type != JSON_ARRAY)
    return NULL;

  int count = json_get_child_num(arr);
  if (count <= 0)
    return NULL;

  int valid = 0;
  for (int i = 0; i < count; i++) {
    json_t* item = json_get_array_item(arr, i);
    if (item && item->type == JSON_STRING)
      valid++;
  }
  if (valid <= 0)
    return NULL;

  char** items = calloc(valid + 1, sizeof(char*));
  if (!items)
    return NULL;

  int j = 0;
  for (int i = 0; i < count; i++) {
    json_t* item = json_get_array_item(arr, i);
    if (item && item->type == JSON_STRING) {
      items[j] = strdup(item->value_string);
      if (items[j])
        j++;
    }
  }
  items[j] = NULL;
  return items;
}

int* json_load_int_array(json_t* arr) {
  if (!arr || arr->type != JSON_ARRAY)
    return NULL;

  int count = json_get_child_num(arr);
  if (count <= 0)
    return NULL;

  int valid = 0;
  for (int i = 0; i < count; i++) {
    json_t* item = json_get_array_item(arr, i);
    if (item && item->type == JSON_NUMBER)
      valid++;
  }
  if (valid <= 0)
    return NULL;

  int* items = malloc((valid + 1) * sizeof(int));
  if (!items)
    return NULL;

  items[0] = valid;
  int j = 1;
  for (int i = 0; i < count; i++) {
    json_t* item = json_get_array_item(arr, i);
    if (item && item->type == JSON_NUMBER)
      items[j++] = (int)item->value_int;
  }
  return items;
}
