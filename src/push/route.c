
#define _GNU_SOURCE
#include <inttypes.h>
#include <regex.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "api/base.h"
#include "util/json.h"

#define LOG_TAG "push.route"
#include "util/log.h"

#include "util/setting.h"

#include "core/sms.h"
#include "pushs.h"

/* Maximum number of compiled regex patterns */
#define ROUTE_MAX_REGEX 32

static const push_backend_t* g_matched_backend = NULL;
static const push_backend_t* g_unmatched_backend = NULL;

static char** g_keywords = NULL;
static regex_t g_regex[ROUTE_MAX_REGEX];
static int g_regex_count = 0;
static char* g_match_field = NULL;

/* ------------------------------------------------------------------ */
/* Forward declaration so load_config can update min_interval_ms       */
/* ------------------------------------------------------------------ */
extern push_backend_t route_backend;

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

/**
 * @brief Look up a backend by name in the global push_backends registry.
 * @return Pointer to the backend, or NULL if not found.
 */
static const push_backend_t* find_backend(const char* name) {
  if (!name || !*name)
    return NULL;
  const push_backend_t* const* p = push_backends;
  while (*p) {
    if (strcmp((*p)->name, name) == 0)
      return *p;
    p++;
  }
  return NULL;
}

/**
 * @brief Check whether text contains any of the configured keywords
 *        (case-insensitive substring match).
 */
static bool match_keyword(const char* text) {
  if (!g_keywords || !text)
    return false;
  for (int i = 0; g_keywords[i]; i++) {
    if (strcasestr(text, g_keywords[i]))
      return true;
  }
  return false;
}

/**
 * @brief Check whether text matches any of the compiled regex patterns.
 */
static bool match_regex(const char* text) {
  if (!text || g_regex_count == 0)
    return false;
  for (int i = 0; i < g_regex_count; i++) {
    if (regexec(&g_regex[i], text, 0, NULL, 0) == 0)
      return true;
  }
  return false;
}

/**
 * @brief Determine which field of a record to match against.
 */
static const char* get_match_text(const sms_record_t* r) {
  if (g_match_field && strcmp(g_match_field, "phone") == 0)
    return r->phone;
  if (g_match_field && strcmp(g_match_field, "contacts") == 0)
    return r->contacts;
  /* default: "content" */
  return r->content;
}

/**
 * @brief Feed a single sms_record_t to a backend as a synthetic task.
 */
static void feed_record(const push_backend_t* backend,
                        const sms_record_t* record) {
  if (!backend || !backend->add_task)
    return;

  sms_record_t arr[2];
  arr[0] = *record;
  memset(&arr[1], 0, sizeof(sms_record_t));

  push_task_t task = {.type = push_type_msgs, .records = arr};
  backend->add_task(&task);
}

/* ------------------------------------------------------------------ */
/* Lifecycle                                                           */
/* ------------------------------------------------------------------ */

static void route_init() {
  LOG_I("route backend inited");
}

static void route_dispose() {
  if (g_matched_backend && g_matched_backend->dispose)
    g_matched_backend->dispose();
  if (g_unmatched_backend && g_unmatched_backend->dispose)
    g_unmatched_backend->dispose();

  g_matched_backend = NULL;
  g_unmatched_backend = NULL;

  free(g_match_field);
  g_match_field = NULL;

  if (g_keywords) {
    for (int i = 0; g_keywords[i]; i++)
      free(g_keywords[i]);
    free(g_keywords);
    g_keywords = NULL;
  }

  for (int i = 0; i < g_regex_count; i++)
    regfree(&g_regex[i]);
  g_regex_count = 0;
}

static void route_load_config() {
  /* Dispose previous state */
  route_dispose();

  /* Read target backend names */
  char* matched_name = setting_get_str("push.route.matchedBackend", "");
  char* unmatched_name = setting_get_str("push.route.unmatchedBackend", "");

  g_matched_backend = find_backend(matched_name);
  g_unmatched_backend = find_backend(unmatched_name);

  if (!g_matched_backend)
    LOG_W("matchedBackend '%s' not found in registry", matched_name);
  if (!g_unmatched_backend)
    LOG_W("unmatchedBackend '%s' not found in registry", unmatched_name);

  /* Init sub-backends */
  if (g_matched_backend && g_matched_backend->init)
    g_matched_backend->init();
  if (g_unmatched_backend && g_unmatched_backend->init)
    g_unmatched_backend->init();

  /* Load config for sub-backends */
  if (g_matched_backend && g_matched_backend->load_config)
    g_matched_backend->load_config();
  if (g_unmatched_backend && g_unmatched_backend->load_config)
    g_unmatched_backend->load_config();

  /* Dynamically set min_interval_ms = max(matched, unmatched) */
  uint32_t matched_interval =
      g_matched_backend ? g_matched_backend->min_interval_ms : 0;
  uint32_t unmatched_interval =
      g_unmatched_backend ? g_unmatched_backend->min_interval_ms : 0;
  route_backend.min_interval_ms = matched_interval > unmatched_interval
                                      ? matched_interval
                                      : unmatched_interval;

  /* Read route-specific config */
  g_match_field = setting_get_str("push.route.matchField", "content");

  g_keywords = json_load_str_array(
      (json_t*)setting_get_raw("push.route.keywords"));

  /* Compile regex patterns */
  char** patterns = json_load_str_array(
      (json_t*)setting_get_raw("push.route.regexPatterns"));
  if (patterns) {
    for (int i = 0; patterns[i] && g_regex_count < ROUTE_MAX_REGEX; i++) {
      int rc = regcomp(&g_regex[g_regex_count], patterns[i],
                       REG_EXTENDED | REG_NOSUB);
      if (rc != 0) {
        char errbuf[128];
        regerror(rc, &g_regex[g_regex_count], errbuf, sizeof(errbuf));
        LOG_W("regex '%s' compile failed: %s", patterns[i], errbuf);
      } else {
        g_regex_count++;
      }
      free(patterns[i]);
    }
    free(patterns);
  }

  int kw_count = 0;
  if (g_keywords)
    while (g_keywords[kw_count]) kw_count++;

  LOG_I("matchedBackend=%s unmatchedBackend=%s matchField=%s keywords=%d regex=%d"
        " minIntervalMs=%" PRIu32,
        matched_name, unmatched_name, g_match_field,
        kw_count, g_regex_count, route_backend.min_interval_ms);

  free(matched_name);
  free(unmatched_name);
}

static void route_add_task(push_task_t* task) {
  if (task->type == push_type_msgs && task->records) {
    for (sms_record_t* r = task->records; r->phone || r->content; r++) {
      const char* text = get_match_text(r);
      bool matched = match_keyword(text) || match_regex(text);

      if (matched)
        feed_record(g_matched_backend, r);
      else
        feed_record(g_unmatched_backend, r);
    }
  } else if (task->type == push_type_alert_smsbox_almost_full) {
    /* Alert tasks go to both backends */
    if (g_matched_backend && g_matched_backend->add_task)
      g_matched_backend->add_task(task);
    if (g_unmatched_backend && g_unmatched_backend->add_task)
      g_unmatched_backend->add_task(task);
  }
}

static void route_submit() {
  if (g_matched_backend && g_matched_backend->submit)
    g_matched_backend->submit();
  if (g_unmatched_backend && g_unmatched_backend->submit)
    g_unmatched_backend->submit();
}

push_backend_t route_backend = {
    .name = "route",
    .min_interval_ms = 0,
    .init = route_init,
    .dispose = route_dispose,
    .load_config = route_load_config,
    .add_task = route_add_task,
    .submit = route_submit,
};
