
#include <string.h>
#include "api/encrypt.h"

#define LOG_TAG "util.encrypt"
#include "util/log.h"


char* encrypt_data(const void* in_data, size_t in_len,
                             const uint8_t* custom_key, size_t key_len,
                             const uint8_t* custom_iv, int is_cbc) {
  mg_buffer_t out_buf = {0};
  aes_config_t config;
  char* base64_result = NULL;

  memset(&out_buf, 0, sizeof(mg_buffer_t));
  if (!mg_bf_init(&out_buf, 0)) {
    LOG_E("failed to init buffer");
    return NULL;
  }

  memset(&config, 0, sizeof(aes_config_t));

  config.mode = is_cbc ? 1 : 0;  // 1 = CBC, 0 = ECB
  config.padding = 0;            // PKCS#7

  if (custom_iv != NULL && is_cbc) {
    config.iv_len = 16;
    memcpy(config.iv, custom_iv, 16);
  } else {
    config.iv_len = 0;  // filling 0
  }

  if (key_len <= 16) {
    config.key_type = 0;  // AES-128
    config.key_len = 16;
  } else if (key_len <= 24) {
    config.key_type = 1;  // AES-192
    config.key_len = 24;
  } else {
    config.key_type = 2;  // AES-256
    config.key_len = 32;
  }

  memcpy(config.key, custom_key, key_len > 32 ? 32 : key_len);

  if (aes_encrypt(&config, in_data, in_len, &out_buf)) {
    base64_result = base64_encrypt(out_buf.data, out_buf.length);
    if (!base64_result)
        LOG_E("failed to encode base64");
  } else {
    LOG_E("failed to encrypt aes");
  }

  mg_bf_free(&out_buf);

  return base64_result;
}
