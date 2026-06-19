
#pragma once

void sms_init();
int sms_ipc_handler(int msg_type_id, const void* input, int input_len,
                    void* output, int output_len);
