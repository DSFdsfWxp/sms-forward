
#pragma once

#include <stdint.h>
#include "core/sms.h"

typedef struct push_task_t {
  enum {
    push_type_msgs,
    push_type_alert_smsbox_almost_full
  } type;
  sms_record_t* records;
} push_task_t;

typedef struct push_backend_t {
  const char* name;
  uint32_t min_interval_ms;
  void (*init)();
  void (*dispose)();
  void (*load_config)();
  /** @note task need to be freed after use */
  void (*add_task)(push_task_t* task);
  /** @brief finally push the message */
  void (*submit)();
} push_backend_t;

extern const push_backend_t* const push_backends[];

void pushs_dispose_task(push_task_t* task);
