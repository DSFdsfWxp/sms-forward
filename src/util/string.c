
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "string.h"

char* str_template(const char* tmpl, str_template_resolver resolve, void* ctx) {
  if (!tmpl || !*tmpl)
    return strdup("");

  size_t cap = strlen(tmpl) * 2 + 256;
  char* result = malloc(cap);
  if (!result)
    return NULL;

  size_t pos = 0;

  while (*tmpl) {
    if (*tmpl == '{' && *(tmpl + 1) == '{') {
      if (pos + 1 >= cap) {
        cap = pos + 256;
        char* new_r = realloc(result, cap);
        if (!new_r) {
          free(result);
          return NULL;
        }
        result = new_r;
      }
      result[pos++] = '{';
      tmpl += 2;
      continue;
    }

    if (*tmpl == '}') {
      if (*(tmpl + 1) == '}') {
        if (pos + 1 >= cap) {
          cap = pos + 256;
          char* new_r = realloc(result, cap);
          if (!new_r) {
            free(result);
            return NULL;
          }
          result = new_r;
        }
        result[pos++] = '}';
        tmpl += 2;
        continue;
      }
      if (pos + 1 >= cap) {
        cap = pos + 256;
        char* new_r = realloc(result, cap);
        if (!new_r) {
          free(result);
          return NULL;
        }
        result = new_r;
      }
      result[pos++] = '}';
      tmpl++;
      continue;
    }

    if (*tmpl == '{') {
      const char* end = strchr(tmpl + 1, '}');
      if (!end) {
        if (pos + 1 >= cap) {
          cap = pos + 256;
          char* new_r = realloc(result, cap);
          if (!new_r) {
            free(result);
            return NULL;
          }
          result = new_r;
        }
        result[pos++] = '{';
        tmpl++;
        continue;
      }

      size_t var_len = end - tmpl - 1;
      char var_name[64];
      size_t cp_len = var_len < sizeof(var_name) - 1
                          ? var_len
                          : sizeof(var_name) - 1;
      memcpy(var_name, tmpl + 1, cp_len);
      var_name[cp_len] = '\0';

      char* value = resolve(var_name, ctx);
      if (value) {
        size_t vlen = strlen(value);
        if (pos + vlen >= cap) {
          cap = pos + vlen + 256;
          char* new_r = realloc(result, cap);
          if (!new_r) {
            free(result);
            free(value);
            return NULL;
          }
          result = new_r;
        }
        memcpy(result + pos, value, vlen);
        pos += vlen;
        free(value);
      } else {
        size_t vlen = end - tmpl + 1;
        if (pos + vlen >= cap) {
          cap = pos + vlen + 256;
          char* new_r = realloc(result, cap);
          if (!new_r) {
            free(result);
            return NULL;
          }
          result = new_r;
        }
        memcpy(result + pos, tmpl, vlen);
        pos += vlen;
      }

      tmpl = end + 1;
      continue;
    }

    if (pos + 1 >= cap) {
      cap = pos + 256;
      char* new_r = realloc(result, cap);
      if (!new_r) {
        free(result);
        return NULL;
      }
      result = new_r;
    }
    result[pos++] = *tmpl;
    tmpl++;
  }

  result[pos] = '\0';
  return result;
}

char* str_escape(const char* str) {
  if (!str)
    return strdup("");

  size_t len = strlen(str);
  size_t cap = len * 6 + 1;
  char* out = malloc(cap);
  if (!out)
    return NULL;

  size_t pos = 0;
  for (size_t i = 0; str[i]; i++) {
    switch (str[i]) {
      case '&':
        pos += snprintf(out + pos, cap - pos, "&amp;");
        break;
      case '<':
        pos += snprintf(out + pos, cap - pos, "&lt;");
        break;
      case '>':
        pos += snprintf(out + pos, cap - pos, "&gt;");
        break;
      case '"':
        pos += snprintf(out + pos, cap - pos, "&quot;");
        break;
      default:
        out[pos++] = str[i];
        break;
    }
  }
  out[pos] = '\0';
  return out;
}
