
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "api/base.h"
#include "api/json.h"
#include "util/setting.h"
#include "util/http.h"
#include "util/encrypt.h"
#include "util/encode.h"
#include "util/vector.h"
#include "core/sms.h"
#include "pushs.h"

#define LOG_TAG "push.smartroute"
#include "util/log.h"

static char* g_api_url;
static char* g_api_token;
static uint8_t g_aes_key[32];
static uint8_t g_aes_iv[16];
static bool g_key_valid;
static vector_t g_records;

static char* jsonify_record(const sms_record_t* r) {
  json_t* obj = json_create_item(JSON_OBJECT);
  if (!obj)
    return NULL;

  json_add_object_item(obj, "phone",
    json_create_string(r->phone ? r->phone : ""));
  json_add_object_item(obj, "contacts",
    json_create_string(r->contacts ? r->contacts : ""));
  json_add_object_item(obj, "content",
    json_create_string(r->content ? r->content : ""));
  json_add_object_item(obj, "timestamp",
    json_create_number(r->timestamp));

  char* str = json_print_buffered(obj, 0, 0);
  json_free(obj);
  return str;
}

static void smartroute_init() {
  vector_init(&g_records, sizeof(char*));
  LOG_I("smartroute backend inited");
}

static void smartroute_dispose() {
  size_t len = vector_len(&g_records);
  char** data = (char**)vector_data(&g_records);
  for (size_t i = 0; i < len; i++)
    os_mem_free(data[i]);

  free(g_api_url);
  free(g_api_token);
  g_api_url = NULL;
  g_api_token = NULL;
  g_key_valid = false;
  vector_clear(&g_records);
  vector_free(&g_records);
}

static void smartroute_load_config() {
  smartroute_dispose();
  vector_init(&g_records, sizeof(char*));

  g_api_url = setting_get_str("push.smartroute.apiUrl", "");
  g_api_token = setting_get_str("push.smartroute.apiToken", "");
  char* key_hex = setting_get_str("push.smartroute.aesKey", "");
  char* iv_hex = setting_get_str("push.smartroute.aesIv", "");

  g_key_valid = encode_hex_decode(key_hex, g_aes_key, 32)
             && encode_hex_decode(iv_hex, g_aes_iv, 16);

  free(key_hex);
  free(iv_hex);

  LOG_I("apiUrl=%s keyValid=%d", g_api_url, g_key_valid);
}

static void smartroute_add_task(push_task_t* task) {
  if (task->type == push_type_msgs && task->records) {
    for (sms_record_t* r = task->records; r->phone || r->content; r++) {
      char* str = jsonify_record(r);
      if (str)
        vector_push(&g_records, &str);
    }
  } else if (task->type == push_type_alert_smsbox_almost_full) {
    json_t* obj = json_create_item(JSON_OBJECT);
    if (!obj)
      return;

    json_add_object_item(obj, "type", json_create_string("alert"));
    json_add_object_item(obj, "content",
      json_create_string("SMS storage almost full"));

    json_add_object_item(obj, "total_count",
      json_create_number(task->msg_count.draft_count
                         + task->msg_count.inbox_count
                         + task->msg_count.outbox_count));
    json_add_object_item(obj, "max_count",
      json_create_number(task->msg_count.max_count));
    json_add_object_item(obj, "inbox_count",
      json_create_number(task->msg_count.inbox_count));

    char* str = json_print_buffered(obj, 0, 0);
    json_free(obj);
    if (str)
      vector_push(&g_records, &str);
  }
}

static void smartroute_submit() {
  size_t count = vector_len(&g_records);
  if (count == 0)
    return;

  char** data = (char**)vector_data(&g_records);

  if (!g_api_url || !*g_api_url || !g_key_valid) {
    LOG_W("smartroute not configured, dropping %zu records", count);
    for (size_t i = 0; i < count; i++)
      os_mem_free(data[i]);
    vector_clear(&g_records);
    return;
  }

  json_t* arr = json_create_item(JSON_ARRAY);
  if (arr) {
    for (size_t i = 0; i < count; i++) {
      json_t* obj = json_parse(data[i]);
      if (obj)
        json_add_array_item(arr, obj);
    }

    char* full = json_print_buffered(arr, 0, 0);
    json_free(arr);

    if (full) {
      char* cipher = encrypt_data(full, strlen(full),
                                  g_aes_key, 32, g_aes_iv, 1);
      os_mem_free(full);

      if (cipher) {
        http_req_t req;
        http_request_create(&req, g_api_url);
        if (g_api_token && *g_api_token) {
          http_request_header(&req, "Authorization", g_api_token);
        }
        http_res_t resp = http_post(&req, cipher);

        if (resp.code >= 200 && resp.code < 300)
          LOG_I("push success, code=%u", resp.code);
        else
          LOG_E("push failed, code=%u", resp.code);

        http_response_dispose(&resp);
        http_request_dispose(&req);
        os_mem_free(cipher);
      }
    }
  }

  for (size_t i = 0; i < count; i++)
    os_mem_free(data[i]);
  vector_clear(&g_records);
}

const push_backend_t smartroute_backend = {
  .name = "smartroute",
  .min_interval_ms = 1000,
  .init = smartroute_init,
  .dispose = smartroute_dispose,
  .load_config = smartroute_load_config,
  .add_task = smartroute_add_task,
  .submit = smartroute_submit,
};
