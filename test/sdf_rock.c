/**
 *
 * Inspired by https://youtu.be/6Qb6QtC6QMs?si=DRkw9tmLYMkKEzmE
 *
 */

#include "../src/index.c"

// gcc -O3 -fopenmp -o app test/sdf_rock.c -lSDL2 -lm

/* =============================================================================
 * Constants
 * ========================================================================== */

static const u32 WIDTH = 640;
static const u32 HEIGHT = 480;

static const u32 MAX_RAYMARCH_ITERATIONS = 20;
static const f32 MAX_RAYMARCH_DISTANCE = 20.0f;
static const f32 RAYMARCH_EPSILON = 0.1f;
static const f32 NORMAL_EPSILON = 1e-6f;

static const u32 NUM_HYPERPLANES = 15;

/* =============================================================================
 * Types
 * ========================================================================== */

typedef struct {
  Vec3 a;  // point on the plane
  Vec3 n;  // normal vector of the plane
} HyperPlane;

typedef struct {
  Tela *tela;
  Window *window;
  Camera *camera;
  bool use_parallel;
  HyperPlane *hyperPlanes;
  Tela *background;
} App;

typedef struct {
  f32 time;
  Vec3 light_pos;
  HyperPlane *hyperPlanes;
  Tela *background;
} SceneContext;

/* =============================================================================
 * Global State (for input handling)
 * ========================================================================== */

static bool g_mouse_down = false;
static Vec2 g_mouse_pos = { 0 };

/* =============================================================================
 * Background
 * ========================================================================== */

static Color sample_background(const Tela *bg, Vec3 dir) {
  f32 theta = atan2f(dir.y, dir.x) / (2.0f * PI) + 0.5f;
  f32 alpha = acosf(clamp(-dir.z, -1.0f, 1.0f)) / PI;
  u32 u = (u32)(theta * bg->width);
  u32 v = (u32)(alpha * bg->height);
  return get_pxl_tela(bg, u, v);
}

/* =============================================================================
 * Signed Distance Functions
 * ========================================================================== */

f32 frac(f32 x);
Vec3 frac_vec3(Vec3 p);
f32 mix(f32 x, f32 y, f32 t);

f32 hash(Vec3 p) {
  Vec3 np = add_vec3(p, vec3(1000, 1000, 1000));
  return frac(
      123 * sinf(np.x * 21.6f) * sinf(np.y * 43.4f) *
      sinf(np.z * 14.7f)
  );
}

f32 cubic_spline_smooth(f32 x) {
  return x * x * (3 - 2 * x);
}

f32 cnoise(Vec3 p) {
  Vec3 ip = map_vec3(p, floorf);
  Vec3 fp = frac_vec3(p);
  const Vec3 u = map_vec3(fp, cubic_spline_smooth);
  const f32 a = hash(ip);
  const f32 b = hash(add_vec3(ip, vec3(1, 0, 0)));
  const f32 c = hash(add_vec3(ip, vec3(0, 1, 0)));
  const f32 d = hash(add_vec3(ip, vec3(1, 1, 0)));
  const f32 e = hash(add_vec3(ip, vec3(0, 0, 1)));
  const f32 f = hash(add_vec3(ip, vec3(1, 0, 1)));
  const f32 g = hash(add_vec3(ip, vec3(0, 1, 1)));
  const f32 h = hash(add_vec3(ip, vec3(1, 1, 1)));
  const f32 x1 = mix(a, b, u.x);
  const f32 x2 = mix(c, d, u.x);
  const f32 y1 = mix(x1, x2, u.y);
  const f32 x3 = mix(e, f, u.x);
  const f32 x4 = mix(g, h, u.x);
  const f32 y2 = mix(x3, x4, u.y);
  return mix(y1, y2, u.z);
}

f32 mix(f32 x, f32 y, f32 t) {
  return x * (1 - t) + y * t;
}

// Fractal Brownian Motion (FBM) noise function
f32 fbm(const Vec3 p) {
  f32 a = 0.5;
  f32 f = 0;
  Vec3 pi = vec3(p.x, p.y, p.z);
  for (u32 i = 0; i < 4; i++) {
    f += a * cnoise(pi);
    a *= 0.51;
    pi = scale_vec3(pi, 1.99);
    pi = add_vec3(pi, vec3(0.1, 0.1, 0.1));
  }
  return f;
}

f32 soft_max(f32 x, f32 y, f32 epsilon) {
  return (x + y + sqrtf((x - y) * (x - y) + epsilon)) / 2.0f;
}

f32 frac(f32 x) {
  return x - floorf(x);
}

Vec3 frac_vec3(Vec3 p) {
  return map_vec3(p, frac);
}

f32 half_plane_sdf(Vec3 p, Vec3 a, Vec3 n) {
  return dot_vec3(sub_vec3(p, a), n);
}

/**
 * Composite scene SDF - morphs between torus and rounded cube over time.
 */
static f32 sdf_scene(Vec3 p, f32 time, HyperPlane *hyperPlanes) {
  if (hyperPlanes == NULL)
    return 0;
  f32 minD = half_plane_sdf(p, hyperPlanes[0].a, hyperPlanes[0].n);
  for (u32 i = 1; i < NUM_HYPERPLANES; i++) {
    HyperPlane hp = hyperPlanes[i];
    minD = soft_max(minD, half_plane_sdf(p, hp.a, hp.n), 0.01f);
  }
  return minD + fbm(scale_vec3(p, 1 / 0.15)) * 0.07;
}

