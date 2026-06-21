
#pragma once

#include <stdbool.h>
#include "api/json.h"

bool json_item_check_type(const json_t* item, int type);
json_t* json_get_by_path(json_t* root, const char* path);
