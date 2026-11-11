/**
 * AI generated interactive 3D fluid on a periodic grid.
 * Left drag inside the cube adds density. Right drag inside adds velocity;
 * right drag starting outside orbits the camera. The mouse wheel zooms.
 *
 * gcc -O3 -fopenmp -o app test/fluid_3d.c -lSDL2 -lm && ./app
 */
#include "../src/index.c"

#define WIDTH 640
#define HEIGHT 480
#define GRID_SIZE 48
#define GRID_CELLS (GRID_SIZE * GRID_SIZE * GRID_SIZE)
#define VISCOSITY 0.5f
#define MAX_DT 0.05f
#define PRESSURE_ITERATIONS 30
#define MARCH_STEP 0.04f

static f32 ux[GRID_CELLS], uy[GRID_CELLS], uz[GRID_CELLS];
static f32 next_x[GRID_CELLS], next_y[GRID_CELLS], next_z[GRID_CELLS];
static f32 density[GRID_CELLS], next_density[GRID_CELLS];
static f32 pressure[GRID_CELLS], divergence[GRID_CELLS];

typedef enum { RIGHT_NONE, RIGHT_FLUID, RIGHT_ORBIT } RightDrag;

typedef struct {
  Tela* tela;
  Window* window;
  Camera camera;
  bool left_down;
  RightDrag right_drag;
  Vec2 mouse;
  bool has_point;
  Vec3 point;
} App;

static inline i32 wrap(i32 n) {
  n %= GRID_SIZE;
  return n < 0 ? n + GRID_SIZE : n;
}

static inline i32 index_at(i32 x, i32 y, i32 z) {
  return wrap(x) + GRID_SIZE * (wrap(y) + GRID_SIZE * wrap(z));
}

static f32 sample(const f32* field, f32 x, f32 y, f32 z) {
  i32 x0 = (i32)floorf(x), y0 = (i32)floorf(y), z0 = (i32)floorf(z);
  f32 tx = x - x0, ty = y - y0, tz = z - z0;
  f32 value = 0.0f;
  for (i32 dz = 0; dz <= 1; dz++) {
    for (i32 dy = 0; dy <= 1; dy++) {
      for (i32 dx = 0; dx <= 1; dx++) {
        f32 weight = (dx ? tx : 1.0f - tx) * (dy ? ty : 1.0f - ty) *
            (dz ? tz : 1.0f - tz);
        value += weight * field[index_at(x0 + dx, y0 + dy, z0 + dz)];
      }
    }
  }
  return value;
}

static void diffuse(const f32* source, f32* target, f32 dt) {
  f32 a = VISCOSITY * dt;
  for (i32 z = 0; z < GRID_SIZE; z++) {
    for (i32 y = 0; y < GRID_SIZE; y++) {
      for (i32 x = 0; x < GRID_SIZE; x++) {
        i32 i = index_at(x, y, z);
        f32 laplacian = source[index_at(x - 1, y, z)] +
            source[index_at(x + 1, y, z)] + source[index_at(x, y - 1, z)] +
            source[index_at(x, y + 1, z)] + source[index_at(x, y, z - 1)] +
            source[index_at(x, y, z + 1)] - 6.0f * source[i];
        target[i] = source[i] + a * laplacian;
      }
    }
  }
}

static void advect(
    const f32* source, f32* target, const f32* vx, const f32* vy,
    const f32* vz, f32 dt
) {
  for (i32 z = 0; z < GRID_SIZE; z++) {
    for (i32 y = 0; y < GRID_SIZE; y++) {
      for (i32 x = 0; x < GRID_SIZE; x++) {
        i32 i = index_at(x, y, z);
        target[i] = sample(
            source, x - dt * vx[i], y - dt * vy[i], z - dt * vz[i]
        );
      }
    }
  }
}

static void project(void) {
  memset(pressure, 0, sizeof(pressure));
  for (i32 z = 0; z < GRID_SIZE; z++) {
    for (i32 y = 0; y < GRID_SIZE; y++) {
      for (i32 x = 0; x < GRID_SIZE; x++) {
        divergence[index_at(x, y, z)] = 0.5f * (
            ux[index_at(x + 1, y, z)] - ux[index_at(x - 1, y, z)] +
            uy[index_at(x, y + 1, z)] - uy[index_at(x, y - 1, z)] +
            uz[index_at(x, y, z + 1)] - uz[index_at(x, y, z - 1)]
        );
      }
    }
  }
  for (i32 iteration = 0; iteration < PRESSURE_ITERATIONS; iteration++) {
    for (i32 z = 0; z < GRID_SIZE; z++) {
      for (i32 y = 0; y < GRID_SIZE; y++) {
        for (i32 x = 0; x < GRID_SIZE; x++) {
          i32 i = index_at(x, y, z);
          pressure[i] = (
              pressure[index_at(x - 1, y, z)] + pressure[index_at(x + 1, y, z)] +
              pressure[index_at(x, y - 1, z)] + pressure[index_at(x, y + 1, z)] +
              pressure[index_at(x, y, z - 1)] + pressure[index_at(x, y, z + 1)] -
              divergence[i]
          ) / 6.0f;
        }
      }
    }
  }
  for (i32 z = 0; z < GRID_SIZE; z++) {
    for (i32 y = 0; y < GRID_SIZE; y++) {
      for (i32 x = 0; x < GRID_SIZE; x++) {
        i32 i = index_at(x, y, z);
        ux[i] -= 0.5f * (pressure[index_at(x + 1, y, z)] - pressure[index_at(x - 1, y, z)]);
        uy[i] -= 0.5f * (pressure[index_at(x, y + 1, z)] - pressure[index_at(x, y - 1, z)]);
        uz[i] -= 0.5f * (pressure[index_at(x, y, z + 1)] - pressure[index_at(x, y, z - 1)]);
      }
    }
  }
}

