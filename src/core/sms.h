
#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct sms_record_t {
  char* phone;
  char* content;
  uint64_t timestamp;
} sms_record_t;

void sms_init();
int sms_ipc_handler(int msg_type_id, const void* input, int input_len,
                    void* output, int output_len);

void sms_records_free(sms_record_t* record);
