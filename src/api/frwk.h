
#pragma once

#define FRWK_OK 0

#define FRWK_MGDB_TYPE_BOOL    1
#define FRWK_MGDB_TYPE_BYTE    3
#define FRWK_MGDB_TYPE_SHORT   6
#define FRWK_MGDB_TYPE_INT     8
#define FRWK_MGDB_TYPE_UINT    9
#define FRWK_MGDB_TYPE_LONG    10
#define FRWK_MGDB_TYPE_ULONG   11
#define FRWK_MGDB_TYPE_STRING  14
#define FRWK_MGDB_TYPE_FLOAT   16
#define FRWK_MGDB_TYPE_DOUBLE  17

typedef int (*frwk_module_exit_request_handler)(int reason);
typedef int (*frwk_ipc_msg_handler)(unsigned int from_module_id,
                                    int msg_type_id, const void* input,
                                    unsigned int input_len, void* output,
                                    unsigned int output_len,
                                    unsigned int timeout_ms);

int frwk_ipc_send_sync(unsigned int target_module_id, int msg_type_id,
                       const void* input, int input_len, void* output,
                       int output_len, unsigned int timeout_ms,
                       int* result_code);

int frwk_ipc_msg_pub(const char* msg_type_name, int msg_type_id,
                     const void* payload, signed int payload_len);

int frwk_ipc_msg_sub(const char* msg_type_name);

int frwk_ipc_service(frwk_ipc_msg_handler msg_handler);

int frwk_module_init(unsigned int module_id,
                     frwk_module_exit_request_handler exit_handler);

const char* mg_enum_to_str_apps_message_id_e_type(int msg_type_id);

int mgdb_get(const char* name, char* value, unsigned int size);
int mgdb_set(const char* name, const char* value);

int frwk_mgdb_get_key_value(char* key, int data_type, void* value,
                            unsigned int size);
int frwk_mgdb_set_key_value(char* key, int data_type, void* value);
