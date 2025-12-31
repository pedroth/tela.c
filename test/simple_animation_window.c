#include "Color/Color.h"
#include "Tela/Tela.h"
#include "Tela/Window.h"
#include "utils/time.h"
#include "utils/types.h"
#include "utils/loop.h"
#include "utils/string.h"

const u32 WIDTH = 640;
const u32 HEIGHT = 480;

Color *shader(u32 x, u32 y, const void *context) {
  f32 time = *(f32 *)context;
  f32 fx = fmodf(((f32)x * time) / (f32)(WIDTH), 1.0f);
  f32 fy = fmodf(((f32)y * time) / (f32)(HEIGHT), 1.0f);
  return new_color(fx, fy, 0.0f);
}

typedef struct {
  Tela *tela;
  Window *window;
} AnimeCtx;

void animation_loop(f32 dt, f32 time, void *context) {
  AnimeCtx *ctx = (AnimeCtx *)context;
  map_tela(ctx->tela, shader, &time);
  set_window_title(ctx->window, format_string("FPS: %.2f", 1.0f / dt));
  paint_window(ctx->window, ctx->tela);
}

void on_close_lambda(Window *window, void *context) {
  Loop *anime_loop = (Loop *)context;
  stop_loop(anime_loop);
}

int main() {
  Tela *tela = new_tela(WIDTH, HEIGHT);
  Window *window = new_window(WIDTH, HEIGHT, "Simple Animation");
  Loop *anime_loop = loop(animation_loop, &(AnimeCtx){tela, window});
  on_close_window(window, on_close_lambda, anime_loop);
  play_loop(anime_loop);
  return 0;
}
