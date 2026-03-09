/**
 * Turing Pattern (Gray-Scott Reaction-Diffusion)
 *
 * A 2D reaction-diffusion simulation with interactive mouse drawing.
 * Uses Gray-Scott model on a toroidal grid.
 * Color encodes U concentration (red) and V concentration (blue).
 */

#include "../src/index.c"

/* =============================================================================
 * Constants
 * ========================================================================== */

static const u32 WIDTH = 640;
static const u32 HEIGHT = 480;

#define GRID_SIZE 200
#define AMPLITUDE 1.0f
#define DU 1.02f
#define DV 0.4f
#define FEED 0.046f
#define KILL 0.062f
#define DT 0.8f

/* =============================================================================
 * Utilities
 * ========================================================================== */

static u32 wrap(i32 n, i32 m) { return (u32)(((n % m) + m) % m); }

/* =============================================================================
 * Reaction-Diffusion State
 * ========================================================================== */

static f32 g_U[GRID_SIZE][GRID_SIZE];
static f32 g_V[GRID_SIZE][GRID_SIZE];

static void initialize_state(void) {
  for (u32 i = 0; i < GRID_SIZE; i++) {
    for (u32 j = 0; j < GRID_SIZE; j++) {
      g_U[i][j] = 0.0f;
      g_V[i][j] = 0.0f;
    }
  }
}

/* =============================================================================
 * Types
 * ========================================================================== */

typedef struct {
  Tela* tela;
  Window* window;
} App;

typedef struct {
  f32 min_U;
  f32 max_U;
  f32 min_V;
  f32 max_V;
} TuringShaderContext;

/* =============================================================================
 * Shader
 * ========================================================================== */

static Color turing_shader(u32 x, u32 y, const void* context) {
  TuringShaderContext* ctx = (TuringShaderContext*)context;

  u32 xi = (u32)((x / (f32)WIDTH) * GRID_SIZE);
  u32 yi = (u32)((y / (f32)HEIGHT) * GRID_SIZE);

  f32 range_U = ctx->max_U - ctx->min_U;
  f32 range_V = ctx->max_V - ctx->min_V;

  f32 red = range_U > 0.0f ? (g_U[yi][xi] - ctx->min_U) / range_U : 1.0f;
  f32 blue = range_V > 0.0f ? (g_V[yi][xi] - ctx->min_V) / range_V : 1.0f;

  return (Color){ red, 0.0f, blue, 1.0f };
}

/* =============================================================================
 * Animation Loop
 * ========================================================================== */

static void on_frame(f32 dt_unused, f32 time, void* ctx) {
  App* app = (App*)ctx;

  f32 max_U = -__FLT_MAX__;
  f32 min_U = __FLT_MAX__;
  f32 max_V = -__FLT_MAX__;
  f32 min_V = __FLT_MAX__;

  // Gray-Scott reaction-diffusion update
  for (u32 i = 0; i < GRID_SIZE; i++) {
    for (u32 j = 0; j < GRID_SIZE; j++) {
      // Discrete Laplacian (toroidal boundary)
      f32 u_laplacian =
        (g_U[i][wrap((i32)j + 1, GRID_SIZE)] +
         g_U[i][wrap((i32)j - 1, GRID_SIZE)] +
         g_U[wrap((i32)i + 1, GRID_SIZE)][j] +
         g_U[wrap((i32)i - 1, GRID_SIZE)][j]) / 4.0f -
        g_U[i][j];

      f32 v_laplacian =
        (g_V[i][wrap((i32)j + 1, GRID_SIZE)] +
         g_V[i][wrap((i32)j - 1, GRID_SIZE)] +
         g_V[wrap((i32)i + 1, GRID_SIZE)][j] +
         g_V[wrap((i32)i - 1, GRID_SIZE)][j]) / 4.0f -
        g_V[i][j];

      f32 uvv = g_U[i][j] * g_V[i][j] * g_V[i][j];

      // Update U and V
      g_U[i][j] += DT * (DU * u_laplacian - uvv + FEED * (1.0f - g_U[i][j]));
      g_V[i][j] += DT * (DV * v_laplacian + uvv - (KILL + FEED) * g_V[i][j]);

      // Track min/max for shader normalization
      if (g_U[i][j] > max_U) max_U = g_U[i][j];
      if (g_U[i][j] < min_U) min_U = g_U[i][j];
      if (g_V[i][j] > max_V) max_V = g_V[i][j];
      if (g_V[i][j] < min_V) min_V = g_V[i][j];
    }
  }

  // Render
  TuringShaderContext shader_ctx = { min_U, max_U, min_V, max_V };
  map_tela(app->tela, turing_shader, &shader_ctx);

  set_window_title(app->window, format_string("Turing Pattern | FPS: %.2f | R: reset", 1.0f / dt_unused));
  paint_window(app->window, app->tela);
}

static void on_close(Window* window, void* ctx) {
  Loop* animation = (Loop*)ctx;
  stop_loop(animation);
}

/* =============================================================================
 * Input Handlers
 * ========================================================================== */

static bool g_mouse_down = false;

static void on_mouse_down(Window* window, i32 x, i32 y, u32 button, void* ctx) {
  g_mouse_down = true;
}

static void on_mouse_up(Window* window, i32 x, i32 y, u32 button, void* ctx) {
  g_mouse_down = false;
}

static void on_key_down(Window* window, u32 keycode, void* ctx) {
  if (keycode == SDLK_r) {
    initialize_state();
  }
}

static void on_mouse_move(Window* window, i32 x, i32 y, void* ctx) {
  if (!g_mouse_down) return;

  // Map window coordinates to grid coordinates
  i32 xi = (i32)((x / (f32)WIDTH) * GRID_SIZE);
  i32 yi = (i32)((y / (f32)HEIGHT) * GRID_SIZE);
  u32 i = wrap(yi - (i32)GRID_SIZE + 1, (i32)GRID_SIZE);
  u32 j = wrap(xi, (i32)GRID_SIZE);

  // Paint a 3x3 brush of V concentration
  i32 brush[] = { -1, 0, 1 };
  // i32 brush[] = {-2, -1, 0, 1, 2 }; // Uncomment for larger brush
  u32 brush_size = sizeof(brush) / sizeof(brush[0]);
  u32 nn = brush_size * brush_size;

  for (u32 k = 0; k < nn; k++) {
    u32 u = k / brush_size;
    u32 v = k % brush_size;
    g_V[wrap((i32)i + brush[u], GRID_SIZE)][wrap((i32)j + brush[v], GRID_SIZE)] = AMPLITUDE;
  }
}

static void register_input_handlers(Window* window, App* app) {
  on_mouse_down_window(window, on_mouse_down, app);
  on_mouse_up_window(window, on_mouse_up, app);
  on_mouse_move_window(window, on_mouse_move, app);
  on_key_down_window(window, on_key_down, app);
}

/* =============================================================================
 * Main
 * ========================================================================== */

int main(void) {
  initialize_state();

  Tela* tela = new_tela(WIDTH, HEIGHT);
  Window* window = new_window(WIDTH, HEIGHT, "Turing Pattern");

  App app = { .tela = tela, .window = window };

  Loop* animation = loop(on_frame, &app);

  on_close_window(window, on_close, animation);
  register_input_handlers(window, &app);

  play_loop(animation);
  return 0;
}
