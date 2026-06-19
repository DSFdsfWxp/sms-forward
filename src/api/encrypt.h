
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "api/base.h"


typedef struct {
  int mode;     // 1 = CBC, 0 = ECB
  int padding;  // 4 = NoPadding, others = PKCS#7
  size_t iv_len;
  uint8_t iv[16];
  int key_type;  // 0=128bit, 1=192bit, 2=256bit
  size_t key_len;
  uint8_t key[32];
} aes_config_t;

int aes_encrypt(aes_config_t* config, const void* in_data, size_t in_len,
                mg_buffer_t* out_buf);
char* base64_encrypt(const uint8_t* data, size_t length);
