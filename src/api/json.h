
#pragma once

#include <stdint.h>

/**
 * @brief JSON Node Type Enumeration
 */
typedef enum {
  JSON_FALSE = 0,  /**< Boolean False */
  JSON_TRUE = 1,   /**< Boolean True */
  JSON_NULL = 2,   /**< Null value */
  JSON_NUMBER = 3, /**< Number (Integer or Double) */
  JSON_STRING = 4, /**< String */
  JSON_ARRAY = 5,  /**< Array container */
  JSON_OBJECT = 6  /**< Object container (Key-Value pairs) */
} json_type_e;

/**
 * @brief Core JSON Node Structure
 * @note Total struct size is 48 bytes (on 32-bit ARM).
 */
typedef struct json_t {
  struct json_t* next; /**< Offset 0: Next node in linked list */
  struct json_t* prev; /**< Offset 4: Previous node in linked list */
  struct json_t* child_next; /**< Offset 8: Head of child list (for Arrays/Objects) */
  struct json_t* child_prev; /**< Offset 12: Head of child list (for Arrays/Objects) */
  int type;          /**< Offset 16: Node type (from json_type_e) */
  char* value_string; /**< Offset 20: String value (if type == JSON_STRING) */
  int64_t value_int;      /**< Offset 24: Integer or Boolean value */
  double value_double; /**< Offset 32: Floating point value */
  char* name;       /**< Offset 40: Node key name (if child of an Object) */
} json_t;



/**
 * @brief Parse a standard JSON string into a JSON node tree.
 * @param value Null-terminated JSON string.
 * @return Pointer to the root JSON node, or NULL on failure.
 */
json_t* json_parse(const char* value);

/**
 * @brief Parse a JSON string with advanced options.
 * @param value JSON string buffer.
 * @param return_parse_end Pointer to store the position where parsing ended.
 * @param require_null_terminated 1 to require null termination, 0 otherwise.
 * @return Pointer to the root JSON node, or NULL on failure.
 */
json_t* json_parse_with_opts(const unsigned char* value,
                             const char** return_parse_end,
                             int require_null_terminated);

/**
 * @brief Serialize a JSON tree into a string using a buffered approach.
 * @param item Root node to serialize.
 * @param prebuffer Initial buffer size to allocate.
 * @param fmt 1 to format (pretty-print) the output, 0 for raw output.
 * @return Dynamically allocated string containing the serialized JSON.
 */
char* json_print_buffered(json_t* item, int prebuffer, int fmt);

/**
 * @brief Minify a JSON string in-place (removes whitespaces and comments).
 * @param json_string The target JSON string to minify.
 * @return Pointer to the modified string.
 */
unsigned char* json_minify(unsigned char* json_string);

/**
 * @brief Retrieve the global error pointer when parsing fails.
 * @return Pointer to the string location where the syntax error occurred.
 */
const char* json_get_error(void);



/**
 * @brief Recursively free a JSON node and all its children.
 * @param item The target JSON node.
 */
void json_free(json_t* item);

/**
 * @brief Duplicate a JSON node.
 * @param item The JSON node to duplicate.
 * @param recurse 1 to deep-copy all child nodes, 0 to copy only the current node.
 * @return Pointer to the newly allocated duplicated node.
 */
json_t* json_dup(json_t* item, int recurse);



/**
 * @brief Get the number of child elements in an Array or Object.
 * @param array_or_object Target Array or Object node.
 * @return The number of children.
 */
int json_get_child_num(json_t* array_or_object);

/**
 * @brief Retrieve an item from an Array by index.
 * @param array Target Array node.
 * @param index Zero-based index.
 * @return Pointer to the child node, or NULL if out of bounds.
 */
json_t* json_get_array_item(json_t* array, int index);

/**
 * @brief Retrieve an item from an Object by its key.
 * @param object Target Object node.
 * @param string Key name to search for (case-insensitive).
 * @return Pointer to the child node, or NULL if not found.
 */
json_t* json_get_object_item(json_t* object, const char* string);



/**
 * @brief Append a node to the end of an Array.
 * @param array Target Array node.
 * @param item Node to append.
 */
void json_add_array_item(json_t* array, json_t* item);

/**
 * @brief Remove a node from an Array by index (does NOT free its memory).
 * @param array Target Array node.
 * @param index Index of the item to remove.
 * @return Pointer to the detached node.
 */
json_t* json_detach_array_item(json_t* array, int index);

/**
 * @brief Remove and free a node from an Array by index.
 * @param array Target Array node.
 * @param index Index of the item to delete.
 */
void json_delete_array_item(json_t* array, int index);

/**
 * @brief Insert a node into an Array at the specified index.
 * @param array Target Array node.
 * @param index Target index.
 * @param item Node to insert.
 */
void json_insert_array_item(json_t* array, int index, json_t* item);

/**
 * @brief Replace an existing node in an Array (the old node is freed).
 * @param array Target Array node.
 * @param index Target index.
 * @param item The replacement node.
 */
void json_replace_array_item(json_t* array, int index, json_t* item);



/**
 * @brief Add a key-value pair to an Object.
 * @param object Target Object node.
 * @param string The key name (will be duplicated internally).
 * @param item The value node to add.
 */
void json_add_object_item(json_t* object, const char* string, json_t* item);

/**
 * @brief Remove a node from an Object by key (does NOT free its memory).
 * @param object Target Object node.
 * @param string The key name.
 * @return Pointer to the detached node.
 */
json_t* json_detach_object_item(json_t* object, const char* string);

/**
 * @brief Remove and free a node from an Object by key.
 * @param object Target Object node.
 * @param string The key name.
 */
void json_delete_object_item(json_t* object, const char* string);

/**
 * @brief Replace an existing node in an Object (the old node is freed).
 * @param object Target Object node.
 * @param string The key name.
 * @param item The replacement node.
 */
void json_replace_object_item(json_t* object, const char* string, json_t* item);



/**
 * @brief Base allocation for a node of a specific type.
 * @param type Node type (from json_type_e).
 * @return Pointer to the new empty node.
 */
json_t* json_create_item(int type);

/** @brief Create a Boolean Node (0 = False, >0 = True) */
json_t* json_create_bool(int b);

/** @brief Create a Number Node */
json_t* json_create_number(double num);

/** @brief Create a String Node */
json_t* json_create_string(const char* string);

/** @brief Create an Array filled with Integer nodes */
json_t* json_create_array_int(const int* numbers, int count);

/** @brief Create an Array filled with Float nodes */
json_t* json_create_array_float(const float* numbers, int count);

/** @brief Create an Array filled with Double nodes */
json_t* json_create_array_double(const double* numbers, int count);

/** @brief Create an Array filled with String nodes */
json_t* json_create_array_string(const char** strings, int count);
