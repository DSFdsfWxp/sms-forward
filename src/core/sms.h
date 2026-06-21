
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct sms_record_t {
  char* contacts;
  char* phone;
  char* content;
  uint32_t timestamp;
} sms_record_t;

void sms_init();
void sms_load_config();
void sms_check_new_msg();
int sms_ipc_handler(unsigned int from_module_id, int msg_type_id,
                    const void* input, unsigned int input_len, void* output,
                    unsigned int output_len, unsigned int timeout_ms);

void sms_records_free(sms_record_t* records);
