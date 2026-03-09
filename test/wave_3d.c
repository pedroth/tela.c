/**
 * Volumetric Rendering Test
 *
 * A volumetric ray marching demo that renders a glowing noise field inside a
 * unit cube. Density accumulates along each ray using exponential attenuation.
 * Features interactive orbit camera controls via mouse.
 */

#include "../src/index.c"

/* =============================================================================
 * Constants
 * ========================================================================== */

static const u32 WIDTH = 640;
static const u32 HEIGHT = 480;

static const f32 MARCH_STEP = 0.05f;
static const f32 MAX_DISTANCE = 10.0f;
static const f32 SURFACE_EPSILON = 1e-3f;
static const f32 ALPHA_DENSITY = 1.0f;

#define GRID_SIZE 50
#define AMPLITUDE 10.0f
#define FRICTION 0.2f
#define WAVE_SPEED 25.0f
#define SPREAD 800.0f

/* =============================================================================
 * Utilities
 * ========================================================================== */

static u32 wrap(i32 n, i32 m) {
  return (u32)(((n % m) + m) % m);
}

/* =============================================================================
 * Wave State
 * ========================================================================== */

static f32 g_density[GRID_SIZE][GRID_SIZE][GRID_SIZE];
static f32 g_velocity[GRID_SIZE][GRID_SIZE][GRID_SIZE];

static void initialize_wave(void) {
  for (u32 i = 0; i < GRID_SIZE; i++) {
    for (u32 j = 0; j < GRID_SIZE; j++) {
      for (u32 k = 0; k < GRID_SIZE; k++) {
        f32 x = (j - GRID_SIZE / 2.0f) / (f32)GRID_SIZE;
        f32 y = (i - GRID_SIZE / 2.0f) / (f32)GRID_SIZE;
        f32 z = (k - GRID_SIZE / 2.0f) / (f32)GRID_SIZE;
        f32 bump1 = AMPLITUDE *
                    expf(-SPREAD * ((x - 0.25f) * (x - 0.25f) + y * y + z * z));
        f32 bump2 = AMPLITUDE *
                    expf(-SPREAD * ((x + 0.25f) * (x + 0.25f) + y * y + z * z));
        f32 bump3 = AMPLITUDE *
                    expf(-SPREAD * (x * x + (y - 0.25f) * (y - 0.25f) + z * z));
        f32 bump4 = AMPLITUDE *
                    expf(-SPREAD * (x * x + y * y + (z + 0.25f) * (z + 0.25f)));
        g_density[i][j][k] = bump1 + bump2 + bump3 + bump4;
        g_velocity[i][j][k] = 0.0f;
      }
    }
  }
}

/* =============================================================================
 * Types
 * ========================================================================== */
typedef struct {
  f32 min_height;
  f32 max_height;
  f32 max_abs_velocity;
} WaveShaderContext;

typedef struct {
  Tela* tela;
  Window* window;
  Camera* camera;
  WaveShaderContext* shader_ctx;
} App;

/* =============================================================================
 * Global State (for input handling)
 * ========================================================================== */

static bool g_left_mouse_down = false;
static bool g_right_mouse_down = false;
static Vec2 g_mouse_pos = { 0 };

/* =============================================================================
 * Scene
 * ========================================================================== */

/** Unit cube centered at origin: [-1, 1]^3 */
static const AABB g_box = {
  .min = { -1.0f, -1.0f, -1.0f },
  .max = { 1.0f, 1.0f, 1.0f },
  .center = { 0.0f, 0.0f, 0.0f },
  .diagonal = { 2.0f, 2.0f, 2.0f },
};

/**
 * SDF for the unit cube.
 */
static f32 sdf_box(Vec3 p) {
  return distance_aabb(&g_box, p);
}

