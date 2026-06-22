
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <stdbool.h>

#include "api/base.h"
#include "util/json.h"
#include "util/setting.h"
#include "util/http.h"
#include "util/string.h"
#include "util/vector.h"
#include "core/sms.h"
#include "pushs.h"

#define LOG_TAG "push.wxpusher"
#include "util/log.h"

#define API_SEND_MESSAGE \
  "https://wxpusher.zjiecode.com/api/send/message"
#define API_SEND_SIMPLE \
  "https://wxpusher.zjiecode.com/api/send/message/simple-push"
#define CONTENT_MAX_LEN 40000

static char* g_mode;
static char* g_app_token;
static char** g_uids;
static int* g_topic_ids;
static char* g_spt;
static char** g_spt_list;
static int g_content_type;
static char* g_summary;
static char* g_url;
static char* g_template_msg;
static char* g_template_alert;
static char* g_separator;

static vector_t g_msg_buf;
static dbm_get_msg_count_res_t g_dbm_cnt;
static bool g_dbm_cnt_valid;

static char* resolve_var(const char* var, void* ctx) {
  const sms_record_t* record = (const sms_record_t*)ctx;
  bool is_html = (g_content_type == 2);
  char tmp[64];

  if (strcmp(var, "phone") == 0) {
    const char* v = (record && record->phone) ? record->phone : "";
    return is_html ? str_escape(v) : strdup(v);
  }
  if (strcmp(var, "contacts") == 0) {
    const char* v = (record && record->contacts) ? record->contacts : "";
    return is_html ? str_escape(v) : strdup(v);
  }
  if (strcmp(var, "content") == 0) {
    const char* v = (record && record->content) ? record->content : "";
    return is_html ? str_escape(v) : strdup(v);
  }
  if (strcmp(var, "timestamp") == 0) {
    uint32_t ts = record ? record->timestamp : 0;
    snprintf(tmp, sizeof(tmp), "%" PRIu32, ts);
    return strdup(tmp);
  }
  if (strcmp(var, "datetime") == 0) {
    if (record && record->timestamp > 0) {
      time_t ts = record->timestamp;
      struct tm tm_val;
      localtime_r(&ts, &tm_val);
      strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", &tm_val);
      return strdup(tmp);
    }
    return strdup("");
  }
  if (strcmp(var, "inbox_count") == 0) {
    uint32_t v = g_dbm_cnt_valid ? g_dbm_cnt.inbox_count : 0;
    snprintf(tmp, sizeof(tmp), "%" PRIu32, v);
    return strdup(tmp);
  }
  if (strcmp(var, "unread_count") == 0) {
    uint32_t v = g_dbm_cnt_valid ? g_dbm_cnt.unread_count : 0;
    snprintf(tmp, sizeof(tmp), "%" PRIu32, v);
    return strdup(tmp);
  }
  if (strcmp(var, "outbox_count") == 0) {
    uint32_t v = g_dbm_cnt_valid ? g_dbm_cnt.outbox_count : 0;
    snprintf(tmp, sizeof(tmp), "%" PRIu32, v);
    return strdup(tmp);
  }
  if (strcmp(var, "draft_count") == 0) {
    uint32_t v = g_dbm_cnt_valid ? g_dbm_cnt.draft_count : 0;
    snprintf(tmp, sizeof(tmp), "%" PRIu32, v);
    return strdup(tmp);
  }
  if (strcmp(var, "total_count") == 0) {
    uint32_t v = g_dbm_cnt_valid
      ? g_dbm_cnt.draft_count + g_dbm_cnt.inbox_count
        + g_dbm_cnt.outbox_count
      : 0;
    snprintf(tmp, sizeof(tmp), "%" PRIu32, v);
    return strdup(tmp);
  }
  if (strcmp(var, "max_count") == 0) {
    uint32_t v = g_dbm_cnt_valid ? g_dbm_cnt.max_count : 0;
    snprintf(tmp, sizeof(tmp), "%" PRIu32, v);
    return strdup(tmp);
  }

  return NULL;
}

static void free_state() {
  free(g_mode);
  free(g_app_token);
  free(g_spt);
  free(g_summary);
  free(g_url);
  free(g_template_msg);
  free(g_template_alert);
  free(g_separator);

  if (g_uids) {
    for (int i = 0; g_uids[i]; i++)
      free(g_uids[i]);
    free(g_uids);
  }
  if (g_spt_list) {
    for (int i = 0; g_spt_list[i]; i++)
      free(g_spt_list[i]);
    free(g_spt_list);
  }
  free(g_topic_ids);

  vector_free(&g_msg_buf);

  g_mode = NULL;
  g_app_token = NULL;
  g_spt = NULL;
  g_summary = NULL;
  g_url = NULL;
  g_template_msg = NULL;
  g_template_alert = NULL;
  g_separator = NULL;
  g_uids = NULL;
  g_spt_list = NULL;
  g_topic_ids = NULL;
  g_dbm_cnt_valid = false;
}

static void wxpusher_init() {
  vector_init(&g_msg_buf, 1);
  LOG_I("wxpusher backend inited");
}

static void wxpusher_dispose() {
  free_state();
  LOG_I("wxpusher backend disposed");
}

