#include "../src/index.c"

const u32 WIDTH = 640;
const u32 HEIGHT = 480;

typedef struct {
  Tela *tela;
  Window *window;
} AnimeContext;

f32 clamp(f32 x) { return fminf(fmaxf(0, x), 1); }
f32 *palette(f32 t) {
  static f32 a[3] = {0.5f, 0.5f, 0.5f};
  static f32 b[3] = {0.5f, 0.5f, 0.5f};
  static f32 c[3] = {1.0f, 1.0f, 1.0f};
  static f32 d[3] = {0.263f, 0.416f, 0.557f};
  return (f32[]){a[0] + b[0] * cosf(6.28318f * (c[0] * t + d[0])),
                 a[1] + b[1] * cosf(6.28318f * (c[1] * t + d[1])),
                 a[2] + b[2] * cosf(6.28318f * (c[2] * t + d[2]))};
}

Color shader(u32 x, u32 y, const void *context) {
  f32 time = *(const f32 *)context;
  f32 u = (2.0f * (f32)x - (f32)WIDTH) / (f32)HEIGHT;
  f32 v = (2.0f * (f32)y - (f32)HEIGHT) / (f32)HEIGHT;
  const f32 u0 = u;
  const f32 v0 = v;
  f32 finalColor[3] = {0.0f, 0.0f, 0.0f};

  f32 d0 = -sqrtf(u0 * u0 + v0 * v0); // compute once
  for (int i = 0; i < 4; i++) {
    u = (u * 1.5f - floorf(u * 1.5f)) - 0.5f;
    v = (v * 1.5f - floorf(v * 1.5f)) - 0.5f;
    f32 d = sqrtf(u * u + v * v) * expf(d0);
    f32 *col = palette(d0 + i * 0.4f + time * 0.4f);
    d = sinf(d * 8.0f + time) / 8.0f;
    d = fabsf(d);
    d = fmaxf(d, 1e-6f);                       // avoid div/0
    d = powf(0.01f / d, 1.2f);
    finalColor[0] += col[0] * d;
    finalColor[1] += col[1] * d;
    finalColor[2] += col[2] * d;
  }

  // clamp final color to [0,1]
  finalColor[0] = clamp(finalColor[0]);
  finalColor[1] = clamp(finalColor[1]);
  finalColor[2] = clamp(finalColor[2]);

  return (Color){finalColor[0], finalColor[1], finalColor[2], 1.0f};
}

void anime_lambda(f32 dt, f32 time, void *context) {
  AnimeContext *anime_context = (AnimeContext *)context;
  Tela *tela = map_tela(anime_context->tela, shader, &time);
  set_window_title(anime_context->window,
                   format_string("FPS: %.2f", 1.0f / dt));
  paint_window(anime_context->window, tela);
}

void on_close_lambda(Window *window, void *context) {
  Loop *anime_loop = (Loop *)context;
  stop_loop(anime_loop);
}

int main() {
  Tela *tela = new_tela(WIDTH, HEIGHT);
  Window *window = new_window(WIDTH, HEIGHT, "Amazing Shader");
  AnimeContext anime_context = {tela, window};
  Loop *anime_loop = loop(anime_lambda, &anime_context);
  on_close_window(window, on_close_lambda, anime_loop);
  // must be last function to be called in main
  play_loop(anime_loop);
  return 0;
}
