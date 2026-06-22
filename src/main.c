
#include "api/ipc/apptest.h"
#include "api/base.h"
#include "api/frwk.h"
#include "core/sms.h"
#include "core/push.h"
#include "core/config.h"
#include "util/setting.h"

#define LOG_TAG "main"
#include "util/log.h"

int main() {
  log_init();
  LOG_I(" # sms-forward");
  LOG_I(" # 1.0.0");
  LOG_I("");
  LOG_I("bootstrap");

  frwk_module_init(IPC_MODULE_APPTEST, NULL);
  os_debug_disable();

  if (!setting_init()) {
    LOG_E("failed to init setting");
    return 1;
  }

  sms_init();
  push_init();
  config_init();
  sms_check_new_msg();

  frwk_ipc_service(sms_ipc_handler);
  return 0;
}
