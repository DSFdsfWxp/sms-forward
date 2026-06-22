
#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Encrypt data with AES and return Base64-encoded ciphertext.
 *        AES key size is inferred from key_len: <=16 → AES-128,
 *        <=24 → AES-192, else → AES-256. PKCS#7 padding is applied.
 * @param in_data    Plaintext data.
 * @param in_len     Length of plaintext.
 * @param custom_key AES key bytes.
 * @param key_len    Key length (16/24/32 for AES-128/192/256).
 * @param custom_iv  IV for CBC mode (16 bytes). Ignored if is_cbc == 0.
 * @param is_cbc     Non-zero for AES-CBC, zero for AES-ECB.
 * @return malloc'd Base64-encoded string, or NULL on error.
 *         Must be freed with os_mem_free().
 */
char* encrypt_data(const void* in_data, size_t in_len,
                   const uint8_t* custom_key, size_t key_len,
                   const uint8_t* custom_iv, int is_cbc);