static f32 density(Vec3 p) {
  return g_density[wrap((i32)((p.y + 1.0f) / 2.0f * GRID_SIZE), GRID_SIZE)]
                  [wrap((i32)((p.x + 1.0f) / 2.0f * GRID_SIZE), GRID_SIZE)]
                  [wrap((i32)((p.z + 1.0f) / 2.0f * GRID_SIZE), GRID_SIZE)];
}

static f32 velocity_at(Vec3 p) {
  return g_velocity[wrap((i32)((p.y + 1.0f) / 2.0f * GRID_SIZE), GRID_SIZE)]
                   [wrap((i32)((p.x + 1.0f) / 2.0f * GRID_SIZE), GRID_SIZE)]
                   [wrap((i32)((p.z + 1.0f) / 2.0f * GRID_SIZE), GRID_SIZE)];
}

/* =============================================================================
 * Ray Scene
 * ========================================================================== */

/**
 * Volumetric ray marcher.
 *
 * First, sphere-traces to find the box surface. Then marches through the
 * interior in fixed steps, accumulating density weighted by exponential
 * attenuation and a noise function.
 */
static Color ray_scene(Ray r, void* ctx) {
  App* app = (App*)ctx;
  WaveShaderContext* shader_ctx = app->shader_ctx;
  f32 range = shader_ctx->max_height - shader_ctx->min_height;
  if (range < 1e-6f)
    range = 1e-6f;
  f32 max_v = shader_ctx->max_abs_velocity;
  if (max_v < 1e-6f)
    max_v = 1e-6f;
  u32 max_volume_steps = (u32)(2.0f / MARCH_STEP);

  // Phase 1: sphere-trace to the box surface
  Vec3 p = r.init;
  f32 t = sdf_box(p);

  for (u32 i = 0; i < (u32)MAX_DISTANCE; i++) {
    p = trace_ray(r, t);
    f32 d = sdf_box(p);
    t += d;

    if (d < SURFACE_EPSILON) {
      break;
    }
    if (d > MAX_DISTANCE) {
      return (Color){ 0.0f, 0.0f, 0.0f, 1.0f };
    }
  }

  // Phase 2: march through the volume, accumulating wave-style color
  f32 t0 = t;
  f32 acc_red = 0.0f, acc_green = 0.0f, acc_blue = 0.0f;

  for (u32 i = 0; i < max_volume_steps; i++) {
    t += MARCH_STEP;
    p = trace_ray(r, t);
    f32 d = sdf_box(p);

    if (d < 0.0f) {
      f32 local_density = density(p);
      f32 normalized = (local_density - shader_ctx->min_height) / range;
      normalized = clamp(normalized, 0.0f, 1.0f);

      f32 local_velocity = fabsf(velocity_at(p));

      // Wave-style coloring: red=high, blue=low, green=speed
      f32 red = normalized;
      f32 blue = 1.0f - normalized;
      f32 green = local_velocity / max_v;


      // Accumulate with exponential attenuation for transparency
      f32 weight = expf(-(t - t0) * ALPHA_DENSITY) * MARCH_STEP;
      acc_red += red * weight;
      acc_green += green * weight;
      acc_blue += blue * weight;
    }
  }

  return (Color){ acc_red, acc_green, acc_blue, 1.0f };
}

/* =============================================================================
 * Animation Loop
 * ========================================================================== */

