
#include "api/ipc/sms.h"
#include "api/ipc/dbm.h"
#include "api/frwk.h"
#include "api/json.h"
#include "sms.h"

#define LOG_TAG "core.sms"
#include "util/log.h"

void sms_init() {
  const char* msg_name = mg_enum_to_str_apps_message_id_e_type(
      IPC_SMS_MSG_ID_SMS_NOTIFY_MSG_COUNT);
  frwk_ipc_msg_sub(msg_name);
  LOG_I("sms inited");
}

int sms_ipc_handler(int msg_type_id, const void* input, int input_len,
                    void* output, int output_len) {
  if (msg_type_id == IPC_SMS_MSG_ID_SMS_NOTIFY_MSG_COUNT) {
    LOG_I("checking new message");
  }
  return 0;
}
