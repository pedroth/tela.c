/**
 * Simple Animation
 *
 * A minimal animated shader demo. Renders a time-varying color gradient
 * that shifts across the window based on pixel coordinates and elapsed time.
 */

#include "../src/index.c"

/* =============================================================================
 * Constants
 * ========================================================================== */

static const u32 WIDTH = 640;
static const u32 HEIGHT = 480;

/* =============================================================================
 * Types
 * ========================================================================== */

typedef struct {
  Tela *tela;
  Window *window;
} App;

/* =============================================================================
 * Shader
 * ========================================================================== */

/**
 * Color gradient shader - maps pixel position and time to an RG color ramp.
 */
static Color shader(u32 x, u32 y, const void *context) {
  f32 time = *(f32 *)context;
  f32 red = fmodf(((f32)x * time) / (f32)WIDTH, 1.0f);
  f32 green = fmodf(((f32)y * time) / (f32)HEIGHT, 1.0f);
  return (Color){red, green, 0.0f, 1.0f};
}

/* =============================================================================
 * Animation Loop
 * ========================================================================== */

static void on_frame(f32 dt, f32 time, void *ctx) {
  App *app = (App *)ctx;

  // Render
  map_tela(app->tela, shader, &time);

  // Display
  set_window_title(app->window, format_string("FPS: %.2f", 1.0f / dt));
  paint_window(app->window, app->tela);
}

static void on_close(Window *window, void *ctx) {
  Loop *animation = (Loop *)ctx;
  stop_loop(animation);
}

/* =============================================================================
 * Main
 * ========================================================================== */

int main(void) {
  Tela *tela = new_tela(WIDTH, HEIGHT);
  Window *window = new_window(WIDTH, HEIGHT, "Simple Animation");

  App app = {.tela = tela, .window = window};

  Loop *animation = loop(on_frame, &app);
  on_close_window(window, on_close, animation);

  play_loop(animation);
  return 0;
}
