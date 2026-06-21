
#pragma once

#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct queue_page_t queue_page_t;

typedef struct {
  queue_page_t *head, *tail;
  queue_page_t* free;
  uint8_t r_ptr, w_ptr;
  uint32_t item_size;
  pthread_mutex_t lock;
  sem_t item_sem;
} queue_t;

void queue_create(queue_t* obj, uint32_t item_size);

void queue_push(queue_t* obj, const void* item);
void queue_wait_and_pop(queue_t* obj, void* item);
