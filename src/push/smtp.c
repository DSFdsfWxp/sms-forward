#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "util/json.h"
#include "util/smtp.h"
#include "util/string.h"
#include "util/vector.h"

#define LOG_TAG "push.smtp"
#include "util/log.h"

#include "util/setting.h"

#include "core/sms.h"
#include "pushs.h"

/* ------------------------------------------------------------------ */
/* Forward declaration so load_config can update min_interval_ms       */
/* ------------------------------------------------------------------ */
extern push_backend_t smtp_backend;

/* ------------------------------------------------------------------ */
/* State                                                               */
/* ------------------------------------------------------------------ */

static smtp_config_t g_smtp_cfg;
static smtp_session_t g_smtp_sess;
static char** g_to_list;
static int g_content_type;

static char* g_subject_msg;
static char* g_template_msg;
static char* g_template_alert;
static char* g_separator;

static vector_t g_msg_buf;
static dbm_get_msg_count_res_t g_dbm_cnt;
static bool g_dbm_cnt_valid;

/* Saved first-record fields for subject template resolution */
static char* g_first_contacts;
static char* g_first_phone;

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static void free_state() {
  smtp_config_dispose(&g_smtp_cfg);

  if (g_to_list) {
    for (int i = 0; g_to_list[i]; i++)
      free(g_to_list[i]);
    free(g_to_list);
    g_to_list = NULL;
  }

  free(g_subject_msg);
  free(g_template_msg);
  free(g_template_alert);
  free(g_separator);

  g_subject_msg = NULL;
  g_template_msg = NULL;
  g_template_alert = NULL;
  g_separator = NULL;

  vector_dispose(&g_msg_buf);
  g_dbm_cnt_valid = false;

  free(g_first_contacts);
  free(g_first_phone);
  g_first_contacts = NULL;
  g_first_phone = NULL;
}

/* ------------------------------------------------------------------ */
/* Lifecycle                                                           */
/* ------------------------------------------------------------------ */

static void smtp_init() {
  smtp_config_init(&g_smtp_cfg);
  smtp_session_init(&g_smtp_sess);
  vector_create(&g_msg_buf, 1);
  LOG_I("smtp backend inited");
}

static void smtp_dispose() {
  free_state();
  smtp_session_dispose(&g_smtp_sess);
  LOG_I("smtp backend disposed");
}

static void smtp_load_config() {
  free_state();

  smtp_config_init(&g_smtp_cfg);
  vector_create(&g_msg_buf, 1);

  /* SMTP server settings */
  g_smtp_cfg.server = setting_get_str("push.smtp.server", "");
  g_smtp_cfg.port = (uint16_t)setting_get_int("push.smtp.port", 465);
  g_smtp_cfg.username = setting_get_str("push.smtp.username", "");
  g_smtp_cfg.password = setting_get_str("push.smtp.password", "");
  g_smtp_cfg.from = setting_get_str("push.smtp.from", "");
  g_smtp_cfg.timeout_sec = (int)setting_get_int("push.smtp.timeout", 30);
  g_content_type = (int)setting_get_int("push.smtp.contentType", 2);
  g_smtp_cfg.content_type = g_content_type;

  /* Recipients */
  g_to_list = json_load_str_array(
      (json_t*)setting_get_raw("push.smtp.to"));

  /* Min interval (configurable) */
  smtp_backend.min_interval_ms =
      (uint32_t)setting_get_int("push.smtp.minIntervalMs", 5000);

  /* Templates */
  g_subject_msg = setting_get_str(
      "push.smtp.subject", "[SMS] {contacts}");
  g_template_msg = setting_get_str(
      "push.smtp.template_msg",
      "<h2>新短信</h2>"
      "<pre style=\"font-size:14px;white-space:pre-wrap\">{content}</pre>"
      "<hr><table>"
      "<tr><td><b>发件人</b></td><td>{contacts}</td></tr>"
      "<tr><td><b>号码</b></td><td>{phone}</td></tr>"
      "<tr><td><b>时间</b></td><td>{datetime}</td></tr>"
      "</table>");
  g_template_alert = setting_get_str(
      "push.smtp.template_alert",
      "<h2>短信即将存满</h2>"
      "<p>已用: <b>{total_count}</b> / <b>{max_count}</b></p>");
  g_separator = setting_get_str(
      "push.smtp.separator", "<hr>");

  int to_count = 0;
  if (g_to_list)
    while (g_to_list[to_count]) to_count++;

  LOG_I("server=%s:%u from=%s contentType=%d to_count=%d minIntervalMs=%" PRIu32,
        g_smtp_cfg.server, g_smtp_cfg.port, g_smtp_cfg.from,
        g_content_type, to_count, smtp_backend.min_interval_ms);
}

