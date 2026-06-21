
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>

#include "api/ipc/mnet.h"
#include "api/ipc/dbm.h"
#include "api/ipc/sms.h"
#include "api/frwk.h"
#include "api/base.h"
#include "api/json.h"
#include "core/push.h"
#include "util/setting.h"
#include "sms.h"

#define LOG_TAG "core.sms"
#include "util/log.h"


static pthread_mutex_t sms_lock;

static char temp_buffer[2048];  // 2kb
static dbm_get_msg_list_res_t sms_msg_page = {0};
static dbm_get_msg_count_res_t sms_msg_cnt = {0};

static bool sms_session_enable = false;
static bool sms_auto_clean;
static bool sms_allow_clean_outbox;
static uint32_t sms_min_inbox_msg_num;
static uint32_t sms_msg_cnt_threshold;

static void sms_update_session_conf() {
  pthread_mutex_lock(&sms_lock);
  LOG_I("fetching session conf");
  char enabled[4] = {0};
  int res = mgdb_get("sms_session_enable", enabled, sizeof(enabled));
  if (res) {
    LOG_E("failed to fetch session conf, code: %d", res);
    return;
  }
  sms_session_enable = strcasecmp(enabled, "on") == 0;
  // session mode is supported in sms and dbm service
  // but it's not used in webserver and webui
  if (sms_session_enable)
    LOG_W("sms session mode is not supported");
  pthread_mutex_unlock(&sms_lock);
}

void sms_init() {
  const char* msg_name = mg_enum_to_str_apps_message_id_e_type(
      IPC_SMS_MSG_ID_SMS_NOTIFY_MSG_COUNT);
  int res = frwk_ipc_msg_sub(msg_name);
  if (res != FRWK_OK)
    LOG_E("failed to subscribe sms event, code: %d", res);
  msg_name =
      mg_enum_to_str_apps_message_id_e_type(IPC_MNET_MSG_ID_NOTIFY_SIM_STATUS);
  res = frwk_ipc_msg_sub(msg_name);
  if (res != FRWK_OK)
    LOG_E("failed to subscribe sim event, code: %d", res);

  pthread_mutex_init(&sms_lock, NULL);
  sms_update_session_conf();
  LOG_I("sms inited");
}

void sms_load_config() {
  pthread_mutex_lock(&sms_lock);
  sms_auto_clean = setting_get_bool("sms.autoClean", false);
  sms_allow_clean_outbox = setting_get_bool("sms.allowCleanOutbox", false);
  sms_min_inbox_msg_num = (uint32_t)setting_get_int("sms.minInboxMsgNum", 75);
  // default smsbox capacity is 100
  sms_msg_cnt_threshold =
      (uint32_t)setting_get_int("sms.msgCountThreshold", 75);
  if (sms_min_inbox_msg_num > sms_msg_cnt_threshold)
    sms_min_inbox_msg_num = sms_msg_cnt_threshold;
  pthread_mutex_unlock(&sms_lock);
}

static void sms_record_free(sms_record_t* record) {
  if (!record)
    return;
  if (record->content)
    free(record->content);
  if (record->phone)
    free(record->phone);
  if (record->contacts)
    free(record->contacts);
  record->content = record->phone = record->contacts = NULL;
}

static uint32_t sms_dump_unread_record_from_page(sms_record_t* record,
                                                 uint32_t index) {
  if (index >= sms_msg_page.record_count)
    return 0;
  if (*sms_msg_page.records[index].phone == 0 ||
        *sms_msg_page.records[index].content == 0)
    return 0;
  if (sms_msg_page.records[index].is_read == 1 ||
      sms_msg_page.records[index].smsbox != dbm_smsbox_inbox)
    return 0;
  record->timestamp = sms_msg_page.records[index].timestamp;
  record->phone = strdup(sms_msg_page.records[index].phone);
  record->content = strdup(sms_msg_page.records[index].content);
  record->contacts = strdup(sms_msg_page.records[index].contacts);
  if (!record->phone || !record->content || !record->contacts) {
    LOG_E("no more mem");
    sms_record_free(record);
    return 0;
  }
  return sms_msg_page.records[index].index;
}

static bool sms_get_msg_page(dbm_smsbox_type_t type, uint32_t from,
                             uint32_t to) {
  dbm_get_msg_list_req_t req = {.smsbox = type, .start = from, .end = to};
  int res0 = -1;
  int res1 = frwk_ipc_send_sync(IPC_MODULE_DBM, IPC_DBM_MSG_ID_GET_MSG_LIST,
                                &req, sizeof(req), &sms_msg_page,
                                sizeof(sms_msg_page), 10, &res0);
  if (res0 != FRWK_OK || res1 != FRWK_OK) {
    LOG_W("failed to get msg page from %d to %d, code: %d, %d", from, to, res0, res1);
    return false;
  }
  return true;
}

