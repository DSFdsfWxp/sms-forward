
#pragma once

#include <curl/curl.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief HTTP request handle.
 *        Created with http_request_create, destroyed with http_request_dispose.
 *        Headers are accumulated via http_request_header / http_request_content_type.
 */
typedef struct http_req_t {
  CURL* curl;
  struct curl_slist* headers;
} http_req_t;

/**
 * @brief HTTP response.
 *        code == 0 indicates a transport/CURL error (payload may still be set
 *        with a partial response). Successful responses have 2xx/3xx codes.
 *        Call http_response_dispose to free the payload.
 */
typedef struct http_res_t {
  uint16_t code;        /**< HTTP status code, or 0 on CURL error */
  void* payload;        /**< malloc'd null-terminated response body */
  size_t payload_len;   /**< Length of payload in bytes */
} http_res_t;

/**
 * @brief Create an HTTP request handle for the given URL.
 *        curl_global_init is lazily called on first invocation.
 * @param req  Pointer to http_req_t (uninitialised).
 * @param url  Target URL (copied internally by curl).
 */
void http_request_create(http_req_t* req, const char* url);

/** @brief Destroy the request handle and free all associated resources. */
void http_request_dispose(http_req_t* req);

/**
 * @brief Add a custom HTTP header.
 * @param req    Request handle.
 * @param key    Header name, e.g. "Authorization".
 * @param value  Header value, e.g. "Bearer token".
 */
void http_request_header(http_req_t* req, const char* key, const char* value);

/**
 * @brief Convenience: set Content-Type header.
 * @param req   Request handle.
 * @param type  MIME type, e.g. "application/json".
 */
void http_request_content_type(http_req_t* req, const char* type);

/** @brief Perform a GET request. Returns response (caller must dispose). */
http_res_t http_get(http_req_t* req);

/**
 * @brief Perform a POST request with a null-terminated string body.
 * @param req   Request handle.
 * @param body  NULL-terminated string body.
 * @return Response (caller must dispose).
 */
http_res_t http_post(http_req_t* req, const char* body);

/**
 * @brief Perform a POST request with a binary body.
 * @param req       Request handle.
 * @param body      Binary data.
 * @param body_len  Length of binary data.
 * @return Response (caller must dispose).
 */
http_res_t http_post_binary(http_req_t* req, const void* body, size_t body_len);

/** @brief Free the response payload. Safe to call on zeroed-initialised http_res_t. */
void http_response_dispose(http_res_t* resp);
