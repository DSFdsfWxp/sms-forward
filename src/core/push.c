
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

#include "push/pushs.h"
#include "util/setting.h"
#include "util/queue.h"
#include "push.h"

#define LOG_TAG "core.push"
#include "util/log.h"

typedef struct push_task_t {
  enum {
    push_type_msgs,
    push_type_alert_smsbox_almost_full
  } type;
  sms_record_t* records;
} push_task_t;

static const push_backend_t* push_active_backend = NULL;
static queue_t push_queue;
static pthread_mutex_t push_lock;

void *push_thread(void* arg) {
  push_task_t task;
  while (true) {
    queue_wait_and_pop(&push_queue, &task);
    pthread_mutex_lock(&push_lock);
    if (push_active_backend) {
      if (task.type == push_type_msgs && task.records) {
        LOG_I("pushing msgs");
        if (push_active_backend->submit_msgs)
          push_active_backend->submit_msgs(task.records);
      } else if (task.type == push_type_alert_smsbox_almost_full) {
        LOG_I("pushing smsbox almost full alarm");
        if (push_active_backend->alert_smsbox_almost_full)
          push_active_backend->alert_smsbox_almost_full();
      }
      LOG_I("push task done");
    } else {
      LOG_W("no active backend, ignoring push task");
    }
    pthread_mutex_unlock(&push_lock);
    if (task.records)
      sms_records_free(task.records);
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

void push_load_config() {
  pthread_mutex_lock(&push_lock);
  char* name = setting_get_str("push.backendName", "");
  if (!push_active_backend || strcmp(name, push_active_backend->name) != 0) {
    const push_backend_t* const* backend = push_backends;
    while (backend) {
      if (!strcmp((*backend)->name, name))
        break;
      backend ++;
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
  free(name);
  pthread_mutex_unlock(&push_lock);
}

void push_submit_msgs(sms_record_t* records) {
  push_task_t task = {.type = push_type_msgs, .records = records};
  queue_push(&push_queue, &task);
}

void push_alert_smsbox_almost_full() {
  push_task_t task = {.type = push_type_alert_smsbox_almost_full, .records = NULL};
  queue_push(&push_queue, &task);
}
