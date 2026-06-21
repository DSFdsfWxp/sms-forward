
#include "push.h"

#define LOG_TAG "core.push"
#include "util/log.h"

void push_init() {
  LOG_I("push inited");
}

void push_submit_msgs(sms_record_t* records) {
  sms_records_free(records);
}

void push_alert_smsbox_almost_full() {
  LOG_D("alert smsbox almost full");
}
