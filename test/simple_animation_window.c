#include "Color/Color.h"
#include "Tela/Tela.h"
#include "Tela/Window.h"
#include "utils/time.h"
#include "utils/types.h"

const u32 WIDTH = 640;
const u32 HEIGHT = 480;

Color *shader(u32 x, u32 y, const void *context) {
  f32 time = *(f32 *)context;
  f32 fx = fmodf(((f32)x * time) / (f32)(WIDTH), 1.0f);
  f32 fy = fmodf(((f32)y * time) / (f32)(HEIGHT), 1.0f);
  return new_color(fx, fy, 0.0f);
}

int main() {
  Tela *tela = new_tela(WIDTH, HEIGHT);
  Window *window = new_window(WIDTH * 4, HEIGHT * 4, "Simple Animation");
  u32 old_time = get_time_ms();
  f32 time = 0.0f;
  while (true) {
    f32 dt = (get_time_ms() - old_time) / 1000.0f;
    old_time = get_time_ms();
    time += dt;
    const void *context = &time;
    map_tela(tela, shader, context);
    char title[50];
    snprintf(title, sizeof(title), "FPS: %.2f", 1.0f / dt);
    set_window_title(window, title);
    paint_window(window, tela);
  }
  return 0;
}
