
#include <stddef.h>
#include "pushs.h"

extern const push_backend_t wxpusher_backend;
extern const push_backend_t smartroute_backend;

const push_backend_t* const push_backends[] = {
  &wxpusher_backend,
  &smartroute_backend,
  NULL
};

void pushs_dispose_task(push_task_t* task) {
  if (task->records)
    sms_records_free(task->records);
}
