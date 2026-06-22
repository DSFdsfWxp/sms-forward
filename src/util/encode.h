
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Decode a hex string into a byte array.
 *        Input must be exactly out_len * 2 characters, each pair
 *        representing one byte in hex notation (e.g. "a0" → 0xA0).
 *        Both uppercase and lowercase hex digits are accepted.
 * @param hex     Null-terminated hex string.
 * @param out     Output buffer for decoded bytes.
 * @param out_len Number of bytes to decode (input must be 2*out_len chars).
 * @return true on success, false if hex is NULL, wrong length,
 *         or contains invalid hex characters.
 */
bool encode_hex_decode(const char* hex, uint8_t* out, size_t out_len);
