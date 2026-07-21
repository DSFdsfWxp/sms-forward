
#pragma once

#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct queue_page_t queue_page_t;

/**
 * @brief Fixed-element-size, dynamically-growing, thread-safe queue.
 *        Implemented as a linked list of pages (8 items per page).
 *        Producer-consumer semantics via semaphore.
 */
typedef struct {
  queue_page_t *head, *tail;
  queue_page_t* free;
  uint8_t r_ptr, w_ptr;
  uint32_t item_size;
  pthread_mutex_t lock;
  sem_t item_sem;
} queue_t;

/**
 * @brief Create a queue with the given element size.
 * @param obj        Pointer to queue_t (uninitialised).
 * @param item_size  Size of each queue element in bytes.
 */
void queue_create(queue_t* obj, uint32_t item_size);

void queue_dispose(queue_t* obj);

    /**
 * @brief Push an item onto the tail of the queue (non-blocking).
 *        Grows the page list if needed. Thread-safe.
 * @param obj   Queue handle.
 * @param item  Pointer to the item data to copy into the queue.
 */
    void queue_push(queue_t* obj, const void* item);

/**
 * @brief Pop an item from the head of the queue (blocking).
 *        Blocks if the queue is empty until an item is available.
 * @param obj   Queue handle.
 * @param item  Buffer to receive the popped item data.
 */
void queue_wait_and_pop(queue_t* obj, void* item);

/**
 * @brief Get the current number of items in the queue.
 * @return Number of items (may be stale immediately after return
 *         due to concurrent push/pop; use only as a hint).
 */
uint32_t queue_get_size(queue_t* obj);
