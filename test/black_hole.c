/**
 * Black Hole Simulation
 *
 * A gravitational lensing demo that simulates light bending around a black hole
 * using ray marching with Euler integration. Features an equirectangular
 * background texture and interactive orbit camera controls via mouse.
 */

#include "../src/index.c"

// gcc -O3 -fopenmp -o app test/black_hole.c -lSDL2 -lm


/* =============================================================================
 * Constants
 * ========================================================================== */

static const u32 WIDTH = 640;
static const u32 HEIGHT = 480;

static const u32 SIMULATION_STEPS = 100;
static const f32 SPEED_OF_LIGHT = 2.3f;
static const f32 SIMULATION_DT = 0.01f;

static const f32 TORUS_MAJOR_RADIUS = 0.5f;
static const f32 TORUS_MINOR_RADIUS = 0.05f;
static const f32 TORUS_HIT_EPSILON = 1e-6f;
static const f32 EVENT_HORIZON_EPSILON = 1e-3f;

/* =============================================================================
 * Types
 * ========================================================================== */

typedef struct {
  Tela *tela;
  Window *window;
  Camera *camera;
  Tela *background;
  Sphere black_hole;
  f32 time;
  bool use_parallel;
} App;

/* =============================================================================
 * Global State (for input handling)
 * ========================================================================== */

static bool g_mouse_down = false;
static Vec2 g_mouse_pos = {0};

/* =============================================================================
 * Signed Distance Functions
 * ========================================================================== */

/**
 * SDF for a torus centered at origin, lying in the XY plane.
 */
static f32 sdf_torus(Vec3 p, f32 r, f32 R) {
  f32 q = length_vec2(vec2(p.x, p.y)) - r;
  return length_vec2(vec2(q, p.z)) - R;
}

/* =============================================================================
 * Background Rendering
 * ========================================================================== */

/**
 * Sample a color from the equirectangular background image using a ray
 * direction.
 */
static Color sample_background(const Tela *bg, Vec3 dir) {
  // atan2 returns [-pi, pi], map to [0, 1]
  f32 theta = atan2f(dir.y, dir.x) / (2.0f * PI) + 0.5f;
  // acos returns [0, pi], map to [0, 1]
  f32 alpha = acosf(clamp(-dir.z, -1.0f, 1.0f)) / PI;

  u32 u = (u32)(theta * bg->width);
  u32 v = (u32)(alpha * bg->height);
  return get_pxl_tela(bg, u, v);
}

/* =============================================================================
 * Ray Scene
 * ========================================================================== */

/**
 * Simulate gravitational lensing around the black hole.
 *
 * Traces a photon through the gravitational field using Euler integration.
 * If the photon hits the accretion disk torus, returns a fiery color modulated
 * by angular position and time. If it crosses the event horizon, returns black.
 * Otherwise, blends a distance-based torus glow with the background.
 */
static Color ray_scene(Ray r, void *ctx) {
  App *app = (App *)ctx;
  Sphere bh = app->black_hole;
  f32 time = app->time;

  Vec3 velocity = scale_vec3(r.dir, SPEED_OF_LIGHT);
  Vec3 position = r.init;

  for (u32 i = 0; i < SIMULATION_STEPS; i++) {
    // Gravitational acceleration: a = r / |r|^2
    Vec3 r_vec = sub_vec3(bh.position, position);
    f32 r_sq = dot_vec3(r_vec, r_vec);
    Vec3 acceleration = scale_vec3(r_vec, 1.0f / r_sq);

    // Euler integration
    velocity = add_vec3(velocity, scale_vec3(acceleration, SIMULATION_DT));
    position = add_vec3(position, scale_vec3(velocity, SIMULATION_DT));

    // Check torus hit (accretion disk)
    f32 torus_dist = sdf_torus(position, TORUS_MAJOR_RADIUS, TORUS_MINOR_RADIUS);
    if (torus_dist < TORUS_HIT_EPSILON) {
      f32 theta = fmodf(atan2f(position.y, position.x) - 2.0f * time, PI);
      f32 angle = expf(-0.25f * theta * theta);
      return (Color){0.9f, 0.7f * angle, 0.25f * angle, 1.0f};
    }

    // Check if photon crossed event horizon (distance to surface)
    f32 dist_to_surface = fmaxf(0.0f, length_vec3(r_vec) - bh.radius);
    if (dist_to_surface < EVENT_HORIZON_EPSILON) {
      return (Color){0.0f, 0.0f, 0.0f, 1.0f};
    }
  }

  // Distance-based torus glow
  f32 torus_dist = sdf_torus(position, TORUS_MAJOR_RADIUS, TORUS_MINOR_RADIUS);
  f32 glow = clamp(0.04f / (torus_dist * torus_dist), 0.0f, 1.0f);
  Color torus_color = {glow, 0.5f * glow, 0.3f * glow, 1.0f};

  // Sample background using deflected ray direction
  Vec3 final_dir = normalize_vec3(velocity);
  Color bg_color = sample_background(app->background, final_dir);

  // Blend torus glow with background
  return scale_color(add_color(torus_color, bg_color), 0.5f);
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
                   format_string("Black Hole | Parallel: %s (P) | FPS: %.1f",
                                 app->use_parallel ? "ON" : "OFF",
                                 1.0f / dt));
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
  f32 new_radius = orbit.x + delta_y * 0.1f;

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
  Window *window = new_window(WIDTH, HEIGHT, "Black Hole");

  // Load background texture
  Tela *background = io_read_image("./assets/universe.jpg");

  // Setup camera
  Camera camera = create_camera(vec3(2.0f, 0.0f, 0.0f), vec3(0, 0, 0), 1.0f);

  // Black hole at origin
  Sphere black_hole = build_sphere(vec3(0.0f, 0.0f, 0.0f), 0.1f);

  // Application state
  App app = {
      .tela = tela,
      .window = window,
      .camera = &camera,
      .background = background,
      .black_hole = black_hole,
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
