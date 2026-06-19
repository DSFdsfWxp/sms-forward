

#include <stdarg.h>
#include <stdio.h>

#define LOG_TAG "util.log"
#include "log.h"

void log_raw(char level, const char* tag, const char* fmt, ...) {
    va_list lst;
    va_start(lst, fmt);
    printf("[%c][%s] ", level, tag);
    vprintf(fmt, lst);
    va_end(lst);
    putc('\n', stdout);
}
