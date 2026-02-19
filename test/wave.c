/**
 * Wave Simulation
 *
 * A 2D wave equation simulation with interactive mouse drawing.
 * Uses symplectic integration to solve the wave PDE on a toroidal grid.
 * Color encodes height (red-blue) and speed (green).
 */

#include "../src/index.c"

 /* =============================================================================
  * Constants
  * ========================================================================== */

static const u32 WIDTH = 640;
static const u32 HEIGHT = 480;

#define GRID_SIZE 200
#define AMPLITUDE 10.0f
#define FRICTION 0.1f
#define WAVE_SPEED 10.0f
#define SPREAD 200.0f

/* =============================================================================
 * Utilities
 * ========================================================================== */

static u32 wrap(i32 n, i32 m) { return (u32)(((n % m) + m) % m); }

/* =============================================================================
 * Wave State
 * ========================================================================== */

static f32 g_height[GRID_SIZE][GRID_SIZE];
static f32 g_velocity[GRID_SIZE][GRID_SIZE];

static void initialize_wave(void) {
  for (u32 i = 0; i < GRID_SIZE; i++) {
    for (u32 j = 0; j < GRID_SIZE; j++) {
      f32 x = (j - GRID_SIZE / 2.0f) / (f32)GRID_SIZE;
      f32 y = (i - GRID_SIZE / 2.0f) / (f32)GRID_SIZE;

      // Three Gaussian bumps arranged in a triangle
      f32 bump1 = AMPLITUDE * expf(-SPREAD * ((x - 0.25f) * (x - 0.25f) + y * y));
      f32 bump2 = AMPLITUDE * expf(-SPREAD * ((x + 0.25f) * (x + 0.25f) + y * y));
      f32 bump3 = AMPLITUDE * expf(-SPREAD * (x * x + (y - 0.25f) * (y - 0.25f)));

      g_height[i][j] = bump1 + bump2 + bump3;
      g_velocity[i][j] = 0.0f;
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
  f32 min_height;
  f32 max_height;
  f32 max_abs_velocity;
} WaveShaderContext;

/* =============================================================================
 * Shader
 * ========================================================================== */

static Color wave_shader(u32 x, u32 y, const void* context) {
  WaveShaderContext* ctx = (WaveShaderContext*)context;
  f32 range = ctx->max_height - ctx->min_height;

  u32 xi = (u32)((x / (f32)WIDTH) * GRID_SIZE);
  u32 yi = (u32)((y / (f32)HEIGHT) * GRID_SIZE);
  f32 t = (g_height[yi][xi] - ctx->min_height) / range;
  f32 red = t;
  f32 blue = 1.0f - t;
  f32 green = fabsf(g_velocity[yi][xi]) / ctx->max_abs_velocity;

  return (Color) { red, green, blue, 1.0f };
}

/* =============================================================================
 * Animation Loop
 * ========================================================================== */

static void on_frame(f32 dt, f32 time, void* ctx) {
  App* app = (App*)ctx;

  f32 max_h = -__FLT_MAX__;
  f32 min_h = __FLT_MAX__;
  f32 max_v = -__FLT_MAX__;

  // Symplectic Euler integration of the 2D wave equation
  for (u32 i = 0; i < GRID_SIZE; i++) {
    for (u32 j = 0; j < GRID_SIZE; j++) {
      // Discrete Laplacian (toroidal boundary)
      f32 laplacian =
        g_height[i][wrap((i32)j + 1, GRID_SIZE)] +
        g_height[i][wrap((i32)j - 1, GRID_SIZE)] +
        g_height[wrap((i32)i + 1, GRID_SIZE)][j] +
        g_height[wrap((i32)i - 1, GRID_SIZE)][j] -
        4.0f * g_height[i][j];

      f32 acceleration = WAVE_SPEED * laplacian - FRICTION * g_velocity[i][j];

      // Update velocity, then position (symplectic order)
      g_velocity[i][j] += dt * acceleration;
      g_height[i][j] += dt * g_velocity[i][j];

      // Track min/max for shader normalization
      if (g_height[i][j] > max_h) max_h = g_height[i][j];
      if (g_height[i][j] < min_h) min_h = g_height[i][j];
      f32 abs_v = fabsf(g_velocity[i][j]);
      if (abs_v > max_v) max_v = abs_v;
    }
  }

  // Render
  WaveShaderContext shader_ctx = { min_h, max_h, max_v };
  map_tela(app->tela, wave_shader, &shader_ctx);

  set_window_title(app->window, format_string("FPS: %.2f", 1.0f / dt));
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

static void on_mouse_move(Window* window, i32 x, i32 y, void* ctx) {
  if (!g_mouse_down) return;

  App* app = (App*)ctx;

  // Map window coordinates directly to grid coordinates
  i32 xi = (i32)((x / (f32)WIDTH) * GRID_SIZE);
  i32 yi = (i32)((y / (f32)HEIGHT) * GRID_SIZE);
  u32 i = wrap(yi - (i32)GRID_SIZE + 1, (i32)GRID_SIZE);
  u32 j = wrap(xi, (i32)GRID_SIZE);

  // Paint a 3x3 brush
  i32 brush[] = { -1, 0, 1 };
  // i32 brush[] = {-2, -1, 0, 1, 2 }; // Uncomment for larger brush
  u32 brush_size = sizeof(brush) / sizeof(brush[0]);
  u32 nn = brush_size * brush_size;

  for (u32 k = 0; k < nn; k++) {
    u32 u = k / brush_size;
    u32 v = k % brush_size;
    g_height[wrap((i32)i + brush[u], GRID_SIZE)][wrap((i32)j + brush[v], GRID_SIZE)] = AMPLITUDE;
  }
}

static void register_input_handlers(Window* window, App* app) {
  on_mouse_down_window(window, on_mouse_down, app);
  on_mouse_up_window(window, on_mouse_up, app);
  on_mouse_move_window(window, on_mouse_move, app);
}

/* =============================================================================
 * Main
 * ========================================================================== */

int main(void) {
  initialize_wave();

  Tela* tela = new_tela(WIDTH, HEIGHT);
  Window* window = new_window(WIDTH, HEIGHT, "Wave Simulation");

  App app = { .tela = tela, .window = window };

  Loop* animation = loop(on_frame, &app);

  on_close_window(window, on_close, animation);
  register_input_handlers(window, &app);

  play_loop(animation);
  return 0;
}
