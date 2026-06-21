
#pragma once

#include <stddef.h>
#include <stdint.h>

/** @note result needed to be freed by `os_mem_free` */
char* encrypt_data(const void* in_data, size_t in_len,
                   const uint8_t* custom_key, size_t key_len,
                   const uint8_t* custom_iv, int is_cbc);
