
#pragma once

#define IPC_MODULE_SMS  11

#define IPC_SMS_MSG_ID_SEND_NEW_MESSAGE              0x2AF9  // 11001 -> sms_send_new_message_wrap
#define IPC_SMS_MSG_ID_GET_MSG_COUNT_INFO            0x2AFB  // 11003 -> sms_get_msg_count_info_wrap
#define IPC_SMS_MSG_ID_SET_SCA_ADDRESS               0x2AFC  // 11004 -> sms_set_sca_address_wrap
#define IPC_SMS_MSG_ID_SET_STATUS_REPORT             0x2AFD  // 11005 -> sms_set_status_report_wrap
#define IPC_SMS_MSG_ID_SET_MAX_COUNT                 0x2AFE  // 11006 -> sms_set_max_count_wrap
#define IPC_SMS_MSG_ID_SET_MAX_RECIPIENT             0x2AFF  // 11007 -> sms_set_max_recipient_wrap
#define IPC_SMS_MSG_ID_GET_MESSAGE_LIST              0x2B00  // 11008 -> sms_get_message_list_wrap
#define IPC_SMS_MSG_ID_SET_MESSAGE_READ_INFO         0x2B01  // 11009 -> sms_set_message_read_info_wrap
#define IPC_SMS_MSG_ID_INSERT_MESSAGE_NORMAL         0x2B02  // 11010 -> sms_insert_message_noraml_wrap
#define IPC_SMS_MSG_ID_DELETE_MESSAGE_NORMAL         0x2B03  // 11011 -> sms_delete_message_noraml_wrap

#define IPC_SMS_MSG_ID_PROCESS_SIM_STATUS_CHANGE     0x0FB0  // 4016  -> sms_process_sim_status_change_wrap

#define IPC_SMS_MSG_ID_GET_MAX_COUNT                 0x2B04  // 11012 -> sms_get_max_count_wrap
#define IPC_SMS_MSG_ID_GET_MAX_RECIPIENT             0x2B05  // 11013 -> sms_get_max_recipient_wrap
#define IPC_SMS_MSG_ID_SET_CELLLOCK_NUMBERS          0x2B08  // 11016 -> sms_set_celllock_numbers_wrap
#define IPC_SMS_MSG_ID_SET_VALID_PERIOD              0x2B09  // 11017 -> sms_set_valid_period_wrap
#define IPC_SMS_MSG_ID_SET_FORWARD                   0x2B0A  // 11018 -> sms_set_forward_wrap

#define IPC_SMS_MSG_ID_SESSION_DEL_MSG_BY_ID         0x2B0B  // 11019 -> sms_session_del_msg_by_id_wrap
#define IPC_SMS_MSG_ID_SESSION_DEL_MSG_BY_IDS        0x2B0C  // 11020 -> sms_session_del_msg_by_ids_wrap
#define IPC_SMS_MSG_ID_SESSION_SET_MSG_READ_STATUS   0x2B0D  // 11021 -> sms_session_set_msg_read_status_wrap
#define IPC_SMS_MSG_ID_SESSION_GET_MSG_BY_NUMBER     0x2B0E  // 11022 -> sms_session_get_msg_by_number_wrap
#define IPC_SMS_MSG_ID_SESSION_GET_MSG_BY_CONDITION  0x2B0F  // 11023 -> sms_session_get_msg_by_condition_wrap
#define IPC_SMS_MSG_ID_SESSION_GET_MSG_COUNT         0x2B10  // 11024 -> sms_session_get_msg_count_wrap

#define IPC_SMS_MSG_ID_SMS_NOTIFY_MSG_COUNT          0x2B07  // 11015
