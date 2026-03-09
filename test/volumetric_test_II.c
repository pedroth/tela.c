/**
 * Volumetric Noise Sphere
 *
 * A volumetric ray marching demo that renders an animated noise field inside a
 * unit sphere. Density accumulates along each ray using exponential attenuation.
 * Color encodes density via a heat-map palette.
 * Features interactive orbit camera controls via mouse.
 *
 * Port from tela.js volumetric test.
 */

#include "../src/index.c"

// gcc -O3 -fopenmp -o app test/volumetric_test_II.c -lSDL2 -lm

/* =============================================================================
 * Constants
 * ========================================================================== */

static const u32 WIDTH = 640 / 2;
static const u32 HEIGHT = 480 / 2;

static const f32 MARCH_STEP = 0.01f;
static const f32 MAX_DISTANCE = 10.0f;
static const f32 SURFACE_EPSILON = 1e-3f;
static const f32 ALPHA_DENSITY = 0.01f;

#define NUM_COEFFS 10

/* =============================================================================
 * Types
 * ========================================================================== */

typedef struct {
  Tela *tela;
  Window *window;
  Camera *camera;
  f32 time;
  Vec3 coeffs[NUM_COEFFS];
  f32 phases[NUM_COEFFS];
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

/** SDF for a unit sphere at origin. */
static f32 sdf_sphere(Vec3 p) { return length_vec3(p) - 1.0f; }

/**
 * Noise function: sum of harmonics using random coefficients and phases.
 *
 * noise(p) = Sum_i  halves^i * |sin(2*PI*i * dot(p, coeffs[i]) - phases[i] - time)|
 *
 * Returns value in [0, 1] via frac().
 */
static f32 noise(Vec3 p, const Vec3 *coeffs, const f32 *phases, f32 time) {
  f32 acc = 0.0f;
  f32 halves = 0.7f;
  for (u32 i = 0; i < NUM_COEFFS; i++) {
    acc += halves * sinf(2.0f * PI * (f32)i * dot_vec3(p, coeffs[i]) - phases[i] - time);
    halves *= halves;
  }
  // frac: acc - floor(acc), then abs to keep in [0,1]
  f32 frac = acc - floorf(acc);
  return fabsf(frac);
}

/**
 * Heat-map palette: blue -> cyan -> green -> yellow -> red.
 */
static Color palette(f32 density) {
  f32 t = clamp(density, 0.0f, 1.0f);

  // Color stop table
  static const f32 stops[][4] = {
    /* t     r     g     b   */
    { 0.00f, 0.0f, 0.0f, 1.0f },  // Blue
    { 0.25f, 0.0f, 1.0f, 1.0f },  // Cyan
    { 0.50f, 0.0f, 1.0f, 0.0f },  // Green
    { 0.75f, 1.0f, 1.0f, 0.0f },  // Yellow
    { 1.00f, 1.0f, 0.0f, 0.0f },  // Red
  };
  static const u32 NUM_STOPS = 5;

  // Find the two stops we're between
  u32 i = 0;
  while (i < NUM_STOPS - 2 && stops[i + 1][0] <= t) {
    i++;
  }

  f32 range = stops[i + 1][0] - stops[i][0];
  f32 factor = (range < 1e-6f) ? 0.0f : (t - stops[i][0]) / range;

  f32 r = stops[i][1] + (stops[i + 1][1] - stops[i][1]) * factor;
  f32 g = stops[i][2] + (stops[i + 1][2] - stops[i][2]) * factor;
  f32 b = stops[i][3] + (stops[i + 1][3] - stops[i][3]) * factor;

  // Multiply by t for brightness falloff (matching JS: t * color)
  return (Color){t * r, t * g, t * b, 1.0f};
}

/* =============================================================================
 * Ray Scene
 * ========================================================================== */

/**
 * Volumetric ray marcher.
 *
 * First, sphere-traces to find the sphere surface. Then marches through the
 * interior in fixed steps, accumulating density weighted by exponential
 * attenuation and a noise function.
 */
static Color ray_scene(Ray r, void *ctx) {
  App *app = (App *)ctx;
  f32 time = app->time;
  u32 max_volume_steps = (u32)(2.0f / MARCH_STEP);

  // Phase 1: sphere-trace to the sphere surface
  Vec3 p = r.init;
  f32 t = sdf_sphere(p);

  for (u32 i = 0; i < (u32)MAX_DISTANCE; i++) {
    p = trace_ray(r, t);
    f32 d = sdf_sphere(p);
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
  f32 density_acc = 0.0f;

  for (u32 i = 0; i < max_volume_steps; i++) {
    t += MARCH_STEP;
    p = trace_ray(r, t);
    f32 d = sdf_sphere(p);

    if (d < 0.0f) {
      density_acc +=
          expf(-(t - t0) * ALPHA_DENSITY) * MARCH_STEP *
          noise(p, app->coeffs, app->phases, time);
    }
  }

  return palette(density_acc);
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
                   format_string("Volumetric Noise | Parallel: %s (P) | FPS: %.1f",
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
  f32 new_radius = orbit.x + delta_y * 0.001f;
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
  // Generate random coefficients and phases (matching JS: Vec.RANDOM(3) * 2 - 1, normalized)
  Vec3 coeffs[NUM_COEFFS];
  f32 phases[NUM_COEFFS];
  for (u32 i = 0; i < NUM_COEFFS; i++) {
    Vec3 rv = sub_vec3(scale_vec3(random_vec3(), 2.0f), vec3(1.0f, 1.0f, 1.0f));
    Vec3 normalized_rv = normalize_vec3(rv);
    coeffs[i] = normalized_rv;
    phases[i] = 2.0f * PI * (f32)random_double();
  }

  // Create window and canvas
  Tela *tela = new_tela(WIDTH, HEIGHT);
  Window *window = new_window(WIDTH*2, HEIGHT*2, "Volumetric Noise");

  // Setup camera
  Camera camera = create_camera(vec3(3.0f, 0.0f, 0.0f), vec3(0, 0, 0), 1.0f);

  // Application state
  App app = {
      .tela = tela,
      .window = window,
      .camera = &camera,
      .time = 0.0f,
      .use_parallel = true,
  };
  for (u32 i = 0; i < NUM_COEFFS; i++) {
    app.coeffs[i] = coeffs[i];
    app.phases[i] = phases[i];
  }

  // Animation loop
  Loop *animation = loop(on_frame, &app);

  // Event handlers
  on_close_window(window, on_close, animation);
  register_input_handlers(window, &app);

  // Run
  play_loop(animation);

  return 0;
}
