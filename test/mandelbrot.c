/**
 * Mandelbrot Set Explorer
 *
 * Interactive Mandelbrot set visualization with pan (mouse drag) and
 * zoom (mouse wheel) controls. Ported from the tela.js demo.
 */

#include "../src/index.c"

// gcc -O3 -fopenmp -o app test/mandelbrot.c -lSDL2 -lm

/* =============================================================================
 * Constants
 * ========================================================================== */

static const u32 WIDTH = 640;
static const u32 HEIGHT = 480;

static const u32 MAX_ITERATIONS_SINGLE = 100;
static const u32 MAX_ITERATIONS_PARALLEL = 200;

/* =============================================================================
 * Types
 * ========================================================================== */

typedef struct {
  Tela *tela;
  Window *window;
  AABB_2D box;
  bool use_parallel;
} App;

/* =============================================================================
 * Global State (for input handling)
 * ========================================================================== */

static bool g_mouse_down = false;
static Vec2 g_mouse_pos = {0};

/* =============================================================================
 * Complex Multiplication
 * ========================================================================== */

static inline Vec2 complex_mul(Vec2 z, Vec2 w) {
  return vec2(z.x * w.x - z.y * w.y, z.x * w.y + z.y * w.x);
}

/* =============================================================================
 * Mandelbrot Renderer
 * ========================================================================== */

typedef struct {
  Vec2 size;
  AABB_2D box;
  u32 max_iterations;
} RenderContext;

static Color mandelbrot_pixel(u32 x, u32 y, void const *ctx) {
  const RenderContext *rc = (const RenderContext *)ctx;
  const Vec2 size = rc->size;
  const AABB_2D box = rc->box;

  // Normalize pixel to [-1, 1]
  Vec2 p = div_vec2(vec2((f32)x, (f32)y), size);
  p = vec2(2.0f * p.x - 1.0f, 2.0f * p.y - 1.0f);

  // Apply aspect ratio and center offset
  p = vec2(p.x * (size.x / size.y) - 0.5f, p.y);

  // Map to box coordinates: box.min + box.diagonal * ((p + 1) / 2)
  Vec2 offset = scale_vec2(add_vec2(p, vec2(1.0f, 1.0f)), 0.5f);
  p = add_vec2(box.min, mul_vec2(box.diagonal, offset));

  // Iterate z = z^2 + c
  Vec2 z = vec2(0.0f, 0.0f);
  for (u32 i = 0; i < rc->max_iterations; i++) {
    z = add_vec2(complex_mul(z, z), p);
  }

  f32 l = length_vec2(z);
  f32 ll = clamp(l, 0.0f, 1.0f);
  return (Color){1.0f - ll, ll, 0.0f, 1.0f};
}

static void render(App *app) {
  RenderContext rc = {
      .size = vec2((f32)WIDTH, (f32)HEIGHT),
      .box = app->box,
      .max_iterations = app->use_parallel ? MAX_ITERATIONS_PARALLEL
                                          : MAX_ITERATIONS_SINGLE,
  };

  if (app->use_parallel) {
    map_tela_parallel(app->tela, mandelbrot_pixel, &rc);
  } else {
    map_tela(app->tela, mandelbrot_pixel, &rc);
  }

  paint_window(app->window, app->tela);
}

/* =============================================================================
 * Animation Loop
 * ========================================================================== */

static void on_frame(f32 dt, f32 time, void *ctx) {
  App *app = (App *)ctx;
  render(app);
  set_window_title(app->window,
                   format_string("Mandelbrot Set | Parallel: %s (P) | FPS: %.1f",
                                 app->use_parallel ? "ON" : "OFF", 1.0f / dt));
}

static void on_close(Window *window, void *ctx) {
  Loop *animation = (Loop *)ctx;
  stop_loop(animation);
}

/* =============================================================================
 * Input Handlers
 * ========================================================================== */

static void on_mouse_down(Window *window, i32 x, i32 y, u32 button,
                          void *ctx) {
  g_mouse_down = true;
  g_mouse_pos = vec2((f32)x, (f32)y);
}

static void on_mouse_up(Window *window, i32 x, i32 y, u32 button, void *ctx) {
  g_mouse_down = false;
}

static void on_mouse_move(Window *window, i32 x, i32 y, void *ctx) {
  if (!g_mouse_down)
    return;

  Vec2 new_pos = vec2((f32)x, (f32)y);
  if (equals_vec2(new_pos, g_mouse_pos))
    return;

  App *app = (App *)ctx;
  Vec2 size = vec2((f32)WIDTH, (f32)HEIGHT);
  Vec2 delta = sub_vec2(new_pos, g_mouse_pos);

  // Pan: convert pixel delta to box-space displacement
  Vec2 v = mul_vec2(scale_vec2(delta, -1.0f), div_vec2(app->box.diagonal, size));
  Vec2 new_min = add_vec2(app->box.min, v);
  Vec2 new_max = add_vec2(app->box.max, v);
  app->box = build_aabb_2d(new_min, new_max);

  g_mouse_pos = new_pos;
}

static void on_mouse_scroll(Window *window, i32 delta_y, void *ctx) {
  App *app = (App *)ctx;

  f32 scale = 1.0f + (delta_y > 0 ? 1.0f : -1.0f) * 0.1f;

  // Scale box around its center
  Vec2 center = app->box.center;
  Vec2 half_diag = scale_vec2(app->box.diagonal, 0.5f * scale);
  app->box = build_aabb_2d(sub_vec2(center, half_diag), add_vec2(center, half_diag));
}

static void on_key_down(Window *window, u32 keycode, void *ctx) {
  App *app = (App *)ctx;
  if (keycode == SDLK_p) {
    app->use_parallel = !app->use_parallel;
  }
}

/* =============================================================================
 * Main
 * ========================================================================== */

int main(void) {
  Tela *tela = new_tela(WIDTH, HEIGHT);
  Window *window = new_window(WIDTH, HEIGHT, "Mandelbrot Set");

  App app = {
      .tela = tela,
      .window = window,
      .box = build_aabb_2d(vec2(-1.0f, -1.0f), vec2(1.0f, 1.0f)),
      .use_parallel = true,
  };

  Loop *animation = loop(on_frame, &app);

  on_close_window(window, on_close, animation);
  on_mouse_down_window(window, on_mouse_down, &app);
  on_mouse_up_window(window, on_mouse_up, &app);
  on_mouse_move_window(window, on_mouse_move, &app);
  on_mouse_scroll_window(window, on_mouse_scroll, &app);
  on_key_down_window(window, on_key_down, &app);

  play_loop(animation);

  free_loop(animation);
  free_tela(tela);

  return 0;
}