static void wxpusher_load_config() {
  free_state();

  g_mode = setting_get_str("push.wxpusher.mode", "standard");
  g_content_type = setting_get_int("push.wxpusher.contentType", 2);
  g_app_token = setting_get_str("push.wxpusher.appToken", "");
  g_spt = setting_get_str("push.wxpusher.spt", "");
  g_summary = setting_get_str("push.wxpusher.summary", "");
  g_url = setting_get_str("push.wxpusher.url", "");
  g_template_msg = setting_get_str(
      "push.wxpusher.template_msg",
      "<h2>新短信</h2>"
      "<pre style=\"font-size:14px;white-space:pre-wrap\">{content}</pre>"
      "<p>---</p><table>"
      "<tr><td><b>发件人</b></td><td>{contacts}</td></tr>"
      "<tr><td><b>号码</b></td><td>{phone}</td></tr>"
      "<tr><td><b>时间</b></td><td>{datetime}</td></tr>"
      "</table>");
  g_template_alert = setting_get_str("push.wxpusher.template_alert",
    "<b>短信即将存满！</b><br>已用: {total_count}/{max_count}");
  g_separator = setting_get_str("push.wxpusher.separator", "<hr>");
  g_uids = json_load_str_array(
    (json_t*)setting_get_raw("push.wxpusher.uids"));
  g_topic_ids = json_load_int_array(
    (json_t*)setting_get_raw("push.wxpusher.topicIds"));
  g_spt_list = json_load_str_array(
    (json_t*)setting_get_raw("push.wxpusher.sptList"));

  LOG_I("mode=%s contentType=%d", g_mode, g_content_type);
}

static void wxpusher_add_task(push_task_t* task) {
  if (task->type == push_type_msgs && task->records) {
    for (sms_record_t* r = task->records; r->phone || r->content; r++) {
      char* text = str_template(g_template_msg, resolve_var, r);
      if (!text)
        continue;

      if (vector_len(&g_msg_buf) > 0 && g_separator)
        vector_push_n(&g_msg_buf, g_separator, strlen(g_separator));
      vector_push_n(&g_msg_buf, text, strlen(text));
      free(text);
    }
  } else if (task->type == push_type_alert_smsbox_almost_full) {
    g_dbm_cnt = task->msg_count;
    g_dbm_cnt_valid = true;
    char* text = str_template(g_template_alert, resolve_var, NULL);
    if (!text)
      return;

    if (vector_len(&g_msg_buf) > 0 && g_separator)
      vector_push_n(&g_msg_buf, g_separator, strlen(g_separator));
    vector_push_n(&g_msg_buf, text, strlen(text));
    free(text);
  }
}

static void wxpusher_submit() {
  if (vector_len(&g_msg_buf) == 0)
    return;

  char* buf = vector_data(&g_msg_buf);
  size_t blen = vector_len(&g_msg_buf);

  if (blen > CONTENT_MAX_LEN) {
    buf[CONTENT_MAX_LEN] = '\0';
    vector_resize(&g_msg_buf, CONTENT_MAX_LEN);
  }

  json_t* body = json_create_item(JSON_OBJECT);
  if (!body) {
    LOG_E("failed to create json body");
    vector_clear(&g_msg_buf);
    return;
  }

  json_add_object_item(body, "content", json_create_string(buf));
  json_add_object_item(body, "contentType",
                       json_create_number(g_content_type));

  if (g_summary && *g_summary)
    json_add_object_item(body, "summary", json_create_string(g_summary));
  if (g_url && *g_url)
    json_add_object_item(body, "url", json_create_string(g_url));

  const char* api_url;
  if (strcmp(g_mode, "spt") == 0) {
    api_url = API_SEND_SIMPLE;
    if (g_spt && *g_spt)
      json_add_object_item(body, "spt", json_create_string(g_spt));
    if (g_spt_list) {
      json_t* arr = json_create_item(JSON_ARRAY);
      if (arr) {
        for (int i = 0; g_spt_list[i]; i++)
          json_add_array_item(arr, json_create_string(g_spt_list[i]));
        json_add_object_item(body, "sptList", arr);
      }
    }
  } else {
    api_url = API_SEND_MESSAGE;
    if (g_app_token && *g_app_token)
      json_add_object_item(body, "appToken",
                           json_create_string(g_app_token));
    if (g_uids) {
      json_t* arr = json_create_item(JSON_ARRAY);
      if (arr) {
        for (int i = 0; g_uids[i]; i++)
          json_add_array_item(arr, json_create_string(g_uids[i]));
        json_add_object_item(body, "uids", arr);
      }
    }
    if (g_topic_ids) {
      json_t* arr = json_create_item(JSON_ARRAY);
      if (arr) {
        for (int i = 1; i <= g_topic_ids[0]; i++)
          json_add_array_item(arr,
                              json_create_number(g_topic_ids[i]));
        json_add_object_item(body, "topicIds", arr);
      }
    }
  }

  char* json_str = json_print_buffered(body, 0, 0);
  if (!json_str) {
    LOG_E("failed to serialize json");
    json_free(body);
    vector_clear(&g_msg_buf);
    return;
  }

  http_req_t req;
  http_request_create(&req, api_url);
  http_request_content_type(&req, "application/json");
  http_res_t resp = http_post(&req, json_str);

  if (resp.code >= 200 && resp.code < 300) {
    LOG_I("push success, code=%u", resp.code);
  } else {
    LOG_E("push failed, code=%u, payload=%.*s",
          resp.code, (int)resp.payload_len, (char*)resp.payload);
  }

  http_response_dispose(&resp);
  http_request_dispose(&req);
  os_mem_free(json_str);
  json_free(body);

  g_dbm_cnt_valid = false;
  vector_clear(&g_msg_buf);
}

const push_backend_t wxpusher_backend = {
  .name = "wxpusher",
  .min_interval_ms = 500,
  .init = wxpusher_init,
  .dispose = wxpusher_dispose,
  .load_config = wxpusher_load_config,
  .add_task = wxpusher_add_task,
  .submit = wxpusher_submit,
};
