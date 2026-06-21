
#pragma once

#include <stdint.h>

#define IPC_MODULE_DBM  12

#define IPC_DBM_MSG_ID_INSERT_MSG_DB_RECORD           0x2EE1  // 12001 -> dbm_msg_insert_msg_db_record_cb
#define IPC_DBM_MSG_ID_DELETE_MSG_DB_RECORD           0x2EE2  // 12002 -> dbm_msg_delete_msg_db_record_cb
#define IPC_DBM_MSG_ID_QUERY_COMBMSG_DB_RECORD        0x2EE3  // 12003 -> dbm_msg_query_combmsg_db_record_cb
#define IPC_DBM_MSG_ID_INSERT_COMBMSG_DB_RECORD       0x2EE4  // 12004 -> dbm_msg_insert_combmsg_db_record_cb
#define IPC_DBM_MSG_ID_DELETE_COMBMSG_DB_RECORD       0x2EE5  // 12005 -> dbm_msg_delete_combmsg_db_record_cb
#define IPC_DBM_MSG_ID_GET_MSG_LIST                   0x2EE6  // 12006 -> dbm_msg_get_msg_list_cb
#define IPC_DBM_MSG_ID_SET_MSG_READ                   0x2EE7  // 12007 -> dbm_msg_set_msg_read_cb
#define IPC_DBM_MSG_ID_GET_MSG_COUNT                  0x2EE8  // 12008 -> dbm_msg_get_msg_count_cb
#define IPC_DBM_MSG_ID_UPDATE_MAX_COUNT               0x2EE9  // 12009 -> dbm_msg_update_max_count_cb

#define IPC_DBM_MSG_ID_SESSION_COUNT                  0x2EEA  // 12010 -> dbm_msg_session_count_cb
#define IPC_DBM_MSG_ID_SESSION_MESSAGE_COUNT          0x2EEB  // 12011 -> dbm_msg_session_message_count_cb
#define IPC_DBM_MSG_ID_SESSION_SCAN                   0x2EEC  // 12012 -> dbm_msg_session_scan_cb
#define IPC_DBM_MSG_ID_SESSION_SEARCH                 0x2EED  // 12013 -> dbm_msg_session_search_cb
#define IPC_DBM_MSG_ID_SESSION_UNREAD                 0x2EEE  // 12014 -> dbm_msg_session_unread_cb
#define IPC_DBM_MSG_ID_SESSION_INSERT                 0x2EEF  // 12015 -> dbm_msg_session_insert_cb
#define IPC_DBM_MSG_ID_SESSION_DELETE                 0x2EF0  // 12016 -> dbm_msg_session_delete_cb
#define IPC_DBM_MSG_ID_SESSION_DELETE_SESSION         0x2EF1  // 12017 -> dbm_msg_session_delete_session_cb
#define IPC_DBM_MSG_ID_SESSION_SET_SESSION_STATUS     0x2EF2  // 12018 -> dbm_msg_session_set_session_status_cb
#define IPC_DBM_MSG_ID_SESSION_SET_STATUS             0x2EF3  // 12019 -> dbm_msg_session_set_status_cb
#define IPC_DBM_MSG_ID_SESSION_REACH_MAX              0x2EF4  // 12020 -> dbm_msg_session_reach_max_cb
#define IPC_DBM_MSG_ID_SESSION_USED_STATUS            0x2EF5  // 12021 -> dbm_msg_session_used_status_cb

#define IPC_DBM_MSG_ID_PHONEBOOK_SCAN_REQ             0x2EF6  // 12022 -> dbm_msg_phonebook_scan_req_cd
#define IPC_DBM_MSG_ID_PHONEBOOK_SEARCH_GROUP_REQ     0x2EF7  // 12023 -> dbm_msg_phonebook_search_group_req_cd
#define IPC_DBM_MSG_ID_PHONEBOOK_FUZZY_SEARCH_GROUP   0x2EF8  // 12024 -> dbm_msg_phonebook_fuzzy_search_group_re
#define IPC_DBM_MSG_ID_PHONEBOOK_INSERT_REQ           0x2EF9  // 12025 -> dbm_msg_phonebook_insert_req_cd
#define IPC_DBM_MSG_ID_PHONEBOOK_UPDATE_REQ           0x2EFA  // 12026 -> dbm_msg_phonebook_update_req_cd
#define IPC_DBM_MSG_ID_PHONEBOOK_DELETE_REQ           0x2EFB  // 12027 -> dbm_msg_phonebook_delete_req_cd
#define IPC_DBM_MSG_ID_PHONEBOOK_DELETE_RECORD_ALL    0x2EFC  // 12028 -> phonebook_delete_record_all_req
#define IPC_DBM_MSG_ID_PHONEBOOK_REACH_MAX_REQ        0x2EFD  // 12029 -> dbm_msg_phonebook_reach_max_req

#define IPC_DBM_MSG_ID_PHONEBOOK_GROUP_INSERT_REQ     0x2EFE  // 12030 -> dbm_msg_phonebook_group_insert_req_cd
#define IPC_DBM_MSG_ID_PHONEBOOK_GROUP_UPDATE_REQ     0x2EFF  // 12031 -> dbm_msg_phonebook_group_update_req_cd
#define IPC_DBM_MSG_ID_PHONEBOOK_GROUP_DELETE_REQ     0x2F00  // 12032 -> dbm_msg_phonebook_group_delete_req_cd

#define IPC_DBM_MSG_ID_PHONEBOOK_GET_ALL_RECORDS      0x2F01  // 12033 -> dbm_phonebook_get_all_records
#define IPC_DBM_MSG_ID_PHONEBOOK_ADD_RECORD           0x2F02  // 12034 -> dbm_phonebook_add_record
#define IPC_DBM_MSG_ID_PHONEBOOK_EDIT_RECORD          0x2F03  // 12035 -> dbm_phonebook_edit_record
#define IPC_DBM_MSG_ID_PHONEBOOK_DEL_RECORD           0x2F04  // 12036 -> dbm_phonebook_del_record
#define IPC_DBM_MSG_ID_PHONEBOOK_DEL_ALL_RECORD       0x2F05  // 12037 -> dbm_phonebook_del_all_record

typedef enum dbm_smsbox_type_t {
  dbm_smsbox_draft = 0,
  dbm_smsbox_inbox,
  dbm_smsbox_outbox
} dbm_smsbox_type_t;

typedef struct dbm_set_msg_read_req_t {
  int smsbox;
  uint32_t index;
} dbm_set_msg_read_req_t;

typedef struct dbm_get_msg_list_req_t {
  int smsbox;
  uint32_t start;
  uint32_t end;
} dbm_get_msg_list_req_t;

typedef struct dbm_get_msg_list_res_t {
  uint32_t total_msg_count;
  uint32_t record_count;
  struct {
    uint32_t index;
    int smsbox;
    int is_read;
    uint32_t timestamp;
    char phone[720];
    char content[1024];
    char contacts[64];
  } records [10];
} dbm_get_msg_list_res_t;

typedef struct dbm_get_msg_count_res_t {
  uint32_t draft_count;
  uint32_t outbox_count;
  uint32_t unread_count;
  uint32_t inbox_count;
  uint32_t max_count;
} dbm_get_msg_count_res_t;
