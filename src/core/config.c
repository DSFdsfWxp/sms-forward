
#include <stdbool.h>
#include <pthread.h>
#include <limits.h>

#include "core/sms.h"
#include "core/push.h"
#include "util/setting.h"
#include "config.h"

#define LOG_TAG "core.config"
#include "util/log.h"

#define CONFIG_FILE_PATH "/home/root/sms/config.json"

static void config_load() {
  sms_load_config();
  push_load_config();
}

static void* config_reload_thread(void* arg) {
  while (true) {
    if (!setting_wait_reload_signal())
      break;
    LOG_I("reloading config");
    if (setting_reload())
      config_load();
    else
      LOG_E("failed to reload config");
  }
  return NULL;
}

void config_init() {
  bool res = setting_open(CONFIG_FILE_PATH);
  if (!res)
    LOG_W("failed to open config");
  config_load();

  pthread_t thread;
  pthread_attr_t attr;
  int ret = pthread_attr_init(&attr);
  ret = pthread_attr_setstacksize(&attr, 16384);  // 16kb
  if (ret != 0) {
    LOG_E("failed to set thread stack size, code: %d", ret);
    return;
  }
  ret = pthread_create(&thread, &attr, config_reload_thread, NULL);
  if (ret != 0) {
    LOG_E("failed to create thread, code: %d", ret);
    return;
  }
  LOG_I("config inited");
}
