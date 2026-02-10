/**
 * Line Test Window
 *
 * Interactive demo for drawing lines on a canvas.
 * Click and drag to draw colored lines.
 */

#include "../src/index.c"

/* =============================================================================
 * Constants
 * ========================================================================== */

#define WIDTH 640
#define HEIGHT 480

#define BLACK_COLOR ((Color){0.0f, 0.0f, 0.0f, 1.0f})
#define WHITE_COLOR ((Color){1.0f, 1.0f, 1.0f, 1.0f})

/* =============================================================================
 * Type Definitions
 * ========================================================================== */

/**
 * Context for line shader - stores the color for each line
 */
typedef struct {
  Color color;
} LineContext;

/**
 * Application context containing all state
 */
typedef struct {
  Tela *tela;
  Window *window;
  Array lines;
} AppContext;

/* =============================================================================
 * Global State
 * ========================================================================== */

static bool g_is_mouse_down = false;
static Vec2 g_line_start = {0.0f, 0.0f};
static Vec2 g_line_end = {0.0f, 0.0f};

/* =============================================================================
 * Shader
 * ========================================================================== */

/**
 * Shader function for rendering lines
 * Returns the color stored in the line's context
 */
Color line_shader(u32 x, u32 y, Line_2D *line, void *context) {
  LineContext *line_ctx = (LineContext *)line->context;
  return line_ctx->color;
}

/* =============================================================================
 * Animation Loop
 * ========================================================================== */

/**
 * Main render loop - clears screen, draws all lines, updates FPS
 */
void render_frame(f32 dt, f32 time, void *context) {
  AppContext *app = (AppContext *)context;

  // Clear the canvas
  fill_tela(app->tela, BLACK_COLOR);

  // Draw all stored lines
  Array *lines = &app->lines;
  for (u32 i = 0; i < lines->length; i++) {
    Line_2D *line = get_array_element(lines, i);
    draw_line_tela(app->tela, line, line_shader, NULL);
  }

  // Draw preview line while dragging
  if (g_is_mouse_down) {
    LineContext preview_ctx = {WHITE_COLOR};
    Line_2D preview_line = {g_line_start, g_line_end, &preview_ctx};
    draw_line_tela(app->tela, &preview_line, line_shader, NULL);
  }

  // Update window title with FPS
  set_window_title(app->window, format_string("FPS: %.2f", 1.0f / dt));

  // Present the frame
  paint_window(app->window, app->tela);
}

/* =============================================================================
 * Event Handlers
 * ========================================================================== */

/**
 * Called when window close button is clicked
 */
void on_window_close(Window *window, void *context) {
  Loop *animation_loop = (Loop *)context;
  stop_loop(animation_loop);
}

/**
 * Called when mouse button is pressed - starts a new line
 */
void on_mouse_down(Window *window, i32 x, i32 y, u32 button, void *context) {
  g_is_mouse_down = true;
  g_line_start = vec2((f32)x, (f32)y);
}

/**
 * Called when mouse button is released - finalizes the line
 */
void on_mouse_up(Window *window, i32 x, i32 y, u32 button, void *context) {
  g_is_mouse_down = false;

  AppContext *app = (AppContext *)context;
  Vec2 line_end = vec2((f32)x, (f32)y);

  // Create persistent line context with random color
  LineContext *line_ctx = malloc(sizeof(LineContext));
  line_ctx->color = random_color();

  // Add the new line to the collection
  Line_2D new_line = {g_line_start, line_end, line_ctx};
  push_array(&app->lines, &new_line);
}

/**
 * Called when mouse moves - updates preview line end position
 */
void on_mouse_move(Window *window, i32 x, i32 y, void *context) {
  g_line_end = vec2((f32)x, (f32)y);
}

/**
 * Registers all mouse event handlers
 */
void register_event_handlers(Window *window, AppContext *app) {
  on_mouse_down_window(window, on_mouse_down, app);
  on_mouse_up_window(window, on_mouse_up, app);
  on_mouse_move_window(window, on_mouse_move, app);
}

/* =============================================================================
 * Main
 * ========================================================================== */

int main() {
  // Initialize graphics
  Tela *tela = new_tela(WIDTH, HEIGHT);
  Window *window = new_window(WIDTH, HEIGHT, "Line Tests");

  // Initialize application state
  Array lines = new_array(100, sizeof(Line_2D));
  AppContext app = {tela, window, lines};

  // Setup animation loop
  Loop *animation_loop = loop(render_frame, &app);
  on_close_window(window, on_window_close, animation_loop);

  // Register input handlers
  register_event_handlers(window, &app);

  // Start the application (must be last)
  play_loop(animation_loop);

  return 0;
}
