
#include <stddef.h>
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
