/**
 * AI generated fluid simulation example.
 * Interactive fluid simulation on a periodic grid.
 * Left drag paints density; right drag adds velocity.
 */
#include "../src/index.c"

/* gcc -O3 -fopenmp -o app test/fluid.c -lSDL2 -lm && ./app */

#define WIDTH 640
#define HEIGHT 480
#define GRID_SIZE 200
#define GRID_SCALE (GRID_SIZE / 100.0f)
#define GRID_CELLS (GRID_SIZE * GRID_SIZE)
#define VISCOSITY 1.2f
// Unit grid spacing: explicit diffusion requires VISCOSITY * dt <= 1/4.
#define MAX_DT 0.05f
#define PRESSURE_ITERATIONS 50

static f32 ux[GRID_CELLS], uy[GRID_CELLS], density[GRID_CELLS];
static f32 pressure[GRID_CELLS], divergence[GRID_CELLS];
static f32 next_x[GRID_CELLS], next_y[GRID_CELLS], next_density[GRID_CELLS];

typedef struct {
  Tela* tela;
  Window* window;
  bool left_down;
  bool right_down;
  f32 last_x;
  f32 last_y;
} App;

static inline i32 wrap(i32 n) {
  n %= GRID_SIZE;
  return n < 0 ? n + GRID_SIZE : n;
}

static inline i32 index_at(i32 x, i32 y) {
  return wrap(x) + GRID_SIZE * wrap(y);
}

static void diffuse(const f32* source, f32* target, f32 dt) {
  f32 a = VISCOSITY * dt;
  for (i32 y = 0; y < GRID_SIZE; y++) {
    for (i32 x = 0; x < GRID_SIZE; x++) {
      i32 i = index_at(x, y);
      f32 laplacian = source[index_at(x - 1, y)] + source[index_at(x + 1, y)] +
          source[index_at(x, y - 1)] + source[index_at(x, y + 1)] - 4.0f * source[i];
      target[i] = source[i] + a * laplacian;
    }
  }
}

static void advect(
    const f32* source, f32* target, const f32* velocity_x,
    const f32* velocity_y, f32 dt
) {
  for (i32 y = 0; y < GRID_SIZE; y++) {
    for (i32 x = 0; x < GRID_SIZE; x++) {
      i32 i = index_at(x, y);
      f32 px = x - dt * velocity_x[i];
      f32 py = y - dt * velocity_y[i];
      i32 x0 = (i32)floorf(px);
      i32 y0 = (i32)floorf(py);
      f32 tx = px - x0;
      f32 ty = py - y0;
      target[i] = (1.0f - ty) * ((1.0f - tx) * source[index_at(x0, y0)] +
          tx * source[index_at(x0 + 1, y0)]) +
          ty * ((1.0f - tx) * source[index_at(x0, y0 + 1)] +
          tx * source[index_at(x0 + 1, y0 + 1)]);
    }
  }
}

static void project(f32* velocity_x, f32* velocity_y) {
  // Approximate incompressibility by removing the pressure gradient:
  // div(u - grad(p)) = 0, so solve Laplacian(p) = div(u).
  memset(pressure, 0, sizeof(pressure));
  for (i32 y = 0; y < GRID_SIZE; y++) {
    for (i32 x = 0; x < GRID_SIZE; x++) {
      divergence[index_at(x, y)] = 0.5f * (
          velocity_x[index_at(x + 1, y)] - velocity_x[index_at(x - 1, y)] +
          velocity_y[index_at(x, y + 1)] - velocity_y[index_at(x, y - 1)]);
    }
  }
  // Solve Laplacian(p) = div(u) on the periodic grid. With unit spacing:
  // p[i-1,j] + p[i+1,j] + p[i,j-1] + p[i,j+1] - 4*p[i,j] = div(u)[i,j].
  // Rearranging gives p[i,j] = (sum of neighbors - div(u)[i,j]) / 4.
  // Gauss-Seidel repeats this update in place to approximate the solution.
  // The centered divergence and gradient compose to a different stencil,
  // so subtracting the pressure gradient only approximately removes divergence.
  for (i32 iteration = 0; iteration < PRESSURE_ITERATIONS; iteration++) {
    for (i32 y = 0; y < GRID_SIZE; y++) {
      for (i32 x = 0; x < GRID_SIZE; x++) {
        pressure[index_at(x, y)] = 0.25f * (
            pressure[index_at(x - 1, y)] + pressure[index_at(x + 1, y)] +
            pressure[index_at(x, y - 1)] + pressure[index_at(x, y + 1)] -
            divergence[index_at(x, y)]);
      }
    }
  }
  for (i32 y = 0; y < GRID_SIZE; y++) {
    for (i32 x = 0; x < GRID_SIZE; x++) {
      i32 i = index_at(x, y);
      velocity_x[i] -= 0.5f * (pressure[index_at(x + 1, y)] - pressure[index_at(x - 1, y)]);
      velocity_y[i] -= 0.5f * (pressure[index_at(x, y + 1)] - pressure[index_at(x, y - 1)]);
    }
  }
}

