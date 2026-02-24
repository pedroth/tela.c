/**
 * SDF Test Window
 *
 * A raymarching demo that renders a morphing torus-cube using signed distance
 * functions. Features interactive orbit camera controls via mouse.
 * 
 * gcc -O3 -fopenmp -o app test/sdf_test_parallel.c -lSDL2 -lm
 */

#include "../src/index.c"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* =============================================================================
 * Constants
 * ========================================================================== */

static const u32 WIDTH = 640;
static const u32 HEIGHT = 480;

static const u32 MAX_RAYMARCH_ITERATIONS = 100;
static const f32 MAX_RAYMARCH_DISTANCE = 10.0f;
static const f32 RAYMARCH_EPSILON = 1e-3f;
static const f32 NORMAL_EPSILON = 1e-3f;

/* =============================================================================
 * Types
 * ========================================================================== */

typedef struct {
  Tela *tela;
  Window *window;
  Camera *camera;
} App;

typedef struct {
  f32 time;
  Vec3 light_pos;
} SceneContext;

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
 * @param p     Point to evaluate
 * @param r     Major radius (distance from center to tube center)
 * @param R     Minor radius (tube thickness)
 */
static f32 sdf_torus(Vec3 p, f32 r, f32 R) {
  f32 q = length_vec2(vec2(p.x, p.y)) - r;
  return length_vec2(vec2(q, p.z)) - R;
}

/**
 * SDF for a rounded cube (cube with spherical carving).
 * @param p         Point to evaluate
 * @param box       Axis-aligned bounding box
 * @param sphere    Sphere to subtract from the box
 */
static f32 sdf_rounded_cube(Vec3 p, const AABB *box, Vec3 sphere_pos,
                            f32 sphere_radius) {
  f32 box_dist = distance_aabb(box, p);
  f32 sphere_dist = length_vec3(sub_vec3(sphere_pos, p)) - sphere_radius;
  return fmaxf(box_dist, -sphere_dist);
}

/**
 * Composite scene SDF - morphs between torus and rounded cube over time.
 */
static f32 sdf_scene(Vec3 p, f32 time) {
  // Torus parameters
  const f32 torus_major = 0.5f;
  const f32 torus_minor = 0.25f;

  // Rounded cube parameters
  AABB box = build_aabb(vec3(-0.5f, -0.5f, -0.5f), vec3(0.5f, 0.5f, 0.5f));
  Vec3 sphere_pos = vec3(0.0f, 0.0f, 0.0f);
  f32 sphere_radius = 0.65f;

  // Morph factor: oscillates between 0 and 1
  f32 morph = (sinf(2.0f * M_PI * 0.25f * (time - 1.0f)) + 1.0f) * 0.5f;

  f32 torus_dist = sdf_torus(p, torus_major, torus_minor);
  f32 cube_dist = sdf_rounded_cube(p, &box, sphere_pos, sphere_radius);

  return morph * torus_dist + (1.0f - morph) * cube_dist;
}

/**
 * Compute surface normal using central differences.
 */
static Vec3 sdf_normal(Vec3 p, f32 time) {
  const f32 eps = NORMAL_EPSILON;
  f32 center = sdf_scene(p, time);

  Vec3 gradient = vec3(sdf_scene(add_vec3(p, vec3(eps, 0, 0)), time) - center,
                       sdf_scene(add_vec3(p, vec3(0, eps, 0)), time) - center,
                       sdf_scene(add_vec3(p, vec3(0, 0, eps)), time) - center);

  Vec3 normal = {0};
  normalize_vec3(gradient, &normal);
  return normal;
}

/* =============================================================================
 * Ray Marching
 * ========================================================================== */

/**
 * Ray march the scene and compute lighting.
 */
static Color ray_march(Ray ray, f32 time, Vec3 light_pos) {
  Vec3 p = ray.init;
  f32 t = sdf_scene(p, time);

  for (u32 i = 0; i < MAX_RAYMARCH_ITERATIONS; i++) {
    p = trace_ray(ray, t);
    f32 dist = sdf_scene(p, time);
    t += dist;

    // Hit surface
    if (dist < RAYMARCH_EPSILON) {
      Vec3 normal = sdf_normal(p, time);
      Vec3 to_light = sub_vec3(light_pos, p);
      Vec3 light_dir = {0};
      normalize_vec3(to_light, &light_dir);

      f32 diffuse = fmaxf(0.0f, dot_vec3(normal, light_dir));
      return (Color){diffuse, 0.0f, 0.0f, 1.0f};
    }

    // Escaped to infinity
    if (dist > MAX_RAYMARCH_DISTANCE) {
      f32 c = 2.0f * (f32)i / MAX_RAYMARCH_ITERATIONS;
      return (Color){c, c, c, 1.0f};
    }
  }

  return (Color){0.0f, 0.0f, 0.0f, 1.0f};
}

/**
 * Ray scene callback for camera ray mapping.
 */
static Color ray_callback(Ray ray, void *ctx) {
  SceneContext *scene = (SceneContext *)ctx;
  return ray_march(ray, scene->time, scene->light_pos);
}

/* =============================================================================
 * Animation Loop
 * ========================================================================== */

static void on_frame(f32 dt, f32 time, void *ctx) {
  App *app = (App *)ctx;

  // Rotating light source
  Vec3 light_pos = scale_vec3(vec3(cosf(time), sinf(time), 1.0f), 2.0f);
  SceneContext scene = {.time = time, .light_pos = light_pos};

  // Render
  ray_map_camera_parallel(app->camera, app->tela, ray_callback, &scene);

  // Display
  set_window_title(app->window,
                   format_string("SDF Demo | FPS: %.1f", 1.0f / dt));
  paint_window(app->window, app->tela);
}

static void on_close(Window *window, void *ctx) {
  Loop *animation = (Loop *)ctx;
  stop_loop(animation);
}

/* =============================================================================
 * Input Handlers
 * ========================================================================== */

static void on_mouse_down(Window *window, i32 x, i32 y, u32 button, void *ctx) {
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
  f32 theta_delta = -2.0f * M_PI * (delta.x / WIDTH);
  f32 phi_delta = -2.0f * M_PI * (delta.y / HEIGHT);

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

static void register_input_handlers(Window *window, App *app) {
  on_mouse_down_window(window, on_mouse_down, app);
  on_mouse_up_window(window, on_mouse_up, app);
  on_mouse_move_window(window, on_mouse_move, app);
  on_mouse_scroll_window(window, on_mouse_scroll, app);
}

/* =============================================================================
 * Main
 * ========================================================================== */

int main(void) {
  // Create window and canvas
  Tela *tela = new_tela(WIDTH, HEIGHT);
  Window *window = new_window(WIDTH, HEIGHT, "SDF Test");

  // Setup camera
  Camera camera = create_camera(vec3(3.0f, 0.0f, 0.0f), vec3(0, 0, 0), 1.0f);

  // Application state
  App app = {
      .tela = tela,
      .window = window,
      .camera = &camera,
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
