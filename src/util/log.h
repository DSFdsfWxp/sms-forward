
#pragma once

#ifndef LOG_TAG
# warning "LOG_TAG is not set"
# define LOG_TAG "unknown"
#endif

#define LOG_I(fmt, ...) log_raw('I', LOG_TAG, fmt, ##__VA_ARGS__)
#define LOG_W(fmt, ...) log_raw('W', LOG_TAG, fmt, ##__VA_ARGS__)
#define LOG_E(fmt, ...) log_raw('E', LOG_TAG, fmt, ##__VA_ARGS__)

void log_raw(char level, const char* tag, const char *fmt, ...);
