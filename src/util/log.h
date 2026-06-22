
#pragma once

/**
 * @brief Thread-safe logging facility.
 *
 * Usage — define LOG_TAG before including this header:
 * @code
 *   #define LOG_TAG "my.module"
 *   #include "util/log.h"
 * @endcode
 *
 * If LOG_TAG is not defined, a compile-time warning is issued.
 */

#ifndef LOG_TAG
# warning "LOG_TAG is not set"
# define LOG_TAG "unknown"
#endif

/** @brief Set to 0 to strip all LOG_D calls at compile time. */
#define LOG_DEBUG_ENABLED 0

/**
 * @def LOG_I
 * Log an info message. Format: [I][LOG_TAG] message
 * @def LOG_W
 * Log a warning message.
 * @def LOG_E
 * Log an error message.
 * @def LOG_D
 * Log a debug message (compiled out when LOG_DEBUG_ENABLED == 0).
 */
#define LOG_I(fmt, ...) log_raw('I', LOG_TAG, fmt, ##__VA_ARGS__)
#define LOG_W(fmt, ...) log_raw('W', LOG_TAG, fmt, ##__VA_ARGS__)
#define LOG_E(fmt, ...) log_raw('E', LOG_TAG, fmt, ##__VA_ARGS__)

#if LOG_DEBUG_ENABLED
# define LOG_D(fmt, ...) log_raw('D', LOG_TAG, fmt, ##__VA_ARGS__)
#else
# define LOG_D(fmt, ...)
#endif

/**
 * @brief Initialise the logger.
 */
void log_init();

/**
 * @brief Low-level logging function; prefer the LOG_I / LOG_W / etc. macros.
 * @param level  Single character level ('I', 'W', 'E', 'D').
 * @param tag    Module tag string.
 * @param fmt    printf-style format string.
 * @param ...    Format arguments.
 */
void log_raw(char level, const char* tag, const char *fmt, ...);
