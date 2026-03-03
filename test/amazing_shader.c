#include "../src/index.c"

// Port from https://www.shadertoy.com/view/mtyGWy
// Beautified by Claude Opus

// gcc -O3 -fopenmp -o app test/amazing_shader.c -lSDL2 -lm

/* =============================================================================
 * Constants
 * ========================================================================== */

static const u32 WIDTH = 640;
static const u32 HEIGHT = 480;

static const f32 TAU = 6.28318f;
static const u32 FRACTAL_ITERATIONS = 4;
static const f32 FRACTAL_SCALE = 1.5f;
static const f32 WAVE_FREQUENCY = 8.0f;
static const f32 GLOW_INTENSITY = 0.01f;
static const f32 GLOW_FALLOFF = 1.2f;

/* =============================================================================
 * Types
 * ========================================================================== */

typedef struct {
  Tela *tela;
  Window *window;
  bool use_parallel;
} App;

/* =============================================================================
 * Color Utilities
 * ========================================================================== */

/**
 * Clamp a value to [0, 1] range.
 */
static inline f32 saturate(f32 x) {
  return fminf(fmaxf(0.0f, x), 1.0f);
}

/**
 * Compute a smooth animated color palette.
 *
 * Formula: color = a + b * cos(2π * (c * t + d))
 *
 * This technique creates beautiful, continuously varying colors.
 * See: https://iquilezles.org/articles/palettes/
 *
 * @param t     Parameter (typically based on distance + time)
 * @param out   Output RGB color array
 */
static void cosine_palette(f32 t, f32 out[3]) {
  // Palette coefficients (can be tweaked for different color schemes)
  static const f32 a[3] = {0.5f, 0.5f, 0.5f};   // Brightness offset
  static const f32 b[3] = {0.5f, 0.5f, 0.5f};   // Contrast/amplitude
  static const f32 c[3] = {1.0f, 1.0f, 1.0f};   // Frequency
  static const f32 d[3] = {0.263f, 0.416f, 0.557f}; // Phase offset (hue shift)

  for (u32 i = 0; i < 3; i++) {
    out[i] = a[i] + b[i] * cosf(TAU * (c[i] * t + d[i]));
  }
}

/* =============================================================================
 * Shader
 * ========================================================================== */

/**
 * Main fragment shader - computes color for each pixel.
 *
 * Creates a layered fractal pattern with animated glow effects.
 */
static Color fragment_shader(u32 x, u32 y, const void *context) {
  const f32 time = *(const f32 *)context;

  // Normalize coordinates to [-aspect, aspect] x [-1, 1]
  const f32 aspect = (f32)WIDTH / (f32)HEIGHT;
  f32 u = (2.0f * (f32)x / (f32)WIDTH - 1.0f) * aspect;
  f32 v = 2.0f * (f32)y / (f32)HEIGHT - 1.0f;

  // Store original position for distance-based effects
  const f32 u0 = u;
  const f32 v0 = v;
  const f32 dist_from_center = -sqrtf(u0 * u0 + v0 * v0);

  // Accumulate color across fractal iterations
  f32 rgb[3] = {0.0f, 0.0f, 0.0f};

  for (u32 i = 0; i < FRACTAL_ITERATIONS; i++) {
    // Fractal fold: scale and wrap to [-0.5, 0.5]
    u = (u * FRACTAL_SCALE - floorf(u * FRACTAL_SCALE)) - 0.5f;
    v = (v * FRACTAL_SCALE - floorf(v * FRACTAL_SCALE)) - 0.5f;

    // Distance with exponential falloff from center
    f32 d = sqrtf(u * u + v * v) * expf(dist_from_center);

    // Get palette color based on distance + iteration + time
    f32 col[3];
    cosine_palette(dist_from_center + (f32)i * 0.4f + time * 0.4f, col);

    // Animated sine wave pattern
    d = sinf(d * WAVE_FREQUENCY + time) / WAVE_FREQUENCY;
    d = fabsf(d);

    // Glow effect: inverse power falloff
    d = fmaxf(d, 1e-6f); // Prevent division by zero
    d = powf(GLOW_INTENSITY / d, GLOW_FALLOFF);

    // Accumulate weighted color
    rgb[0] += col[0] * d;
    rgb[1] += col[1] * d;
    rgb[2] += col[2] * d;
  }

  // Clamp and return final color
  return (Color){
      .red = saturate(rgb[0]),
      .green = saturate(rgb[1]),
      .blue = saturate(rgb[2]),
      .alpha = 1.0f
  };
}

/* =============================================================================
 * Animation Loop
 * ========================================================================== */

static void on_frame(f32 dt, f32 time, void *ctx) {
  App *app = (App *)ctx;

  if (app->use_parallel) {
    map_tela_parallel(app->tela, fragment_shader, &time);
  } else {
    map_tela(app->tela, fragment_shader, &time);
  }

  set_window_title(
      app->window,
      format_string("Amazing Shader | Parallel: %s (P) | FPS: %.1f",
                    app->use_parallel ? "ON" : "OFF", 1.0f / dt));
  paint_window(app->window, app->tela);
}

static void on_key_down(Window *window, u32 keycode, void *ctx) {
  App *app = (App *)ctx;
  if (keycode == SDLK_p) {
    app->use_parallel = !app->use_parallel;
  }
}

static void on_close(Window *window, void *ctx) {
  Loop *animation = (Loop *)ctx;
  stop_loop(animation);
}

/* =============================================================================
 * Main
 * ========================================================================== */

int main(void) {
  // Create window and canvas
  Tela *tela = new_tela(WIDTH, HEIGHT);
  Window *window = new_window(WIDTH, HEIGHT, "Amazing Shader");

  // Application state
  App app = {
      .tela = tela,
      .window = window,
      .use_parallel = true,
  };

  // Animation loop
  Loop *animation = loop(on_frame, &app);

  // Event handlers
  on_close_window(window, on_close, animation);
  on_key_down_window(window, on_key_down, &app);

  // Run (blocks until window closes)
  play_loop(animation);

  return 0;
}
