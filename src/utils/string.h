#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

char *format_string(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int size = vsnprintf(NULL, 0, fmt, args);
  va_end(args);

  char *buffer = (char *)malloc(size + 1);
  if (!buffer) {
    return NULL;
  }

  va_start(args, fmt);
  vsnprintf(buffer, size + 1, fmt, args);
  va_end(args);

  return buffer;
}