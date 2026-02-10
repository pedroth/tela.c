#include "../src/index.c"

#define WIDTH 1280
#define HEIGHT 720

#define N_SIM 100
#define AMP 10.0f
#define FRICTION 0.1f
#define WAVE_SCALAR_SPEED 10.0f
#define SPREAD 100.0f

u32 mod(u32 n, u32 m) { return (n % m + m) % m; }

typedef struct {
  Color color;
} Line_Context;

Color line_shader(u32 x, u32 y, Line_2D *line, void *context) {
  Line_Context *line_context = (Line_Context *)line->context;
  return line_context->color;
}

typedef struct {
  Tela *tela;
  Window *window;
  Array lines;
} AnimeContext;

void anime_lambda(f32 dt, f32 time, void *context) {
  AnimeContext *anime_context = (AnimeContext *)context;
  Tela *tela = anime_context->tela;
  fill_tela(tela, (Color){0.0f, 0.0f, 0.0f, 1.0f});
  Array *lines = &anime_context->lines;
  for (u32 i = 0; i < lines->length; i++) {
    Line_2D *line = get_array_element(lines, i);
    draw_line_tela(tela, line, line_shader, NULL);
  }
  set_window_title(anime_context->window,
                   format_string("FPS: %.2f", 1.0f / dt));
  paint_window(anime_context->window, tela);
}

void on_close_lambda(Window *window, void *context) {
  Loop *anime_loop = (Loop *)context;
  stop_loop(anime_loop);
}

bool is_mouse_down = false;
Vec2 line_preview_start = {0.0f, 0.0f};
void mouse_down(Window *window, i32 x, i32 y, u32 button, void *context) {
  is_mouse_down = true;
  line_preview_start = vec2((f32)x, (f32)y);
}
void mouse_up(Window *window, i32 x, i32 y, u32 button, void *context) {
  is_mouse_down = false;
  AnimeContext *anime_context = (AnimeContext *)context;
  Array *lines = &anime_context->lines;
  Vec2 line_preview_end = vec2((f32)x, (f32)y);
  // Allocate Line_Context on heap so it persists after function returns
  Line_Context *line_context = malloc(sizeof(Line_Context));
  line_context->color = random_color();
  Line_2D new_line = {line_preview_start, line_preview_end, line_context};
  push_array(lines, &new_line);
}

void mouse_move(Window *window, i32 x, i32 y, void *context) {
  if (!is_mouse_down)
    return;
  AnimeContext *anime_context = (AnimeContext *)context;
  Tela *tela = anime_context->tela;
  Vec2 line_preview_end = vec2((f32)x, (f32)y);
  Line_Context line_context = {{1.0f, 1.0f, 1.0f, 1.0f}};
  Line_2D line_preview = {line_preview_start, line_preview_end, &line_context};
  draw_line_tela(tela, &line_preview, line_shader, NULL);
}

void handle_window_events(Window *window, AnimeContext *anime_context) {
  /* register handlers on the window */
  on_mouse_down_window(window, mouse_down, anime_context);
  on_mouse_up_window(window, mouse_up, anime_context);
  on_mouse_move_window(window, mouse_move, anime_context);
}

int main() {
  Tela *tela = new_tela(WIDTH, HEIGHT);
  Window *window = new_window(WIDTH, HEIGHT, "Line tests");
  Array lines = new_array(100, sizeof(Line_2D));
  AnimeContext anime_context = {tela, window, lines};
  Loop *anime_loop = loop(anime_lambda, &anime_context);
  on_close_window(window, on_close_lambda, anime_loop);
  handle_window_events(window, &anime_context);
  // must be last function to be called in main
  play_loop(anime_loop);
  return 0;
}