static void on_frame(f32 dt, f32 time, void* ctx) {
  App* app = (App*)ctx;

  // Clamp dt to maintain numerical stability (CFL condition)
  if (dt > 1.0f / 60.0f)
    dt = 1.0f / 60.0f;

  f32 max_h = -__FLT_MAX__;
  f32 min_h = __FLT_MAX__;
  f32 max_v = -__FLT_MAX__;

  // Symplectic Euler integration of the 3D wave equation
  // Pass 1: update all velocities using current densities
  for (u32 i = 0; i < GRID_SIZE; i++) {
    for (u32 j = 0; j < GRID_SIZE; j++) {
      for (u32 k = 0; k < GRID_SIZE; k++) {
        // Discrete Laplacian (toroidal boundary)
        f32 laplacian = g_density[i][wrap((i32)j + 1, GRID_SIZE)][k] +
                        g_density[i][wrap((i32)j - 1, GRID_SIZE)][k] +
                        g_density[wrap((i32)i + 1, GRID_SIZE)][j][k] +
                        g_density[wrap((i32)i - 1, GRID_SIZE)][j][k] +
                        g_density[i][j][wrap((i32)k + 1, GRID_SIZE)] +
                        g_density[i][j][wrap((i32)k - 1, GRID_SIZE)] -
                        6.0f * g_density[i][j][k];

        f32 acceleration =
            WAVE_SPEED * laplacian - FRICTION * g_velocity[i][j][k];
        g_velocity[i][j][k] += dt * acceleration;
      }
    }
  }

  // Pass 2: update all densities using new velocities
  for (u32 i = 0; i < GRID_SIZE; i++) {
    for (u32 j = 0; j < GRID_SIZE; j++) {
      for (u32 k = 0; k < GRID_SIZE; k++) {
        g_density[i][j][k] += dt * g_velocity[i][j][k];

        // Track min/max for shader normalization
        if (g_density[i][j][k] > max_h)
          max_h = g_density[i][j][k];
        if (g_density[i][j][k] < min_h)
          min_h = g_density[i][j][k];
        f32 abs_v = fabsf(g_velocity[i][j][k]);
        if (abs_v > max_v)
          max_v = abs_v;
      }
    }
  }

  app->shader_ctx = &(WaveShaderContext){ min_h, max_h, max_v };
  // Render
  ray_map_camera_parallel(app->camera, app->tela, ray_scene, app);

  // Display
  set_window_title(
      app->window,
      format_string(
          "Volumetric | FPS: %.1f |min: %.2f |max: %.2f |R: reset",
          1.0f / dt,
          app->shader_ctx->min_height,
          app->shader_ctx->max_height
      )
  );
  paint_window(app->window, app->tela);
}

static void on_close(Window* window, void* ctx) {
  Loop* animation = (Loop*)ctx;
  stop_loop(animation);
}

/* =============================================================================
 * Input Handlers
 * ========================================================================== */

static void paint_density(App* app, f32 mx, f32 my) {
  Ray ray = ray_from_tela_camera(app->camera, app->tela, mx, my);
  SceneHit hit = intersect_with_ray_aabb(ray, &g_box);
  if (hit.hit) {
    // Plane through origin, facing the camera
    Vec3 normal =
        ray_from_tela_camera(app->camera, app->tela, WIDTH / 2, HEIGHT / 2).dir;
    f32 denom = dot_vec3(normal, ray.dir);
    if (fabsf(denom) < 1e-6f)
      return;
    f32 t_plane = -dot_vec3(normal, ray.init) / denom;
    Vec3 p = trace_ray(ray, t_plane);

    // Clamp to box bounds [-1, 1]
    if (p.x < -1.0f || p.x > 1.0f || p.y < -1.0f || p.y > 1.0f || p.z < -1.0f ||
        p.z > 1.0f)
      return;

    i32 grid_x = (i32)(((p.x + 1.0f) / 2.0f) * GRID_SIZE);
    i32 grid_y = (i32)(((p.y + 1.0f) / 2.0f) * GRID_SIZE);
    i32 grid_z = (i32)(((p.z + 1.0f) / 2.0f) * GRID_SIZE);

    // Paint a 3D brush (3x3x3)
    i32 brush[] = { -1, 0, 1 };
    u32 brush_size = sizeof(brush) / sizeof(brush[0]);
    for (u32 bi = 0; bi < brush_size; bi++) {
      for (u32 bj = 0; bj < brush_size; bj++) {
        for (u32 bk = 0; bk < brush_size; bk++) {
          u32 gy = wrap(grid_y + brush[bi], GRID_SIZE);
          u32 gx = wrap(grid_x + brush[bj], GRID_SIZE);
          u32 gz = wrap(grid_z + brush[bk], GRID_SIZE);
          g_density[gy][gx][gz] = AMPLITUDE;
        }
      }
    }
  }
}

