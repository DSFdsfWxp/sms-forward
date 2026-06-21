
#include "queue.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct queue_page_t {
  struct queue_page_t* next;
  void* buff;
};

void queue_create(queue_t* obj, uint32_t item_size) {
  obj->head = obj->tail = NULL;
  obj->free = NULL;
  obj->r_ptr = obj->w_ptr = 0;
  obj->item_size = item_size;
  pthread_mutex_init(&obj->lock, NULL);
  sem_init(&obj->item_sem, 0, 0);
}

static queue_page_t* queue_create_page(queue_t* obj) {
  queue_page_t* page = obj->free;
  if (page)
    obj->free = NULL;
  else
    page = malloc(sizeof(queue_page_t) + obj->item_size * 8);

  if (page) {
    page->next = NULL;
    page->buff = (uint8_t*)page + sizeof(queue_page_t);
  }
  return page;
}

void queue_push(queue_t* obj, const void* item) {
  pthread_mutex_lock(&obj->lock);

  if (!obj->tail) {
    obj->head = obj->tail = queue_create_page(obj);
    if (!obj->tail)
      abort();
  } else if (obj->w_ptr == 8) {
    obj->tail->next = queue_create_page(obj);
    if (!obj->tail->next)
      abort();
    obj->tail = obj->tail->next;
    obj->w_ptr = 0;
  }

  void* ptr = (uint8_t*)obj->tail->buff + obj->item_size * obj->w_ptr;
  memcpy(ptr, item, obj->item_size);
  obj->w_ptr++;

  pthread_mutex_unlock(&obj->lock);
  sem_post(&obj->item_sem);
}

void queue_wait_and_pop(queue_t* obj, void* item) {
  sem_wait(&obj->item_sem);
  pthread_mutex_lock(&obj->lock);

  void* ptr = (uint8_t*)obj->head->buff + obj->item_size * obj->r_ptr;
  memcpy(item, ptr, obj->item_size);
  obj->r_ptr++;

  if (obj->r_ptr == 8) {
    queue_page_t* head = obj->head;
    obj->head = obj->head->next;
    obj->r_ptr = 0;

    if (obj->tail == head) {
      obj->tail = NULL;
      obj->w_ptr = 0;
    }

    if (obj->free)
      free(head);
    else
      obj->free = head;
  }

  pthread_mutex_unlock(&obj->lock);
}
