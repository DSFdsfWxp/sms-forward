
#pragma once

#include <stdint.h>

#include "api/ipc/dbm.h"
#include "core/sms.h"

/**
 * @brief Task dispatched by the push engine to the active backend.
 *
 * Two task types exist:
 *   - push_type_msgs: carries sms records for forwarding.
 *   - push_type_alert_smsbox_almost_full: storage alert, msg_count is valid.
 *
 * Memory ownership: the engine owns the task; backends must deep-copy
 * records if they need them after add_task returns.
 */
typedef struct push_task_t {
  enum {
    push_type_msgs,                      /**< SMS record forwarding */
    push_type_alert_smsbox_almost_full   /**< SMS storage almost full alert */
  } type;
  sms_record_t* records;                 /**< NULL-terminated array (msgs only) */
  dbm_get_msg_count_res_t msg_count;     /**< DBM stats (alert only) */
} push_task_t;

/**
 * @brief Pluggable push backend interface.
 *
 * Lifecycle:
 *   init() → load_config() → [add_task() ...] → submit() → dispose()
 *
 * The push engine calls add_task() for each task in a batch, then
 * calls submit() once. At least max(push.minPushIntervalMs,
 * min_interval_ms) ms elapses before the next batch.
 */
typedef struct push_backend_t {
  const char* name;           /**< Matches push.backendName in config */
  uint32_t min_interval_ms;   /**< Minimum interval between submit() calls */
  void (*init)();             /**< Initialize backend state */
  void (*dispose)();          /**< Free all backend resources */
  void (*load_config)();      /**< Reload config from global setting */
  /** @brief Handle a single task. Deep-copy if storing for async submit. */
  void (*add_task)(push_task_t* task);
  /** @brief Send all buffered tasks. Engine calls this once per batch. */
  void (*submit)();
} push_backend_t;

/** @brief NULL-terminated registry of all compiled-in backends. */
extern const push_backend_t* const push_backends[];

/**
 * @brief Free the records owned by a task.
 *        Called by the engine when there is no active backend.
 *        NOT called when a backend is active (records remain caller-owned).
 */
void pushs_dispose_task(push_task_t* task);
