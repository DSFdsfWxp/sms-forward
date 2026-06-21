
#include <time.h>
#include <errno.h>

#include "time.h"

int64_t time_get_ms_tick() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000;
}

void time_sleep(int64_t ms_ticks) {
  if (ms_ticks == 0)
    return;
  struct timespec ts;
  ts.tv_sec = ms_ticks / 1000;
  ts.tv_nsec = (ms_ticks % 1000) * 1000000L;
  while (clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, &ts) == -1 &&
         errno == EINTR) {
    continue;
  }
}
