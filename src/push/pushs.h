
#pragma once

#include "core/sms.h"

typedef struct push_backend_t {
  const char* name;
  void (*init)();
  void (*dispose)();
  void (*load_config)();
  void (*submit_msgs)(sms_record_t* records);
  void (*alert_smsbox_almost_full)();
} push_backend_t;

extern const push_backend_t* const push_backends[];
