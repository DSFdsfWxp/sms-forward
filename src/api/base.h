
#pragma once

#include <stddef.h>
#include <stdint.h>


typedef struct {
  uint32_t capacity;
  uint32_t length;
  uint8_t* data;
} mg_buffer_t;

int os_debug_enable();
int os_debug_disable();

int os_mem_free(void* buff);

void *mg_bf_init(mg_buffer_t* buf, int initial_size);
void mg_bf_free(mg_buffer_t* buf);

