
#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "api/json.h"

bool setting_open(const char* path);
bool setting_reload();
bool setting_wait_reload_signal();

/** @note needed to be freed manually */
char* setting_get_str(const char* key, const char* def);
uint64_t setting_get_int(const char* key, uint64_t def);
double setting_get_float(const char* key, double def);
bool setting_get_bool(const char* key, bool def);
const json_t* setting_get_raw(const char* key);
