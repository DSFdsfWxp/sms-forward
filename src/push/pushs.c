
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stddef.h>

#include "util/string.h"
#include "pushs.h"

extern const push_backend_t wxpusher_backend;
extern const push_backend_t route_backend;
extern const push_backend_t smtp_backend;

const push_backend_t* const push_backends[] = {
  &wxpusher_backend,
  &route_backend,
  &smtp_backend,
  NULL
};

void pushs_dispose_task(push_task_t* task) {
  if (task->records)
    sms_records_free(task->records);
}

static char* resolve_impl(const char* var, void* ctx, bool html_escape) {
  pushs_resolve_ctx_t* c = (pushs_resolve_ctx_t*)ctx;
  const sms_record_t* record = c ? c->record : NULL;
  const dbm_get_msg_count_res_t* cnt = c ? c->dbm_cnt : NULL;
  bool cnt_valid = c ? c->dbm_cnt_valid : false;
  char tmp[64];

  if (strcmp(var, "phone") == 0) {
    const char* v = (record && record->phone) ? record->phone : "";
    return html_escape ? str_escape(v) : strdup(v);
  }
  if (strcmp(var, "contacts") == 0) {
    const char* v = (record && record->contacts) ? record->contacts : "";
    return html_escape ? str_escape(v) : strdup(v);
  }
  if (strcmp(var, "content") == 0) {
    const char* v = (record && record->content) ? record->content : "";
    return html_escape ? str_escape(v) : strdup(v);
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
    uint32_t v = cnt_valid ? cnt->inbox_count : 0;
    snprintf(tmp, sizeof(tmp), "%" PRIu32, v);
    return strdup(tmp);
  }
  if (strcmp(var, "unread_count") == 0) {
    uint32_t v = cnt_valid ? cnt->unread_count : 0;
    snprintf(tmp, sizeof(tmp), "%" PRIu32, v);
    return strdup(tmp);
  }
  if (strcmp(var, "outbox_count") == 0) {
    uint32_t v = cnt_valid ? cnt->outbox_count : 0;
    snprintf(tmp, sizeof(tmp), "%" PRIu32, v);
    return strdup(tmp);
  }
  if (strcmp(var, "draft_count") == 0) {
    uint32_t v = cnt_valid ? cnt->draft_count : 0;
    snprintf(tmp, sizeof(tmp), "%" PRIu32, v);
    return strdup(tmp);
  }
  if (strcmp(var, "total_count") == 0) {
    uint32_t v =
        cnt_valid ? cnt->draft_count + cnt->inbox_count + cnt->outbox_count : 0;
    snprintf(tmp, sizeof(tmp), "%" PRIu32, v);
    return strdup(tmp);
  }
  if (strcmp(var, "max_count") == 0) {
    uint32_t v = cnt_valid ? cnt->max_count : 0;
    snprintf(tmp, sizeof(tmp), "%" PRIu32, v);
    return strdup(tmp);
  }

  return NULL;
}

char* pushs_resolve_var(const char* var, void* ctx) {
  return resolve_impl(var, ctx, false);
}

char* pushs_resolve_var_html(const char* var, void* ctx) {
  return resolve_impl(var, ctx, true);
}
