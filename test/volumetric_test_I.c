/**
 * Volumetric Rendering Test
 *
 * A volumetric ray marching demo that renders a glowing noise field inside a
 * unit cube. Density accumulates along each ray using exponential attenuation.
 * Features interactive orbit camera controls via mouse.
 */

#include "../src/index.c"

// gcc -O3 -fopenmp -o app test/volumetric_test_I.c -lSDL2 -lm

/* =============================================================================
 * Constants
 * ========================================================================== */

static const u32 WIDTH = 640;
static const u32 HEIGHT = 480;

static const f32 MARCH_STEP = 0.05f;
static const f32 MAX_DISTANCE = 10.0f;
static const f32 SURFACE_EPSILON = 1e-3f;
static const f32 ALPHA_DENSITY = 1.0f;

/* =============================================================================
 * Types
 * ========================================================================== */

typedef struct {
  Tela *tela;
  Window *window;
  Camera *camera;
  f32 time;
  bool use_parallel;
} App;

/* =============================================================================
 * Global State (for input handling)
 * ========================================================================== */

static bool g_mouse_down = false;
static Vec2 g_mouse_pos = {0};

/* =============================================================================
 * Scene
 * ========================================================================== */

/** Unit cube centered at origin: [-1, 1]^3 */
static const AABB g_box = {
    .min = {-1.0f, -1.0f, -1.0f},
    .max = {1.0f, 1.0f, 1.0f},
    .center = {0.0f, 0.0f, 0.0f},
    .diagonal = {2.0f, 2.0f, 2.0f},
};

/**
 * SDF for the unit cube.
 */
static f32 sdf_box(Vec3 p) { return distance_aabb(&g_box, p); }

/**
 * Volumetric noise function: radial sine wave.
 */
static f32 noise(Vec3 p, f32 t) {
  return (sinf(10.0f * length_vec3(p) - t) + 1.0f) * 0.5f;
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
static Color ray_scene(Ray r, void *ctx) {
  App *app = (App *)ctx;
  f32 time = app->time;
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
      return (Color){0.0f, 0.0f, 0.0f, 1.0f};
    }
  }

  // Phase 2: march through the volume, accumulating density
  f32 t0 = t;
  f32 density = 0.0f;

  for (u32 i = 0; i < max_volume_steps; i++) {
    t += MARCH_STEP;
    p = trace_ray(r, t);
    f32 d = sdf_box(p);

    if (d < 0.0f) {
      density +=
          expf(-(t - t0) * ALPHA_DENSITY) * noise(p, 2.0f * time) * MARCH_STEP;
    }
  }

  return (Color){density, density, density, 1.0f};
}

/* =============================================================================
 * Animation Loop
 * ========================================================================== */

static void on_frame(f32 dt, f32 time, void *ctx) {
  App *app = (App *)ctx;
  app->time = time;

  // Render
  if (app->use_parallel) {
    ray_map_camera_parallel(app->camera, app->tela, ray_scene, app);
  } else {
    ray_map_camera(app->camera, app->tela, ray_scene, app);
  }

  // Display
  set_window_title(app->window,
                   format_string("Volumetric | Parallel: %s (P) | FPS: %.1f",
                                 app->use_parallel ? "ON" : "OFF", 1.0f / dt));
  paint_window(app->window, app->tela);
}

static void on_close(Window *window, void *ctx) {
  Loop *animation = (Loop *)ctx;
  stop_loop(animation);
}

/* =============================================================================
 * Input Handlers
 * ========================================================================== */

static void on_mouse_down(Window *window, i32 x, i32 y, u32 button,
                          void *ctx) {
  g_mouse_down = true;
  g_mouse_pos = vec2((f32)x, (f32)y);
}

static void on_mouse_up(Window *window, i32 x, i32 y, u32 button, void *ctx) {
  g_mouse_down = false;
}

static void on_mouse_move(Window *window, i32 x, i32 y, void *ctx) {
  if (!g_mouse_down)
    return;

  Vec2 new_pos = vec2((f32)x, (f32)y);
  if (equals_vec2(new_pos, g_mouse_pos))
    return;

  App *app = (App *)ctx;
  Vec2 delta = sub_vec2(new_pos, g_mouse_pos);

  Vec3 orbit = get_camera_orbit(app->camera);
  f32 theta_delta = -2.0f * PI * (delta.x / WIDTH);
  f32 phi_delta = -2.0f * PI * (delta.y / HEIGHT);

  set_orbit_camera(app->camera, orbit.x, orbit.y + theta_delta,
                   orbit.z + phi_delta);

  g_mouse_pos = new_pos;
}

static void on_mouse_scroll(Window *window, i32 delta_y, void *ctx) {
  App *app = (App *)ctx;

  Vec3 orbit = get_camera_orbit(app->camera);
  f32 new_radius = orbit.x + delta_y * 0.5f;
  if (new_radius < 0.1f)
    new_radius = 0.1f;

  set_orbit_camera(app->camera, new_radius, orbit.y, orbit.z);
}

static void on_key_down(Window *window, u32 keycode, void *ctx) {
  App *app = (App *)ctx;
  if (keycode == SDLK_p) {
    app->use_parallel = !app->use_parallel;
  }
}

static void register_input_handlers(Window *window, App *app) {
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
  // Create window and canvas
  Tela *tela = new_tela(WIDTH, HEIGHT);
  Window *window = new_window(WIDTH, HEIGHT, "Volumetric Test");

  // Setup camera
  Camera camera = create_camera(vec3(3.0f, 0.0f, 0.0f), vec3(0, 0, 0), 1.0f);

  // Application state
  App app = {
      .tela = tela,
      .window = window,
      .camera = &camera,
      .use_parallel = true,
  };

  // Animation loop
  Loop *animation = loop(on_frame, &app);

  // Event handlers
  on_close_window(window, on_close, animation);
  register_input_handlers(window, &app);

  // Run
  play_loop(animation);

  return 0;
}
