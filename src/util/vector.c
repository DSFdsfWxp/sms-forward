
#include <stdlib.h>
#include <string.h>

#include "vector.h"

void vector_create(vector_t* v, size_t elem_size) {
  v->data = NULL;
  v->len = 0;
  v->cap = 0;
  v->item_size = elem_size;
}

void vector_dispose(vector_t* v) {
  free(v->data);
  v->data = NULL;
  v->len = 0;
  v->cap = 0;
}

static int vector_grow(vector_t* v, size_t needed) {
  if (needed <= v->cap)
    return 0;

  size_t new_cap = v->cap ? v->cap : 8;
  while (new_cap < needed)
    new_cap *= 2;

  void* new_data = realloc(v->data, new_cap * v->item_size);
  if (!new_data)
    return -1;

  v->data = new_data;
  v->cap = new_cap;
  return 0;
}

int vector_push(vector_t* v, const void* elem) {
  size_t needed = v->len + 1;
  if (vector_grow(v, needed) != 0)
    return -1;

  memcpy((char*)v->data + v->len * v->item_size, elem, v->item_size);
  v->len++;
  memset((char*)v->data + v->len * v->item_size, 0, v->item_size);
  return 0;
}

int vector_push_n(vector_t* v, const void* elems, size_t n) {
  if (n == 0)
    return 0;

  size_t needed = v->len + n;
  if (vector_grow(v, needed) != 0)
    return -1;

  memcpy((char*)v->data + v->len * v->item_size, elems, n * v->item_size);
  v->len += n;
  memset((char*)v->data + v->len * v->item_size, 0, v->item_size);
  return 0;
}

int vector_resize(vector_t* v, size_t new_len) {
  if (new_len > v->cap) {
    if (vector_grow(v, new_len) != 0)
      return -1;
  }
  v->len = new_len;
  memset((char*)v->data + v->len * v->item_size, 0, v->item_size);
  return 0;
}

size_t vector_len(const vector_t* v) {
  return v->len;
}

void* vector_data(const vector_t* v) {
  return v->data;
}

void* vector_get(const vector_t* v, size_t i) {
  if (i >= v->len)
    return NULL;
  return (char*)v->data + i * v->item_size;
}

void* vector_begin(const vector_t* v) {
  return v->data;
}

void* vector_end(const vector_t* v) {
  return (char*)v->data + v->len * v->item_size;
}

void vector_clear(vector_t* v) {
  v->len = 0;
  if (v->data)
    memset(v->data, 0, v->item_size);
}
