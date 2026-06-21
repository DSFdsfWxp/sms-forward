#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http.h"

#define LOG_TAG "util.http"
#include "util/log.h"

static bool http_inited = false;

void http_request_create(http_req_t* req, const char* url) {
  if (!http_inited) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    http_inited = true;
  }
  req->curl = curl_easy_init();
  if (!req->curl) {
    LOG_E("failed to init curl");
    return;
  }
  curl_easy_setopt(req->curl, CURLOPT_URL, url);
  req->headers = NULL;
}

void http_request_dispose(http_req_t* req) {
  if (req->headers)
    curl_slist_free_all(req->headers);
  if (req->curl)
    curl_easy_cleanup(req->curl);
  req->headers = NULL;
  req->curl = NULL;
}

void http_request_header(http_req_t* req, const char* key, const char* value) {
  if (!req->curl)
    return;
  size_t len = strlen(key) + strlen(value) + 4;
  char* txt = malloc(len);
  if (!txt) {
    LOG_E("failed to set header: %s", key);
    return;
  }
  snprintf(txt, len, "%s: %s", key, value);
  req->headers = curl_slist_append(req->headers, txt);
  free(txt);
}

void http_request_content_type(http_req_t* req, const char* type) {
  http_request_header(req, "Content-Type", type);
}

static size_t http_request_write_callback(void* contents, size_t size,
                                          size_t nmemb, void* userp) {
  size_t realsize = size * nmemb;
  http_res_t* resp = (http_res_t*)userp;
  void* ptr = realloc(resp->payload, resp->payload_len + realsize + 1);
  if (!ptr)
    return 0;
  resp->payload = ptr;
  memcpy((char*)resp->payload + resp->payload_len, contents, realsize);
  resp->payload_len += realsize;
  ((char*)resp->payload)[resp->payload_len] = '\0';
  return realsize;
}

static http_res_t http_request_send(http_req_t* req) {
  http_res_t resp = {0};
  resp.payload = NULL;
  resp.payload_len = 0;

  if (!req || !req->curl) {
    LOG_E("invalid request");
    return resp;
  }

  if (req->headers)
    curl_easy_setopt(req->curl, CURLOPT_HTTPHEADER, req->headers);

  curl_easy_setopt(req->curl, CURLOPT_WRITEFUNCTION, http_request_write_callback);
  curl_easy_setopt(req->curl, CURLOPT_WRITEDATA, &resp);

  CURLcode res = curl_easy_perform(req->curl);
  if (res != CURLE_OK) {
    LOG_E("curl_easy_perform failed: %s", curl_easy_strerror(res));
    if (resp.payload)
      free(resp.payload);
    resp.payload = NULL;
    resp.payload_len = 0;
    return resp;
  }

  long http_code = 0;
  curl_easy_getinfo(req->curl, CURLINFO_RESPONSE_CODE, &http_code);
  resp.code = (uint16_t)http_code;

  return resp;
}

http_res_t http_get(http_req_t* req) {
  if (!req || !req->curl) {
    http_res_t empty = {0};
    return empty;
  }
  curl_easy_setopt(req->curl, CURLOPT_HTTPGET, 1L);
  curl_easy_setopt(req->curl, CURLOPT_POST, 0L);
  curl_easy_setopt(req->curl, CURLOPT_POSTFIELDS, NULL);
  curl_easy_setopt(req->curl, CURLOPT_POSTFIELDSIZE, 0L);
  return http_request_send(req);
}

http_res_t http_post(http_req_t* req, const char* body) {
  if (!req || !req->curl) {
    http_res_t empty = {0};
    return empty;
  }
  if (!body)
    body = "";
  curl_easy_setopt(req->curl, CURLOPT_POST, 1L);
  curl_easy_setopt(req->curl, CURLOPT_POSTFIELDS, body);
  curl_easy_setopt(req->curl, CURLOPT_POSTFIELDSIZE, (long)strlen(body));
  return http_request_send(req);
}

http_res_t http_post_binary(http_req_t* req, const void* body,
                            size_t body_len) {
  if (!req || !req->curl) {
    http_res_t empty = {0};
    return empty;
  }
  curl_easy_setopt(req->curl, CURLOPT_POST, 1L);
  if (body && body_len > 0) {
    curl_easy_setopt(req->curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(req->curl, CURLOPT_POSTFIELDSIZE, (long)body_len);
  } else {
    curl_easy_setopt(req->curl, CURLOPT_POSTFIELDS, NULL);
    curl_easy_setopt(req->curl, CURLOPT_POSTFIELDSIZE, 0L);
  }
  return http_request_send(req);
}

void http_response_dispose(http_res_t* resp) {
  if (resp && resp->payload) {
    free(resp->payload);
    resp->payload = NULL;
    resp->payload_len = 0;
  }
}
