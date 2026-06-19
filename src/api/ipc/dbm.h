
#pragma once

#include <stdint.h>

#define IPC_MODULE_DBM  12

#define IPC_DBM_MSG_ID_INSERT_MSG_DB_RECORD        0x2EE1  // 12001 -> dbm_msg_insert_msg_db_record_cb
#define IPC_DBM_MSG_ID_DELETE_MSG_DB_RECORD        0x2EE2  // 12002 -> dbm_msg_delete_msg_db_record_cb
#define IPC_DBM_MSG_ID_QUERY_COMBMSG_DB_RECORD     0x2EE3  // 12003 -> dbm_msg_query_combmsg_db_record_cb
#define IPC_DBM_MSG_ID_INSERT_COMBMSG_DB_RECORD    0x2EE4  // 12004 -> dbm_msg_insert_combmsg_db_record_cb
#define IPC_DBM_MSG_ID_DELETE_COMBMSG_DB_RECORD    0x2EE5  // 12005 -> dbm_msg_delete_combmsg_db_record_cb
#define IPC_DBM_MSG_ID_GET_MSG_LIST                0x2EE6  // 12006 -> dbm_msg_get_msg_list_cb
#define IPC_DBM_MSG_ID_SET_MSG_READ                0x2EE7  // 12007 -> dbm_msg_set_msg_read_cb
#define IPC_DBM_MSG_ID_GET_MSG_COUNT               0x2EE8  // 12008 -> dbm_msg_get_msg_count_cb

typedef struct dbm_msg_count_t {
  uint32_t draft_count;
  uint32_t outbox_count;
  uint32_t inbox_count;
  uint32_t unread_count;
  uint32_t max_count;
} dbm_msg_count_t;
