
#pragma once

#ifndef LOG_TAG
# warning "LOG_TAG is not set"
# define LOG_TAG "unknown"
#endif

#define LOG_DEBUG_ENABLED  1

#define LOG_I(fmt, ...) log_raw('I', LOG_TAG, fmt, ##__VA_ARGS__)
#define LOG_W(fmt, ...) log_raw('W', LOG_TAG, fmt, ##__VA_ARGS__)
#define LOG_E(fmt, ...) log_raw('E', LOG_TAG, fmt, ##__VA_ARGS__)

#if LOG_DEBUG_ENABLED
# define LOG_D(fmt, ...) log_raw('D', LOG_TAG, fmt, ##__VA_ARGS__)
#else
#define LOG_D(fmt, ...)
#endif

void log_init();
void log_raw(char level, const char* tag, const char *fmt, ...);