static void step(f32 dt) {
  dt = fminf(fmaxf(dt, 0.0f), MAX_DT);
  // Diffuse the current velocity into scratch buffers.
  diffuse(ux, next_x, dt);
  diffuse(uy, next_y, dt);
  // Read both components from the unchanged scratch buffers and write
  // the advected velocity back into the current ux and uy buffers.
  advect(next_x, ux, next_x, next_y, dt);
  advect(next_y, uy, next_x, next_y, dt);
  // Reduce divergence after advection before moving density.
  project(ux, uy);
  advect(density, next_density, ux, uy, dt);
  memcpy(density, next_density, sizeof(density));
}

static void brush(f32 x, f32 y, bool paint_density, f32 dx, f32 dy) {
  i32 radius = (i32)(2.0f * GRID_SCALE);
  for (i32 j = -radius; j <= radius; j++) {
    for (i32 i = -radius; i <= radius; i++) {
      f32 weight = fmaxf(0.0f, 1.0f - (i * i + j * j) / (9.0f * GRID_SCALE * GRID_SCALE));
      i32 k = index_at((i32)floorf(x) + i, (i32)floorf(y) + j);
      if (paint_density) density[k] = fminf(1.0f, density[k] + 0.4f * weight);
      ux[k] += dx * weight;
      uy[k] += dy * weight;
    }
  }
}

static f32 grid_x(i32 x) { return x * (f32)GRID_SIZE / WIDTH; }
static f32 grid_y(i32 y) { return y * (f32)GRID_SIZE / HEIGHT; }

static Color fluid_shader(u32 x, u32 y, const void* ctx) {
  f32 d = clamp(density[index_at((i32)grid_x(x), (i32)grid_y(y))], 0.0f, 1.0f);
  return (Color){ d, d, d, 1.0f };
}

static void on_frame(f32 dt, f32 time, void* ctx) {
  App* app = (App*)ctx;
  char title[128];
  snprintf(title, sizeof(title), "Fluid | FPS: %.0f | Left: density | Right drag: velocity",
      dt > 0.0f ? 1.0f / dt : 0.0f);
  set_window_title(app->window, title);
  step(dt);
  map_tela(app->tela, fluid_shader, NULL);
  paint_window(app->window, app->tela);
}

static void on_close(Window* window, void* ctx) {
  stop_loop((Loop*)ctx);
}

static void on_mouse_down(Window* window, i32 x, i32 y, u32 button, void* ctx) {
  App* app = (App*)ctx;
  f32 gx = grid_x(x), gy = grid_y(y);
  if (button == SDL_BUTTON_LEFT) {
    app->left_down = true;
    brush(gx, gy, true, 0.0f, 0.0f);
  }
  if (button == SDL_BUTTON_RIGHT) app->right_down = true;
  app->last_x = gx;
  app->last_y = gy;
}

static void on_mouse_up(Window* window, i32 x, i32 y, u32 button, void* ctx) {
  App* app = (App*)ctx;
  if (button == SDL_BUTTON_LEFT) app->left_down = false;
  if (button == SDL_BUTTON_RIGHT) app->right_down = false;
}

static void on_mouse_move(Window* window, i32 x, i32 y, void* ctx) {
  App* app = (App*)ctx;
  f32 gx = grid_x(x), gy = grid_y(y);
  if (app->left_down || app->right_down) {
    f32 dx = gx - app->last_x, dy = gy - app->last_y;
    i32 count = (i32)fmaxf(1.0f, ceilf(hypotf(dx, dy) / GRID_SCALE));
    for (i32 k = 1; k <= count; k++) {
      brush(app->last_x + dx * k / count, app->last_y + dy * k / count,
          app->left_down, app->right_down ? 6.0f * dx / count : 0.0f,
          app->right_down ? 6.0f * dy / count : 0.0f);
    }
  }
  app->last_x = gx;
  app->last_y = gy;
}

int main(void) {
  Tela* tela = new_tela(WIDTH, HEIGHT);
  Window* window = new_window(WIDTH, HEIGHT, "Fluid");
  if (!window) {
    free_tela(tela);
    return 1;
  }
  App app = { .tela = tela, .window = window };
  Loop* animation = loop(on_frame, &app);
  on_close_window(window, on_close, animation);
  on_mouse_down_window(window, on_mouse_down, &app);
  on_mouse_up_window(window, on_mouse_up, &app);
  on_mouse_move_window(window, on_mouse_move, &app);
  play_loop(animation);
  free_loop(animation);
  free_window(window);
  free_tela(tela);
  return 0;
}
