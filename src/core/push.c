
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>

#include "api/frwk.h"
#include "api/ipc/dbm.h"
#include "push/pushs.h"
#include "util/setting.h"
#include "util/queue.h"
#include "util/time.h"
#include "push.h"

#define LOG_TAG "core.push"
#include "util/log.h"

static const push_backend_t* push_active_backend = NULL;
static int64_t push_min_interval_ms;
static queue_t push_queue;
static pthread_mutex_t push_lock;

static void push_post_task(push_task_t* task) {
  if (push_active_backend && push_active_backend->add_task)
    push_active_backend->add_task(task);
  else
    LOG_W("backend ignored task");
}

static void *push_thread(void* arg) {
  push_task_t task;
  int64_t interval = 0;
  while (true) {
    queue_wait_and_pop(&push_queue, &task);
    pthread_mutex_lock(&push_lock);
    if (push_active_backend) {
      push_post_task(&task);
      uint32_t num = queue_get_size(&push_queue);
      for (uint32_t i = 0; i < num; i++) {
        queue_wait_and_pop(&push_queue, &task);
        push_post_task(&task);
      }
      if (push_active_backend->submit)
        push_active_backend->submit();
      LOG_I("%u push task(s) done", num + 1u);
    } else {
      pushs_dispose_task(&task);
      LOG_W("no active backend, ignoring push task");
    }
    interval = push_min_interval_ms;
    pthread_mutex_unlock(&push_lock);

    if (push_active_backend) {
      int64_t sleep_ms = interval > push_active_backend->min_interval_ms
                             ? interval
                             : push_active_backend->min_interval_ms;
      time_sleep(sleep_ms);
    }
  }
}

void push_init() {
  pthread_mutex_init(&push_lock, NULL);
  queue_create(&push_queue, sizeof(push_task_t));

  pthread_t thread;
  pthread_attr_t attr;
  int ret = pthread_attr_init(&attr);
  ret = pthread_attr_setstacksize(&attr, 16384);  // 16kb
  if (ret != 0) {
    LOG_E("failed to set thread stack size, code: %d", ret);
    return;
  }
  ret = pthread_create(&thread, &attr, push_thread, NULL);
  if (ret != 0) {
    LOG_E("failed to create thread, code: %d", ret);
    return;
  }
  LOG_I("push inited");
}

static void push_switch_backend(const char* name) {
  if (!push_active_backend || strcmp(name, push_active_backend->name) != 0) {
    const push_backend_t* const* backend = push_backends;
    while (*backend) {
      if (!strcmp((*backend)->name, name))
        break;
      backend++;
    }
    if (push_active_backend) {
      LOG_I("disposing backend: %s", push_active_backend->name);
      if (push_active_backend->dispose)
        push_active_backend->dispose();
    }
    push_active_backend = *backend;
    if (push_active_backend) {
      LOG_I("initing backend: %s", push_active_backend->name);
      if (push_active_backend->init)
        push_active_backend->init();
    } else {
      LOG_W("no active backend");
    }
  }
  if (push_active_backend && push_active_backend->load_config) {
    LOG_I("loading config for backend: %s", push_active_backend->name);
    push_active_backend->load_config();
  }
}

void push_load_config() {
  pthread_mutex_lock(&push_lock);
  char* name = setting_get_str("push.backendName", "");
  push_min_interval_ms = setting_get_int("push.minPushIntervalMs", 0);
  push_switch_backend(name);
  free(name);
  pthread_mutex_unlock(&push_lock);
}

void push_submit_msgs(sms_record_t* records) {
  push_task_t task = {.type = push_type_msgs, .records = records};
  queue_push(&push_queue, &task);
}

void push_alert_smsbox_almost_full() {
  push_task_t task;
  memset(&task, 0, sizeof(task));
  task.type = push_type_alert_smsbox_almost_full;

  int res0 = -1;
  int res1 = frwk_ipc_send_sync(IPC_MODULE_DBM,
    IPC_DBM_MSG_ID_GET_MSG_COUNT, NULL, 0,
    &task.msg_count, sizeof(task.msg_count), 10, &res0);
  if (res0 != FRWK_OK || res1 != FRWK_OK)
    memset(&task.msg_count, 0, sizeof(task.msg_count));

  queue_push(&push_queue, &task);
}
