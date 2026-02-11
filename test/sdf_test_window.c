#include "../src/index.c"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const u32 WIDTH = 640;
const u32 HEIGHT = 480;

typedef struct {
  Tela *tela;
  Window *window;
  Camera *camera;
} AppContext;

/* =============================================================================
 * Global State
 * ========================================================================== */

static bool g_is_mouse_down = false;
static Vec2 g_mouse = {0.0f, 0.0f};

f32 torus_sdf(Vec3 p, f32 r, f32 R) {
  f32 q = length_vec2(vec2(p.x, p.y)) - r;
  return length_vec2(vec2(q, p.z)) - R;
}

typedef struct {
  Vec3 pos;
  f32 radius;
} Sphere_Local;

f32 scene_sdf(Vec3 p, f32 time) {
  // AABB box = build_aabb(vec3(-0.5f, -0.5f, -0.5f), vec3(0.5f, 0.5f, 0.5f));
  // Sphere_Local sphere = {.pos = vec3(0.0f, 0.0f, 0.0f), .radius = 0.65f};
  // f32 tau = (sinf(2 * M_PI * 0.25f * (time - 1)) + 1) / 2;
  // const f32 cube =
  //     fmaxf(distance_aabb(&box, p),
  //           -(length_vec3(sub_vec3(sphere.pos, p)) - sphere.radius));
  const f32 torus = torus_sdf(p, 0.5f, 0.25f);
  return torus;
  // return tau * torus + (1 - tau) * cube;
}

Vec3 normal_scene_sdf(Vec3 p, f32 time) {
  const f32 epsilon = 1e-3f;
  const f32 f = scene_sdf(p, time);
  const Vec3 n = vec3(scene_sdf(add_vec3(p, vec3(epsilon, 0, 0)), time) - f,
                      scene_sdf(add_vec3(p, vec3(0, epsilon, 0)), time) - f,
                      scene_sdf(add_vec3(p, vec3(0, 0, epsilon)), time) - f);
  Vec3 result = {0};
  normalize_vec3(n, &result);
  return result;
}

typedef struct {
  f32 time;
  Vec3 lightPos;
} RaySceneContext;

f32 to_color(f32 x) { return (x + 1.0f) / 2.0f; }

Color simple_ray_scene(Ray ray) {
  Vec3 color = map_vec3(ray.dir, to_color);
  return (Color){color.x, color.y, color.z, 1.0f};
}

Color sdf_torus_box_anime(Ray ray, f32 time, Vec3 lightPos) {
  const u32 max_ite = 100;
  const f32 max_dist = 10;
  const f32 epsilon = 1e-3;
  const Vec3 init = ray.init;
  Vec3 p = init;
  f32 t = scene_sdf(p, time);
  for (u32 i = 0; i < max_ite; i++) {
    p = trace_ray(ray, t);
    const f32 d = scene_sdf(p, time);
    t += d;
    if (d < epsilon) {
      Vec3 light_dir = sub_vec3(lightPos, p);
      Vec3 light_dir_norm;
      normalize_vec3(light_dir, &light_dir_norm);
      const f32 shade =
          fmaxf(0, dot_vec3(normal_scene_sdf(p, time), light_dir_norm));
      return (Color){shade, 0, 0, 1.0f};
    }
    if (d > max_dist) {
      f32 c = 2.0f * (f32)i / max_ite;
      return (Color){c, c, c, 1.0f};
    }
  }
  return (Color){0, 0, 0, 1.0f};
};

Color ray_scene(Ray ray, void *context) {
  RaySceneContext *ray_scene_context = (RaySceneContext *)context;
  // return simple_ray_scene(ray);
  return sdf_torus_box_anime(ray, ray_scene_context->time,
                             ray_scene_context->lightPos);
}

void anime_lambda(f32 dt, f32 time, void *context) {
  AppContext *app_context = (AppContext *)context;
  Camera *camera = app_context->camera;
  Tela *tela = app_context->tela;

  Vec3 lightPos = scale_vec3(vec3(cosf(time), sinf(time), 1.0f), 2.0f);
  RaySceneContext ray_scene_context = {time, lightPos};

  ray_map_camera(camera, tela, ray_scene, &ray_scene_context);
  set_window_title(app_context->window, format_string("FPS: %.2f", 1.0f / dt));
  paint_window(app_context->window, tela);
}

void on_close_lambda(Window *window, void *context) {
  Loop *anime_loop = (Loop *)context;
  stop_loop(anime_loop);
}

/**
 * Called when mouse button is pressed - starts a new line
 */
void on_mouse_down(Window *window, i32 x, i32 y, u32 button, void *context) {
  g_is_mouse_down = true;
  g_mouse = vec2((f32)x, (f32)y);
}

/**
 * Called when mouse button is released - finalizes the line
 */
void on_mouse_up(Window *window, i32 x, i32 y, u32 button, void *context) {
  g_is_mouse_down = false;
  g_mouse = vec2(0.0f, 0.0f);
}

/**
 * Called when mouse moves - updates preview line end position
 */

void on_mouse_move(Window *window, i32 x, i32 y, void *context) {
  const Vec2 new_mouse = vec2((f32)x, (f32)y);
  if (!g_is_mouse_down || equals_vec2(new_mouse, g_mouse)) {
    return;
  }
  AppContext *app_context = (AppContext *)context;
  Camera *camera = app_context->camera;

  const Vec2 delta = sub_vec2(new_mouse, g_mouse);
  Vec3 orbit_coords = get_camera_orbit(camera);
  Vec3 delta_orbit =
      vec3(0, -2 * M_PI * (delta.x / WIDTH), -2 * M_PI * (delta.y / HEIGHT));

  Vec3 new_orbit = add_vec3(orbit_coords, delta_orbit);

  set_orbit_camera(camera, new_orbit.x, new_orbit.y, new_orbit.z);
  g_mouse = new_mouse;
};

void on_mouse_scroll(Window *window, i32 deltaY, void *context) {
  AppContext *app_context = (AppContext *)context;
  Camera *camera = app_context->camera;

  Vec3 orbit = get_camera_orbit(camera);
  Vec3 new_orbit = add_vec3(orbit, vec3(deltaY * 0.001f, 0, 0));

  set_orbit_camera(camera, new_orbit.x, new_orbit.y, new_orbit.z);
};

/**
 * Registers all mouse event handlers
 */
void register_event_handlers(Window *window, AppContext *app) {
  on_mouse_down_window(window, on_mouse_down, app);
  on_mouse_up_window(window, on_mouse_up, app);
  on_mouse_move_window(window, on_mouse_move, app);
  on_mouse_scroll_window(window, on_mouse_scroll, app);
}

int main() {
  // Initialize graphics
  Tela *tela = new_tela(WIDTH, HEIGHT);
  Window *window = new_window(WIDTH, HEIGHT, "SDF Test");
  Camera camera = create_camera(vec3(3, 0, 0), vec3(0, 0, 0), 1.0f);
  set_orbit_camera(&camera, 3.0f, 0, 0);

  // Initialize application state
  AppContext app = {tela, window, &camera};

  // Setup animation loop
  Loop *animation_loop = loop(anime_lambda, &app);
  on_close_window(window, on_close_lambda, animation_loop);

  // Register input handlers
  register_event_handlers(window, &app);

  // Start the application (must be last)
  play_loop(animation_loop);

  return 0;
}
