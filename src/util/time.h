
#pragma once

#include <stdint.h>

/**
 * @brief Get the current monotonic time in milliseconds.
 *        Based on clock_gettime(CLOCK_MONOTONIC).
 * @return Monotonic time in ms, suitable for interval measurement.
 */
int64_t time_get_ms_tick();

/**
 * @brief Sleep for the specified number of milliseconds.
 * @param ms_ticks  Milliseconds to sleep (busy-waits in sub-ms remainder).
 */
void time_sleep(int64_t ms_ticks);