static void step(f32 dt) {
  // The six-neighbor explicit diffusion stencil requires viscosity * dt <= 1/6.
  dt = clamp(dt, 0.0f, MAX_DT);
  diffuse(ux, next_x, dt);
  diffuse(uy, next_y, dt);
  diffuse(uz, next_z, dt);
  advect(next_x, ux, next_x, next_y, next_z, dt);
  advect(next_y, uy, next_x, next_y, next_z, dt);
  advect(next_z, uz, next_x, next_y, next_z, dt);
  project();
  advect(density, next_density, ux, uy, uz, dt);
  memcpy(density, next_density, sizeof(density));
}

static Vec3 grid_position(Vec3 p) {
  return scale_vec3(add_vec3(p, vec3(1.0f, 1.0f, 1.0f)), GRID_SIZE / 2.0f);
}

static void brush(Vec3 p, bool paint_density, Vec3 velocity) {
  Vec3 g = grid_position(p);
  for (i32 z = -2; z <= 2; z++) {
    for (i32 y = -2; y <= 2; y++) {
      for (i32 x = -2; x <= 2; x++) {
        f32 weight = fmaxf(0.0f, 1.0f - (x * x + y * y + z * z) / 9.0f);
        i32 i = index_at((i32)floorf(g.x) + x, (i32)floorf(g.y) + y, (i32)floorf(g.z) + z);
        if (paint_density) density[i] = fminf(1.0f, density[i] + 0.4f * weight);
        ux[i] += velocity.x * weight;
        uy[i] += velocity.y * weight;
        uz[i] += velocity.z * weight;
      }
    }
  }
}

// Return the complete forward ray interval inside the cube, including when
// the ray starts inside it.
static bool box_interval(Ray ray, f32* enter, f32* exit) {
  f32 origins[3] = { ray.init.x, ray.init.y, ray.init.z };
  f32 directions[3] = { ray.dir.x, ray.dir.y, ray.dir.z };
  f32 lo = 0.0f, hi = INFINITY;
  for (i32 i = 0; i < 3; i++) {
    if (fabsf(directions[i]) < 1e-8f) {
      if (origins[i] < -1.0f || origins[i] > 1.0f) return false;
      continue;
    }
    f32 a = (-1.0f - origins[i]) / directions[i];
    f32 b = (1.0f - origins[i]) / directions[i];
    lo = fmaxf(lo, fminf(a, b));
    hi = fminf(hi, fmaxf(a, b));
    if (hi < lo) return false;
  }
  *enter = lo;
  *exit = hi;
  return true;
}

static bool near_edge(Vec3 p) {
  i32 faces = (fabsf(p.x) >= 0.985f) + (fabsf(p.y) >= 0.985f) +
      (fabsf(p.z) >= 0.985f);
  return faces >= 2;
}

static Color ray_scene(Ray ray, void* ctx) {
  (void)ctx;
  f32 enter, exit;
  if (!box_interval(ray, &enter, &exit)) return (Color){ 0, 0, 0, 1 };
  f32 light = 0.0f;
  for (f32 t = enter; t < exit; t += MARCH_STEP) {
    f32 length = fminf(MARCH_STEP, exit - t);
    Vec3 g = grid_position(trace_ray(ray, t + 0.5f * length));
    light += expf(-(t - enter) * 1.0f) * length * sample(density, g.x, g.y, g.z);
  }
  if (near_edge(trace_ray(ray, enter)) || near_edge(trace_ray(ray, exit))) {
    light = fmaxf(light, 0.25f);
  }
  return (Color){ light, light, light, 1.0f };
}

static bool mouse_ray(App* app, i32 x, i32 y, Ray* ray) {
  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return false;
  *ray = ray_from_tela_camera(&app->camera, app->tela, (u32)x, (u32)y);
  return true;
}

static bool interaction_point(App* app, i32 x, i32 y, Vec3* point) {
  Ray ray;
  f32 enter, exit;
  if (!mouse_ray(app, x, y, &ray) || !box_interval(ray, &enter, &exit)) return false;
  Ray center_ray = ray_from_tela_camera(&app->camera, app->tela, WIDTH / 2, HEIGHT / 2);
  f32 denom = dot_vec3(center_ray.dir, ray.dir);
  if (fabsf(denom) < 1e-6f) return false;
  f32 t = -dot_vec3(center_ray.dir, ray.init) / denom;
  if (t < enter || t > exit) return false;
  *point = trace_ray(ray, t);
  return true;
}

