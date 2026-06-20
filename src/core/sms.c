
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "api/frwk.h"
#include "api/ipc/dbm.h"
#include "api/ipc/sms.h"
#include "api/json.h"
#include "sms.h"

#define LOG_TAG "core.sms"
#include "util/log.h"

#define SMS_BUFFER_SIZE 16384  // 16kb
#define SMS_TEMP_BUFFER_SIZE 2048  // 2kb

static char* sms_buffer = NULL;
static char* temp_buffer = NULL;

void sms_init() {
  const char* msg_name = mg_enum_to_str_apps_message_id_e_type(
      IPC_SMS_MSG_ID_SMS_NOTIFY_MSG_COUNT);
  int res = frwk_ipc_msg_sub(msg_name);
  if (res != FRWK_OK)
    LOG_E("failed to subscribe sms event, code: %d", res);

  sms_buffer = malloc(SMS_BUFFER_SIZE); 
  if (!sms_buffer)
    LOG_E("failed to allocate buffer");
  temp_buffer = malloc(SMS_TEMP_BUFFER_SIZE);
  if (!temp_buffer)
    LOG_E("failed to allocate temp buffer");

  LOG_I("sms inited");
}

static void sms_record_free(sms_record_t* record) {
  if (!record)
    return;
  if (record->content)
    free(record->content);
  if (record->phone)
    free(record->phone);
  record->content = record->phone = NULL;
}

static int sms_parse_msg_record(sms_record_t* record, json_t* node) {
  if (!node || !record)
    return -1;
  record->content = record->phone = NULL;
  int index = 0;
  json_t* item = json_get_object_item(node, "isread");
  if (!item || item->type != JSON_NUMBER)
    goto PARSE_FAIL;
  if (item->value_int == 1)
    return 0;

  item = json_get_object_item(node, "index");
  if (!item || item->type != JSON_NUMBER)
    goto PARSE_FAIL;
  index = item->value_int;

  item = json_get_object_item(node, "datetime");
  if (!item || item->type != JSON_NUMBER)
    goto PARSE_FAIL;
  record->timestamp = item->value_int;

  item = json_get_object_item(node, "phone");
  if (!item || item->type != JSON_STRING)
    goto PARSE_FAIL;
  record->phone = strdup(item->value_string);
  if (!record->phone)
    goto MEM_FAIL;

  item = json_get_object_item(node, "content");
  if (!item || item->type != JSON_STRING)
    goto PARSE_FAIL;
  record->content = strdup(item->value_string);
  if (!record->content)
    goto MEM_FAIL;

  return index;
MEM_FAIL:
  LOG_W("no more mem");
  goto FAIL;
PARSE_FAIL:
  LOG_W("invalid msg record");
FAIL:
  sms_record_free(record);
  return -1;
}

static json_t* sms_resolve_msg_list(json_t *json) {
  json_t* retcode = json_get_object_item(json, "retcode");
  if (!retcode || retcode->type != JSON_NUMBER || retcode->value_int != 0) {
    LOG_W("msg info query failed");
    return NULL;
  }
  json_t* list = json_get_object_item(json, "smsList");
  if (!list || list->type != JSON_ARRAY) {
    LOG_W("invalid query result");
    return NULL;
  }
  return list;
}

static bool sms_mark_msg_is_read(int index) {
  if (index <= 0)
    return false;
  snprintf(temp_buffer, SMS_TEMP_BUFFER_SIZE, "{\"smsbox\":1,\"index\":%d}", index);
  int res0 = -1;
  int res1 = frwk_ipc_send_sync(
      IPC_MODULE_SMS, IPC_SMS_MSG_ID_SET_MESSAGE_READ_INFO, temp_buffer,
      SMS_TEMP_BUFFER_SIZE, sms_buffer, SMS_BUFFER_SIZE, 10, &res0);
  if (res0 != FRWK_OK || res1 != FRWK_OK) {
    LOG_W("failed to mark msg is read, code: %d, %d", res0, res1);
    return false;
  }
  return true;
}

void sms_check_new_msg() {
  LOG_I("checking new message");
  dbm_msg_count_t msg_cnt = {0};
  int res0 = -1;
  int res1 = frwk_ipc_send_sync(IPC_MODULE_DBM, IPC_DBM_MSG_ID_GET_MSG_COUNT,
                                NULL, 0, &msg_cnt, sizeof(msg_cnt), 10, &res0);
  if (res0 != FRWK_OK || res1 != FRWK_OK) {
    LOG_W("failed to read msg cnt, code: %d, %d", res0, res1);
    return;
  }
  if (msg_cnt.unread_count == 0) {
    LOG_I("no more new msg");
    return;
  }

  if (!sms_buffer || !temp_buffer) {
    LOG_W("no all buffers are allocated, skipped");
    return;
  }

  sms_record_t* records =
      malloc((msg_cnt.unread_count + 1) * sizeof(sms_record_t));
  if (!records) {
    LOG_W("failed to allocate records buffer, skipped");
    return;
  }

  uint32_t processd = 0;
  for (uint32_t i = 1; i < msg_cnt.inbox_count; i += 10) {
    snprintf(temp_buffer, SMS_TEMP_BUFFER_SIZE,
             "{\"start\":%u,\"end\":%u,\"smsbox\":1}", i, i + 9u);
    res0 = -1;
    res1 = frwk_ipc_send_sync(IPC_MODULE_SMS, IPC_SMS_MSG_ID_GET_MESSAGE_LIST,
                              temp_buffer, SMS_TEMP_BUFFER_SIZE, sms_buffer,
                              SMS_BUFFER_SIZE, 10, &res0);
    if (res0 != FRWK_OK || res1 != FRWK_OK) {
      LOG_W("failed to read msg info, code: %d, %d", res0, res1);
      goto FAIL;
    }

    json_t* json = json_parse(sms_buffer);
    if (!json) {
      LOG_W("failed to parse msg info json");
      goto FAIL;
    }
    json_t* list = sms_resolve_msg_list(json);
    if (!list)
      goto FAIL;
    int cnt = json_get_child_num(list);
    for (int j = 0; j < cnt; j++) {
      json_t* item = json_get_array_item(list, j);
      sms_record_t* record = records + processd;
      int index = sms_parse_msg_record(record, item);
      if (index > 0) {
        if (sms_mark_msg_is_read(index)) {
          if (++processd >= msg_cnt.unread_count)
            break;
        } else {
          sms_record_free(record);
        }
      }
    }
    json_free(json);

    if (processd >= msg_cnt.unread_count)
      break;
  }
  records[processd].content = records[processd].phone = NULL;

  LOG_I("got %u unread msg(s), pushing", processd);

  return;
FAIL:
  free(records);
  LOG_W("failed, skipped");
  return;
}

int sms_ipc_handler(unsigned int from_module_id, int msg_type_id,
                    const void* input, unsigned int input_len, void* output,
                    unsigned int output_len, unsigned int timeout_ms) {
  if (msg_type_id == IPC_SMS_MSG_ID_SMS_NOTIFY_MSG_COUNT)
    sms_check_new_msg();
  return 0;
}

void sms_records_free(sms_record_t* record) {
  if (!record)
    return;
  sms_record_t* p = record;
  while (p->content || p->phone) {
    sms_record_free(p);
    p++;
  }
  free(record);
}