static bool sms_mark_msg_is_read(uint32_t index) {
  if (index <= 0)
    return false;
  dbm_set_msg_read_req_t req = {.smsbox = 1, .index = index};
  int res0 = -1;
  int res1 =
      frwk_ipc_send_sync(IPC_MODULE_DBM, IPC_DBM_MSG_ID_SET_MSG_READ, &req,
                         sizeof(req), temp_buffer, 1824, 10, &res0);
  if (res0 != FRWK_OK || res1 != FRWK_OK) {
    LOG_W("failed to mark msg is read, code: %d, %d", res0, res1);
    return false;
  }
  return true;
}

static bool sms_get_msg_count() {
  int res0 = -1;
  int res1 =
      frwk_ipc_send_sync(IPC_MODULE_DBM, IPC_DBM_MSG_ID_GET_MSG_COUNT, NULL, 0,
                         &sms_msg_cnt, sizeof(sms_msg_cnt), 10, &res0);
  if (res0 != FRWK_OK || res1 != FRWK_OK) {
    LOG_W("failed to read msg cnt, code: %d, %d", res0, res1);
    return false;
  }
  return true;
}

static void sms_update_unread_msg_count() {
  int res = frwk_mgdb_set_key_value("sms_unread_count", FRWK_MGDB_TYPE_INT,
                                    &sms_msg_cnt.unread_count);
  if (res != FRWK_OK)
    LOG_W("failed to update msg count, code: %d", res);
}

static bool sms_remove_msgs(dbm_smsbox_type_t type, json_t* index_array) {
  json_t* json = json_create_item(JSON_OBJECT);
  json_t* smsbox = json_create_number(type);
  if (!json || !smsbox) {
    LOG_E("no more mem");
    return false;
  }
  json_add_object_item(json, "index", index_array);
  json_add_object_item(json, "smsbox", smsbox);
  char* request = json_print_buffered(json, 128, 0);
  json_free(json);
  if (!request) {
    LOG_E("no more mem");
    return false;
  }

  // using high level method
  // a update in sms service is needed for updating flag `sms_msg_count_reach_max`
  int res0 = -1;
  int res1 = frwk_ipc_send_sync(
      IPC_MODULE_SMS, IPC_SMS_MSG_ID_DELETE_MESSAGE_NORMAL, request,
      strlen(request) + 1, temp_buffer, sizeof(temp_buffer), 10, &res0);
  os_mem_free(request);
  if (res0 != FRWK_OK || res1 != FRWK_OK) {
    LOG_E("failed to remove msgs, code: %d, %d", res0, res1);
    return false; 
  }
  return true;
}

static bool sms_clean_smsbox(dbm_smsbox_type_t type, uint32_t num) {
  uint32_t* cnt = NULL;
  if (type == dbm_smsbox_inbox)
    cnt = &sms_msg_cnt.inbox_count;
  else if (type == dbm_smsbox_outbox)
    cnt = &sms_msg_cnt.outbox_count;
  else if (type == dbm_smsbox_draft)
    cnt = &sms_msg_cnt.draft_count;
  else
    return false;
  if (num == 0)
    return true;
  // no enough msgs to clean
  if (num > *cnt)
    return false;

  uint32_t from = *cnt - 1u - ((*cnt - 1) % 10) + 1u + 10u;
  uint32_t to = from;
  char txt[16];
  json_t* indexes = json_create_item(JSON_ARRAY);
  if (!indexes) {
    LOG_E("no more mem");
    return false;
  }
  for (uint32_t n = 0; n < num; ) {
    if (to <= from) {
      // no enough msgs to clean
      if (from <= 10)
        goto FAIL;
      from -= 10u;
      if (!sms_get_msg_page(type, from, from + 9u))
        goto FAIL;
      to = from + sms_msg_page.record_count - 1;
    } else {
      to --;
    }
    uint32_t i = to - from;
    if (sms_msg_page.records[i].is_read) {
      snprintf(txt, sizeof(txt), "%u", sms_msg_page.records[i].index);
      json_t* index = json_create_string(txt);
      if (!index) {
        LOG_E("no more mem");
        goto FAIL;
      }
      json_add_array_item(indexes, index);
      n++;
    }
  }

  if (!sms_remove_msgs(type, indexes)) {
    LOG_E("failed to remove %u msg(s) from smsbox %d", num, (int)type);
    return false;
  }
  *cnt -= num;
  LOG_I("remove %u msg(s) from smsbox %d", num, (int) type);
  return true;
FAIL:
  json_free(indexes);
  LOG_E("failed");
  return false;
}