static void smtp_add_task(push_task_t* task) {
  str_template_resolver resolver =
      (g_content_type == 1) ? pushs_resolve_var : pushs_resolve_var_html;
  if (task->type == push_type_msgs && task->records) {
    for (sms_record_t* r = task->records; r->phone || r->content; r++) {
      /* Save first record's fields for subject template */
      if (!g_first_contacts && r->contacts && *r->contacts)
        g_first_contacts = strdup(r->contacts);
      if (!g_first_phone && r->phone && *r->phone)
        g_first_phone = strdup(r->phone);

      pushs_resolve_ctx_t rctx = {
          .record = r,
          .dbm_cnt = &g_dbm_cnt,
          .dbm_cnt_valid = g_dbm_cnt_valid};
      char* text = str_template(g_template_msg, resolver, &rctx);
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
    pushs_resolve_ctx_t rctx = {
        .dbm_cnt = &g_dbm_cnt,
        .dbm_cnt_valid = true};
    char* text = str_template(g_template_alert, resolver, &rctx);
    if (!text)
      return;

    if (vector_len(&g_msg_buf) > 0 && g_separator)
      vector_push_n(&g_msg_buf, g_separator, strlen(g_separator));
    vector_push_n(&g_msg_buf, text, strlen(text));
    free(text);
  }
}

static void smtp_submit() {
  if (vector_len(&g_msg_buf) == 0)
    return;

  if (!g_smtp_cfg.server || !*g_smtp_cfg.server) {
    LOG_W("SMTP server not configured, dropping %zu bytes",
          vector_len(&g_msg_buf));
    vector_clear(&g_msg_buf);
    g_dbm_cnt_valid = false;
    free(g_first_contacts);
    free(g_first_phone);
    g_first_contacts = NULL;
    g_first_phone = NULL;
    return;
  }

  if (!g_to_list || !g_to_list[0]) {
    LOG_W("no recipients configured, dropping %zu bytes",
          vector_len(&g_msg_buf));
    vector_clear(&g_msg_buf);
    g_dbm_cnt_valid = false;
    free(g_first_contacts);
    free(g_first_phone);
    g_first_contacts = NULL;
    g_first_phone = NULL;
    return;
  }

  char* buf = (char*)vector_data(&g_msg_buf);

  /* Build subject — use first record's contacts/phone if available */
  sms_record_t first_rec = {
      .contacts = g_first_contacts,
      .phone = g_first_phone};
  pushs_resolve_ctx_t rctx = {
      .record = (g_first_contacts || g_first_phone) ? &first_rec : NULL,
      .dbm_cnt = &g_dbm_cnt,
      .dbm_cnt_valid = g_dbm_cnt_valid};
  char* subject = str_template(g_subject_msg, pushs_resolve_var, &rctx);

  bool ok = smtp_send(&g_smtp_cfg, &g_smtp_sess,
                      (const char**)g_to_list,
                      subject ? subject : "", buf);

  if (ok)
    LOG_I("SMTP push success");
  else
    LOG_E("SMTP push failed");

  free(subject);
  vector_clear(&g_msg_buf);
  g_dbm_cnt_valid = false;
  free(g_first_contacts);
  free(g_first_phone);
  g_first_contacts = NULL;
  g_first_phone = NULL;
}

push_backend_t smtp_backend = {
    .name = "smtp",
    .min_interval_ms = 5000,
    .init = smtp_init,
    .dispose = smtp_dispose,
    .load_config = smtp_load_config,
    .add_task = smtp_add_task,
    .submit = smtp_submit,
};
