
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#include "util/json.h"
#include "setting.h"

#define LOG_TAG "util.setting"
#include "util/log.h"

static json_t* setting_json = NULL;
static const char* setting_path = NULL;
static sigset_t setting_reload_mask;

static bool setting_load() {
  if (!setting_path)
    return false;
  FILE* fp = fopen(setting_path, "rb");
  if (!fp) {
    LOG_E("setting file not existed: %s", setting_path);
    return false;
  }

  bool res = false;

  fseek(fp, 0, SEEK_END);
  size_t size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  char* txt = malloc(size);
  if (!txt) {
    LOG_E("no more mem");
    goto CLEAN;
  }

  setting_json = json_parse(txt);
  if (!setting_json) {
    LOG_E("failed to parse setting: %s", setting_path);
    goto CLEAN;
  }
  res = true;
CLEAN:
  if (txt)
    free(txt);
  fclose(fp);

  return res;
}

bool setting_open(const char* path) {
  sigemptyset(&setting_reload_mask);
  sigaddset(&setting_reload_mask, SIGUSR1);
  int ret = pthread_sigmask(SIG_BLOCK, &setting_reload_mask, NULL);
  if (ret != 0) {
    LOG_E("failed to set signal mask");
    perror("pthread_sigmask");
    return false;
  }

  setting_path = path;
  return setting_load();
}

bool setting_reload() {
  json_t* json_bak = setting_json;
  setting_json = NULL;
  bool res = setting_load();
  if (res) {
    if (json_bak)
      json_free(json_bak);
  } else {
    LOG_E("failed to reload");
    if (setting_json)
      json_free(setting_json);
    setting_json = json_bak;
  }
  return res;
}

bool setting_wait_reload_signal() {
  if (!setting_path)
    return false;
  int signo;
  while (true) {
    int err = sigwait(&setting_reload_mask, &signo);
    if (err != 0)
      continue;
    if (signo == SIGUSR1)
      break;
  }
  return true;
}

const json_t* setting_get_raw(const char* key) {
  if (!setting_json)
    return NULL;
  const json_t* res = json_get_object_item(setting_json, key);
  if (!res)
    res = json_get_by_path(setting_json, key);
  return res;
}

char* setting_get_str(const char* key, const char* def) {
  const json_t* item = setting_get_raw(key);
  if (!json_item_check_type(item, JSON_STRING))
    return strdup(def);
  return strdup(item->value_string);
}

uint64_t setting_get_int(const char* key, uint64_t def) {
  const json_t* item = setting_get_raw(key);
  if (!json_item_check_type(item, JSON_NUMBER))
    return def;
  return item->value_int;
}

double setting_get_float(const char* key, double def) {
  const json_t* item = setting_get_raw(key);
  if (!json_item_check_type(item, JSON_NUMBER))
    return def;
  return item->value_double;
}

bool setting_get_bool(const char* key, bool def) {
  const json_t* item = setting_get_raw(key);
  if (!json_item_check_type(item, JSON_TRUE) &&
      !json_item_check_type(item, JSON_FALSE))
    return def;
  return item->type == JSON_TRUE;
}
