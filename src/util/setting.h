
#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "api/json.h"

/**
 * @brief Blocks SIGUSR1 on the calling thread for safe reload signalling.
 * @return true on successful reload, false on failure.
 */
bool setting_init();

/**
 * @brief Open and parse the JSON config file at the given path.
 *        
 * @param path  Absolute path to the JSON config file.
 * @return true on success, false on file open or parse failure.
 */
bool setting_open(const char* path);

/**
 * @brief Atomically reload the config file.
 *        On failure, the old config is preserved.
 * @return true on successful reload, false on failure.
 */
bool setting_reload();

/**
 * @brief Block the calling thread until a SIGUSR1 is received.
 *        Used by the config reload thread in main.
 * @return false if no config file was previously opened.
 */
bool setting_wait_reload_signal();

/**
 * @brief Get a string value from the config by dot-separated path.
 *        The path first tries exact match on the root object, then
 *        splits by '.' and traverses nested objects.
 * @param key  Config path, e.g. "push.wxpusher.appToken".
 * @param def  Default value if key is missing or not a string.
 * @return strdup'd string; caller must free().
 */
char* setting_get_str(const char* key, const char* def);

/**
 * @brief Get an integer value from the config.
 * @param key  Dot-separated config path.
 * @param def  Default value if key is missing or not a number.
 */
uint64_t setting_get_int(const char* key, uint64_t def);

/**
 * @brief Get a floating-point value from the config.
 * @param key  Dot-separated config path.
 * @param def  Default value if key is missing or not a number.
 */
double setting_get_float(const char* key, double def);

/**
 * @brief Get a boolean value from the config.
 * @param key  Dot-separated config path.
 * @param def  Default value if key is missing or not boolean.
 */
bool setting_get_bool(const char* key, bool def);

/**
 * @brief Get a raw JSON node from the config.
 * @param key  Dot-separated config path.
 * @return Pointer to the internal JSON node (do NOT free),
 *         or NULL if the path does not exist.
 */
const json_t* setting_get_raw(const char* key);
