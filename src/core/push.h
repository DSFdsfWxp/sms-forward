
#pragma once

#include "core/sms.h"

void push_init();
void push_load_config();
void push_submit_msgs(sms_record_t* records);
void push_alert_smsbox_almost_full();