static void on_frame(f32 dt, f32 time, void* ctx) {
  (void)time;
  App* app = (App*)ctx;
  step(dt);
  ray_map_camera_parallel(&app->camera, app->tela, ray_scene, app);
  char title[160];
  snprintf(title, sizeof(title),
      "3D Fluid | Left: density | Right inside: velocity, outside: orbit | FPS: %.0f",
      dt > 0.0f ? 1.0f / dt : 0.0f);
  set_window_title(app->window, title);
  paint_window(app->window, app->tela);
}

static void on_close(Window* window, void* ctx) {
  (void)window;
  stop_loop((Loop*)ctx);
}

static void on_mouse_down(Window* window, i32 x, i32 y, u32 button, void* ctx) {
  (void)window;
  App* app = (App*)ctx;
  app->mouse = vec2((f32)x, (f32)y);
  if (button == SDL_BUTTON_LEFT) {
    app->left_down = true;
    app->has_point = interaction_point(app, x, y, &app->point);
    if (app->has_point) brush(app->point, true, vec3(0, 0, 0));
  } else if (button == SDL_BUTTON_RIGHT) {
    Ray ray;
    f32 enter, exit;
    bool hit = mouse_ray(app, x, y, &ray) && box_interval(ray, &enter, &exit);
    app->right_drag = hit ? RIGHT_FLUID : RIGHT_ORBIT;
    app->has_point = hit && interaction_point(app, x, y, &app->point);
  }
}

static void on_mouse_up(Window* window, i32 x, i32 y, u32 button, void* ctx) {
  (void)window;
  (void)x;
  (void)y;
  App* app = (App*)ctx;
  if (button == SDL_BUTTON_LEFT) app->left_down = false;
  if (button == SDL_BUTTON_RIGHT) app->right_drag = RIGHT_NONE;
  app->has_point = false;
}

static void on_mouse_move(Window* window, i32 x, i32 y, void* ctx) {
  (void)window;
  App* app = (App*)ctx;
  Vec2 mouse = vec2((f32)x, (f32)y);
  Vec2 delta = sub_vec2(mouse, app->mouse);
  if (app->right_drag == RIGHT_ORBIT) {
    Vec3 orbit = get_camera_orbit(&app->camera);
    set_orbit_camera(&app->camera, orbit.x,
        orbit.y - 2.0f * PI * delta.x / WIDTH,
        orbit.z - 2.0f * PI * delta.y / HEIGHT);
    app->has_point = false;
  } else if (app->left_down || app->right_drag == RIGHT_FLUID) {
    Vec3 point;
    if (interaction_point(app, x, y, &point)) {
      if (app->has_point) {
        Vec3 movement = sub_vec3(point, app->point);
        f32 distance = length_vec3(movement) * GRID_SIZE / 2.0f;
        i32 count = (i32)fmaxf(1.0f, ceilf(distance));
        Vec3 velocity = app->right_drag == RIGHT_FLUID
            ? scale_vec3(movement, 18.0f * GRID_SIZE / (2.0f * count))
            : vec3(0, 0, 0);
        for (i32 i = 1; i <= count; i++) {
          Vec3 p = add_vec3(app->point, scale_vec3(movement, (f32)i / count));
          brush(p, app->left_down, velocity);
        }
      } else if (app->left_down) {
        brush(point, true, vec3(0, 0, 0));
      }
      app->point = point;
      app->has_point = true;
    } else {
      app->has_point = false;
    }
  }
  app->mouse = mouse;
}

static void on_mouse_scroll(Window* window, i32 delta_y, void* ctx) {
  (void)window;
  App* app = (App*)ctx;
  Vec3 orbit = get_camera_orbit(&app->camera);
  set_orbit_camera(&app->camera, fmaxf(2.0f, orbit.x + delta_y * 0.1f), orbit.y, orbit.z);
  app->has_point = false;
}

int main(void) {
  Tela* tela = new_tela(WIDTH, HEIGHT);
  Window* window = new_window(WIDTH, HEIGHT, "3D Fluid");
  if (!window) {
    free_tela(tela);
    return 1;
  }
  App app = {
    .tela = tela,
    .window = window,
    .camera = create_camera(vec3(4.0f, 0.0f, 0.0f), vec3(0, 0, 0), 1.0f),
  };
  set_orbit_camera(&app.camera, 4.0f, 0.0f, PI / 6.0f);
  Loop* animation = loop(on_frame, &app);
  on_close_window(window, on_close, animation);
  on_mouse_down_window(window, on_mouse_down, &app);
  on_mouse_up_window(window, on_mouse_up, &app);
  on_mouse_move_window(window, on_mouse_move, &app);
  on_mouse_scroll_window(window, on_mouse_scroll, &app);
  play_loop(animation);
  free_loop(animation);
  free_window(window);
  free_tela(tela);
  return 0;
}
