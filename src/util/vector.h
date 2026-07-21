
#pragma once

#include <stddef.h>

/**
 * @brief Generic dynamic array (vector) container.
 *        After each push/push_n/resize, the element at position len
 *        is zeroed, providing automatic null termination for char vectors.
 */
typedef struct {
  void* data;       /**< Pointer to element storage */
  size_t len;       /**< Number of elements currently held */
  size_t cap;       /**< Allocated capacity (in elements) */
  size_t item_size; /**< Size of each element in bytes */
} vector_t;

/**
 * @brief Initialize a vector with the given element size.
 * @param v          Pointer to vector_t (uninitialised).
 * @param item_size  Size of each element in bytes.
 */
void vector_create(vector_t* v, size_t item_size);

/**
 * @brief Free the vector's internal storage and reset to empty.
 *        Does NOT free the vector_t struct itself.
 */
void vector_dispose(vector_t* v);

/**
 * @brief Append one element at the end. Grows capacity by doubling.
 * @return 0 on success, -1 on allocation failure.
 */
int vector_push(vector_t* v, const void* elem);

/**
 * @brief Append n elements at the end.
 * @return 0 on success, -1 on allocation failure.
 */
int vector_push_n(vector_t* v, const void* elems, size_t n);

/**
 * @brief Resize the vector. If new_len > cap, storage is grown.
 *        New space is zero-initialized. After resize the null guard
 *        at position new_len is zeroed.
 * @return 0 on success, -1 on allocation failure.
 */
int vector_resize(vector_t* v, size_t new_len);

/** @return Number of elements currently held. */
size_t vector_len(const vector_t* v);

/** @return Pointer to the underlying element array. */
void* vector_data(const vector_t* v);

void* vector_get(const vector_t* v, size_t i);

void* vector_begin(const vector_t* v);
void* vector_end(const vector_t* v);

/**
 * @brief Reset length to zero without freeing storage.
 *        The element at index 0 is zeroed (null guard).
 */
void vector_clear(vector_t* v);
