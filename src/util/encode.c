
#include <stdio.h>
#include <string.h>

#include "encode.h"

bool encode_hex_decode(const char* hex, uint8_t* out, size_t out_len) {
  if (!hex)
    return false;

  size_t hex_len = strlen(hex);
  if (hex_len != out_len * 2)
    return false;

  for (size_t i = 0; i < out_len; i++) {
    unsigned int b;
    if (sscanf(hex + i * 2, "%2x", &b) != 1)
      return false;
    out[i] = (uint8_t)b;
  }
  return true;
}
