/**
 * AI made Walk on Spheres
 *
 * Progressive solution of Laplace's equation on an annulus. Drag to pan and
 * scroll to zoom; changing the view restarts the accumulation.
 */

// compile with: gcc -O3 -fopenmp -o app test/walk_on_spheres.c -lSDL2 -lm

#include "../src/index.c"

#define WIDTH 256
#define HEIGHT 256
#define PIXELS (WIDTH * HEIGHT)
#define STEPS_PER_FRAME 8

static const f64 INNER_RADIUS = 0.5;
static const f64 OUTER_RADIUS = 1.0;
static const f64 OMEGA = 10.0;
static const f64 WALK_EPSILON = 1e-4;
static const Color BACKGROUND = {0.06f, 0.07f, 0.1f, 1.0f};

typedef struct {
  Tela* tela;
  Window* window;
  AABB_2D box;
  f64 xs[PIXELS];
  f64 ys[PIXELS];
  f32 values[PIXELS];
  bool inside[PIXELS];
  u32 active[PIXELS];
  u32 remaining;
  u32 samples;
  Vec2 mouse;
  bool dragging;
} App;

static f64 annulus_sdf(f64 x, f64 y) {
  f64 radius = hypot(x, y);
  return fmax(radius - OUTER_RADIUS, INNER_RADIUS - radius);
}

static void create_walk_pass(App* app) {
  app->remaining = 0;
  for (u32 y = 0; y < HEIGHT; y++) {
    for (u32 x = 0; x < WIDTH; x++) {
      u32 index = y * WIDTH + x;
      f64 px = app->box.min.x + (f64)x / WIDTH * app->box.diagonal.x;
      f64 py = app->box.min.y + (f64)y / HEIGHT * app->box.diagonal.y;
      app->inside[index] = annulus_sdf(px, py) <= 0.0;
      if (!app->inside[index]) continue;
      app->xs[index] = px;
      app->ys[index] = py;
      app->active[app->remaining++] = index;
    }
  }
}

static bool advance_walk_pass(App* app) {
  u32 remaining = 0;
  for (u32 i = 0; i < app->remaining; i++) {
    u32 index = app->active[i];
    f64 x = app->xs[index];
    f64 y = app->ys[index];
    bool finished = false;
    for (u32 step = 0; step < STEPS_PER_FRAME; step++) {
      f64 distance = -annulus_sdf(x, y);
      if (distance <= WALK_EPSILON) {
        app->values[index] = (f32)sin(OMEGA * atan2(y, x));
        finished = true;
        break;
      }
      f64 angle = 2.0 * PI * random_double();
      x += distance * cos(angle);
      y += distance * sin(angle);
    }
    if (!finished) {
      app->xs[index] = x;
      app->ys[index] = y;
      app->active[remaining++] = index;
    }
  }
  app->remaining = remaining;
  return remaining == 0;
}

static Color walk_pixel(u32 x, u32 y, const void* context) {
  const App* app = (const App*)context;
  u32 index = y * WIDTH + x;
  if (!app->inside[index]) return BACKGROUND;
  f32 value = app->values[index];
  return (Color){0.5f + 0.5f * value, 0.5f, 0.5f - 0.5f * value, 1.0f};
}

static void update_title(App* app) {
  char title[128];
  snprintf(title, sizeof(title),
           "Walk on spheres: %u samples per pixel | Drag to pan, scroll to zoom",
           app->samples);
  set_window_title(app->window, title);
}

static void reset(App* app) {
  app->samples = 0;
  app->tela->iterations = 1;
  fill_tela(app->tela, BACKGROUND);
  create_walk_pass(app);
  update_title(app);
}

static void on_frame(f32 dt, f32 time, void* context) {
  (void)dt;
  (void)time;
  App* app = (App*)context;
  if (advance_walk_pass(app)) {
    map_exposed_tela(app->tela, walk_pixel, app);
    if (app->samples < UINT32_MAX) app->samples++;
    update_title(app);
    create_walk_pass(app);
  }
  paint_window(app->window, app->tela);
}

static void on_close(Window* window, void* context) {
  (void)window;
  stop_loop((Loop*)context);
}

static void on_mouse_down(Window* window, i32 x, i32 y, u32 button, void* context) {
  (void)window;
  (void)button;
  App* app = (App*)context;
  app->dragging = true;
  app->mouse = vec2((f32)x, (f32)y);
}

static void on_mouse_up(Window* window, i32 x, i32 y, u32 button, void* context) {
  (void)window;
  (void)x;
  (void)y;
  (void)button;
  ((App*)context)->dragging = false;
}

static void on_mouse_move(Window* window, i32 x, i32 y, void* context) {
  (void)window;
  App* app = (App*)context;
  Vec2 next = vec2((f32)x, (f32)y);
  if (!app->dragging || equals_vec2(next, app->mouse)) return;
  Vec2 delta = sub_vec2(next, app->mouse);
  Vec2 movement = mul_vec2(scale_vec2(delta, -1.0f),
                           div_vec2(app->box.diagonal, vec2(WIDTH, HEIGHT)));
  app->box = build_aabb_2d(add_vec2(app->box.min, movement),
                           add_vec2(app->box.max, movement));
  app->mouse = next;
  reset(app);
}

static void on_mouse_scroll(Window* window, i32 delta_y, void* context) {
  (void)window;
  if (delta_y == 0) return;
  App* app = (App*)context;
  f32 scale = delta_y > 0 ? 1.1f : 1.0f / 1.1f;
  f32 next_width = app->box.diagonal.x * scale;
  if (next_width < 1e-3f || next_width > 1e3f) return;
  Vec2 half = scale_vec2(app->box.diagonal, 0.5f * scale);
  app->box = build_aabb_2d(sub_vec2(app->box.center, half),
                           add_vec2(app->box.center, half));
  reset(app);
}

int main(void) {
  App* app = (App*)calloc(1, sizeof(App));
  if (!app) {
    fprintf(stderr, "Unable to allocate walk-on-spheres state\n");
    return 1;
  }
  app->tela = new_tela(WIDTH, HEIGHT);
  app->window = new_window(640, 640, "Walk on spheres");
  if (!app->window) {
    free_tela(app->tela);
    free(app);
    return 1;
  }
  app->box = build_aabb_2d(vec2(-1.2f, -1.2f), vec2(1.2f, 1.2f));

  Loop* animation = loop(on_frame, app);
  on_close_window(app->window, on_close, animation);
  on_mouse_down_window(app->window, on_mouse_down, app);
  on_mouse_up_window(app->window, on_mouse_up, app);
  on_mouse_move_window(app->window, on_mouse_move, app);
  on_mouse_scroll_window(app->window, on_mouse_scroll, app);

  reset(app);
  play_loop(animation);

  free_loop(animation);
  free_window(app->window);
  free_tela(app->tela);
  free(app);
  return 0;
}
