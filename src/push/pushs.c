
#include <stddef.h>
#include "pushs.h"

const push_backend_t* const push_backends[] = {
  NULL
};

void pushs_dispose_task(push_task_t* task) {
  if (task->records)
    sms_records_free(task->records);
}
