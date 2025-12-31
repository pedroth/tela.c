#include "types.h"
#include "utils/time.h"
#include <stdlib.h>

typedef struct {
  void (*update)(f32, f32, void *context);
  void *context;
  bool running;
} Loop;

void play_loop(Loop *loop) {
  u32 old_time = get_time_ms();
  f32 time = 0.0f;
  while (loop->running) {
    f32 dt = (get_time_ms() - old_time) / 1000.0f;
    old_time = get_time_ms();
    time += dt;
    loop->update(dt, time, loop->context);
  }
}

void stop_loop(Loop *loop) { loop->running = false; }

Loop *loop(void (*update)(f32, f32, void *ctx), void *context) {
    Loop *new_loop = (Loop *)malloc(sizeof(Loop));
    new_loop->update = update;
    new_loop->context = context;
    new_loop->running = true;
    return new_loop;
}
