
#pragma once

#include <curl/curl.h>
#include <stddef.h>
#include <stdint.h>

typedef struct http_req_t {
  CURL* curl;
  struct curl_slist* headers;
} http_req_t;

typedef struct http_res_t {
  /** 0: error, other: http status code */
  uint16_t code;
  void* payload;
  size_t payload_len;
} http_res_t;

void http_request_create(http_req_t* req, const char* url);
void http_request_dispose(http_req_t* req);

void http_request_header(http_req_t* req, const char* key, const char* value);
void http_request_body_content_type(http_req_t* req, const char* type);

http_res_t http_get(http_req_t* req);
http_res_t http_post(http_req_t* req, const char* body);
http_res_t http_post_binary(http_req_t* req, const void* body, size_t body_len);

void http_response_dispose(http_res_t* resp);