static bool sms_clean_smsboxes() {
  LOG_I("cleaning smsbox");
  uint32_t msg_total_cnt = sms_msg_cnt.draft_count + sms_msg_cnt.inbox_count +
                           sms_msg_cnt.outbox_count;
  if (sms_msg_cnt.inbox_count > sms_min_inbox_msg_num) {
    uint32_t to_clean = sms_msg_cnt.inbox_count - sms_min_inbox_msg_num;
    if (to_clean > msg_total_cnt - sms_msg_cnt_threshold)
      to_clean = msg_total_cnt - sms_msg_cnt_threshold;
    if (!sms_clean_smsbox(dbm_smsbox_inbox, to_clean))
      return false;
    msg_total_cnt -= to_clean;
  }
  if (msg_total_cnt > sms_msg_cnt_threshold) {
    uint32_t to_clean = msg_total_cnt - sms_msg_cnt_threshold;
    bool not_perfect = to_clean > sms_msg_cnt.outbox_count;
    if (not_perfect)
      to_clean = sms_msg_cnt.outbox_count;
    if (!sms_clean_smsbox(dbm_smsbox_outbox, to_clean))
      return false;
    // inform that manual clean is needed
    if (!not_perfect)
      return false;
  }
  return true;
}

void sms_check_new_msg() {
  pthread_mutex_lock(&sms_lock);
  LOG_I("checking new message");
  if (sms_session_enable) {
    LOG_W("sms session mode is on, skipped");
    pthread_mutex_unlock(&sms_lock);
    return;
  }

  if (!sms_get_msg_count()) {
    LOG_W("failed, skipped");
    pthread_mutex_unlock(&sms_lock);
    return;
  }
  if (sms_msg_cnt.unread_count == 0) {
    LOG_I("no more new msg");
    pthread_mutex_unlock(&sms_lock);
    return;
  }

  sms_record_t* records =
      malloc((sms_msg_cnt.unread_count + 1) * sizeof(sms_record_t));
  if (!records) {
    LOG_W("failed to allocate records buffer, skipped");
    pthread_mutex_unlock(&sms_lock);
    return;
  }
  records[0].content = records[0].phone = NULL;

  uint32_t processd = 0;
  for (uint32_t i = 1; i <= sms_msg_cnt.inbox_count; i += 10) {
    if (!sms_get_msg_page(dbm_smsbox_inbox, i, i + 9u)) {
      sms_records_free(records);
      LOG_W("failed, skipped");
      pthread_mutex_unlock(&sms_lock);
      return;
    }

    for (uint32_t j = 0; j < sms_msg_page.record_count; j++) {
      sms_record_t* record = records + processd;
      int index = sms_dump_unread_record_from_page(record, j);
      if (index > 0) {
        // using low level method to mark to avoid unnecessary msg count update events
        if (sms_mark_msg_is_read(index)) {
          processd++;
          records[processd].content = records[processd].phone = NULL;
          if (processd >= sms_msg_cnt.unread_count)
            break;
        } else {
          sms_record_free(record);
        }
      }
    }

    if (processd >= sms_msg_cnt.unread_count)
      break;
  }
  // update once
  // read msg(s) won't produce any new msgs
  sms_update_unread_msg_count();

  for (uint32_t i = 0; records[i].phone; i++)
    LOG_D("msg: contacts: %s, phone: %s, time: %u, content: %s",
          records[i].contacts, records[i].phone, records[i].timestamp,
          records[i].content);

  LOG_I("got %u unread msg(s), pushing", processd);
  push_submit_msgs(records);

  // when smsbox is full, no more new msgs can be sended from or received to smsbox
  // after cleaning smsbox, sms serivce will call `sms_fresh_message_from_modem`
  // so maybe no msgs will be lost
  uint32_t msg_total_cnt = sms_msg_cnt.draft_count + sms_msg_cnt.inbox_count +
                           sms_msg_cnt.outbox_count;
  if (msg_total_cnt > sms_msg_cnt_threshold) {
    LOG_I("smsbox is almost full, current: %u, max: %u", msg_total_cnt, sms_msg_cnt.max_count);
    if (sms_auto_clean) {
      if (!sms_clean_smsboxes())
        push_alert_smsbox_almost_full();
    } else {
      push_alert_smsbox_almost_full();
    }
  }

  pthread_mutex_unlock(&sms_lock);
}

int sms_ipc_handler(unsigned int from_module_id, int msg_type_id,
                    const void* input, unsigned int input_len, void* output,
                    unsigned int output_len, unsigned int timeout_ms) {
  if (from_module_id == IPC_MODULE_SMS &&
      msg_type_id == IPC_SMS_MSG_ID_SMS_NOTIFY_MSG_COUNT)
    sms_check_new_msg();
  if (from_module_id == IPC_MODULE_MNET &&
      msg_type_id == IPC_MNET_MSG_ID_NOTIFY_SIM_STATUS)
    sms_update_session_conf();
  return 0;
}

void sms_records_free(sms_record_t* records) {
  if (!records)
    return;
  sms_record_t* p = records;
  while (p->content || p->phone) {
    sms_record_free(p);
    p++;
  }
  free(records);
}
