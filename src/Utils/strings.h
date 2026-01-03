#include "Array.h"
#include <stdlib.h>
#include <string.h>

i32 starts_with(const char *str, const char *prefix) {
  u32 i = 0;
  while (*prefix != '\0') {
    if (*(prefix++) != *(str++)) {
      return -1;
    }
    i++;
  }
  return i;
}

Array split_str(const char *str, const char *delim) {
  Array result = new_array(0, sizeof(char *));

  const char *start = str;
  const char *curr = str;
  size_t delim_len = strlen(delim);

  while (*curr) {
    if (starts_with(curr, delim) >= 0) {
      size_t len = curr - start;

      char *token = (char *)malloc(len + 1);
      memcpy(token, start, len);
      token[len] = '\0';

      array_push(&result, &token);

      curr += delim_len;
      start = curr;
    } else {
      curr++;
    }
  }

  // push last token
  if (curr != start) {
    size_t len = curr - start;
    char *token = (char *)malloc(len + 1);
    memcpy(token, start, len);
    token[len] = '\0';
    array_push(&result, &token);
  }

  return result;
}