/**
 * Compute surface normal using central differences.
 */
static Vec3 sdf_normal(Vec3 p, f32 time, HyperPlane *hyperPlanes) {
  const f32 eps = NORMAL_EPSILON;
  f32 center = sdf_scene(p, time, hyperPlanes);

  Vec3 gradient = vec3(
      sdf_scene(add_vec3(p, vec3(eps, 0, 0)), time, hyperPlanes) - center,
      sdf_scene(add_vec3(p, vec3(0, eps, 0)), time, hyperPlanes) - center,
      sdf_scene(add_vec3(p, vec3(0, 0, eps)), time, hyperPlanes) - center
  );

  Vec3 normal = normalize_vec3(gradient);
  return normal;
}

/* =============================================================================
 * Ray Marching
 * ========================================================================== */

/**
 * Ray march the scene and compute lighting.
 */
static Color ray_march(
    Ray ray, f32 time, Vec3 light_pos, HyperPlane *hyperPlanes, Tela *background
) {
  Vec3 p = ray.init;
  f32 t = sdf_scene(p, time, hyperPlanes);

  for (u32 i = 0; i < MAX_RAYMARCH_ITERATIONS; i++) {
    p = trace_ray(ray, t);
    f32 dist = sdf_scene(p, time, hyperPlanes);
    t += dist;

    // Hit surface
    if (dist < RAYMARCH_EPSILON) {
      Vec3 normal = sdf_normal(p, time, hyperPlanes);
      Vec3 to_light = sub_vec3(light_pos, p);
      Vec3 light_dir = normalize_vec3(to_light);
      f32 diffuse = fmaxf(0.0f, dot_vec3(normal, light_dir));
      Color bk_color = sample_background(background, normal);
      f32 bk_gray = (bk_color.red + bk_color.green + bk_color.blue) / 3.0f;
      return (Color){ (diffuse + bk_gray) / 2.0f, 0.0f, 0.0f, 1.0f };
    }

    // Escaped to infinity
    if (t > MAX_RAYMARCH_DISTANCE) {
      Color bk_color = sample_background(background, ray.dir);
      f32 c = (f32)i / MAX_RAYMARCH_ITERATIONS;
      return (Color){ bk_color.red, bk_color.green, (bk_color.blue + c) / 2.0f, 1.0f };
    }
  }

  return sample_background(background, ray.dir);
}

/**
 * Ray scene callback for camera ray mapping.
 */
static Color ray_callback(Ray ray, void *ctx) {
  SceneContext *scene = (SceneContext *)ctx;
  return ray_march(ray, scene->time, scene->light_pos, scene->hyperPlanes, scene->background);
}

/* =============================================================================
 * Animation Loop
 * ========================================================================== */

static void on_frame(f32 dt, f32 time, void *ctx) {
  App *app = (App *)ctx;

  // Rotating light source
  Vec3 light_pos = scale_vec3(vec3(cosf(time), sinf(time), 1.0f), 2.0f);
  SceneContext scene = { .time = time,
                         .light_pos = light_pos,
                         .hyperPlanes = app->hyperPlanes,
                         .background = app->background };

  // Render
  if (app->use_parallel) {
    ray_map_camera_parallel(app->camera, app->tela, ray_callback, &scene);
  } else {
    ray_map_camera(app->camera, app->tela, ray_callback, &scene);
  }

  // Display
  set_window_title(
      app->window,
      format_string(
          "Rock SDF demo | Parallel: %s (P) | FPS: %.1f",
          app->use_parallel ? "ON" : "OFF",
          1.0f / dt
      )
  );
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
  f32 theta_delta = -2.0f * PI * (delta.x / WIDTH);
  f32 phi_delta = -2.0f * PI * (delta.y / HEIGHT);

  set_orbit_camera(
      app->camera, orbit.x, orbit.y + theta_delta, orbit.z + phi_delta
  );

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
  Window *window = new_window(WIDTH, HEIGHT, "SDF Test");

  // Load background texture
  Tela *background = io_read_image("./assets/sky.jpg");

  // Setup camera
  Camera camera = create_camera(vec3(10.0f, 0.0f, 0.0f), vec3(0, 0, 0), 1.0f);

  // Setup hyperplanes
  HyperPlane hyperPlanes[NUM_HYPERPLANES];
  for (int i = 0; i < NUM_HYPERPLANES; i++) {
    Vec3 r = normalize_vec3(vec3(
        2.0f * random_double() - 1.0f,
        2.0f * random_double() - 1.0f,
        2.0f * random_double() - 1.0f
    ));
    hyperPlanes[i] = (HyperPlane){ .a = r, .n = r };
  }

  // Application state
  App app = { .tela = tela,
              .window = window,
              .camera = &camera,
              .use_parallel = true,
              .hyperPlanes = hyperPlanes,
              .background = background };

  // Animation loop
  Loop *animation = loop(on_frame, &app);

  // Event handlers
  on_close_window(window, on_close, animation);
  register_input_handlers(window, &app);

  // Run
  play_loop(animation);

  return 0;
}