static void on_mouse_down(Window* window, i32 x, i32 y, u32 button, void* ctx) {
  g_mouse_pos = vec2((f32)x, (f32)y);
  App* app = (App*)ctx;

  if (button == 1) {  // Left click: paint density
    g_left_mouse_down = true;
    paint_density(app, (f32)x, (f32)y);
  } else if (button == 3) {  // Right click: orbit camera
    g_right_mouse_down = true;
  }
}

static void on_mouse_up(Window* window, i32 x, i32 y, u32 button, void* ctx) {
  if (button == 1)
    g_left_mouse_down = false;
  if (button == 3)
    g_right_mouse_down = false;
}

static void on_mouse_move(Window* window, i32 x, i32 y, void* ctx) {
  Vec2 new_pos = vec2((f32)x, (f32)y);
  if (equals_vec2(new_pos, g_mouse_pos))
    return;

  App* app = (App*)ctx;

  // Left drag: paint density
  if (g_left_mouse_down) {
    paint_density(app, (f32)x, (f32)y);
  }

  // Right drag: orbit camera
  if (g_right_mouse_down) {
    Vec2 delta = sub_vec2(new_pos, g_mouse_pos);

    Vec3 orbit = get_camera_orbit(app->camera);
    f32 theta_delta = -2.0f * PI * (delta.x / WIDTH);
    f32 phi_delta = -2.0f * PI * (delta.y / HEIGHT);

    set_orbit_camera(
        app->camera, orbit.x, orbit.y + theta_delta, orbit.z + phi_delta
    );
  }

  g_mouse_pos = new_pos;
}

static void on_mouse_scroll(Window* window, i32 delta_y, void* ctx) {
  App* app = (App*)ctx;

  Vec3 orbit = get_camera_orbit(app->camera);
  f32 new_radius = orbit.x + delta_y * 0.5f;
  if (new_radius < 0.1f)
    new_radius = 0.1f;

  set_orbit_camera(app->camera, new_radius, orbit.y, orbit.z);
}

static void on_key_down(Window* window, u32 keycode, void* ctx) {
  if (keycode == 'r') {
    for (u32 i = 0; i < GRID_SIZE; i++) {
      for (u32 j = 0; j < GRID_SIZE; j++) {
        for (u32 k = 0; k < GRID_SIZE; k++) {
          g_density[i][j][k] = 0.0f;
          g_velocity[i][j][k] = 0.0f;
        }
      }
    }
  }
}

static void register_input_handlers(Window* window, App* app) {
  on_mouse_down_window(window, on_mouse_down, app);
  on_mouse_up_window(window, on_mouse_up, app);
  on_mouse_move_window(window, on_mouse_move, app);
  on_mouse_scroll_window(window, on_mouse_scroll, app);
  on_key_down_window(window, on_key_down, app);
}

/* =============================================================================
 * Main
 * ========================================================================== */

int main(void) {
  // Initialize 3D wave
  initialize_wave();

  // Create window and canvas
  Tela* tela = new_tela(WIDTH, HEIGHT);
  Window* window = new_window(WIDTH, HEIGHT, "Volumetric Test");

  // Setup camera
  Camera camera = create_camera(vec3(3.0f, 0.0f, 0.0f), vec3(0, 0, 0), 1.0f);

  // Application state
  App app = {
    .tela = tela,
    .window = window,
    .camera = &camera,
    .shader_ctx = NULL,
  };

  // Animation loop
  Loop* animation = loop(on_frame, &app);

  // Event handlers
  on_close_window(window, on_close, animation);
  register_input_handlers(window, &app);

  // Run
  play_loop(animation);

  return 0;
}
