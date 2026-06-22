

#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>

#define LOG_TAG "util.log"
#include "log.h"

static pthread_mutex_t log_lock;

void log_init() {
  setlinebuf(stdout);
  pthread_mutex_init(&log_lock, NULL);
}

void log_raw(char level, const char* tag, const char* fmt, ...) {
  pthread_mutex_lock(&log_lock);
  va_list lst;
  va_start(lst, fmt);
  printf("[%c][%s] ", level, tag);
  vprintf(fmt, lst);
  va_end(lst);
  putc('\n', stdout);
  pthread_mutex_unlock(&log_lock);
}
