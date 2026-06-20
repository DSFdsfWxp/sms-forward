
#include <stddef.h>

#include "api/ipc/apptest.h"
#include "api/base.h"
#include "api/frwk.h"
#include "core/sms.h"

#define LOG_TAG "main"
#include "util/log.h"

int main() {
    LOG_I(" # sms-forward");
    LOG_I("bootstrap");

    frwk_module_init(IPC_MODULE_APPTEST, NULL);
    os_debug_disable();

    sms_init();
    sms_check_new_msg();

    frwk_ipc_service(sms_ipc_handler);
    return 0;
}
