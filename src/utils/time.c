
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include "time.h"
#include <time.h>

u32 get_time_ms(void) {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    return 0;

  u64 ms = (u64)ts.tv_sec * 1000ULL + (u64)ts.tv_nsec / 1000000ULL;
  return (u32)ms;
}
