
#pragma once

#include "core/sms.h"

typedef struct push_backend_t {
  void (*init)();
  void (*dispose)();
  void (*submit_msgs)(sms_record_t* records);
  void (*alert_smsbox_almost_full)();
} push_backend_t;

extern push_backend_t *push_backends[];
