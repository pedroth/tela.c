#include "../src/index.c"

const u32 WIDTH = 640;
const u32 HEIGHT = 480;

typedef struct {
  Tela *tela;
  Window *window;
  Camera camera;
} AppContext;

/* =============================================================================
 * Global State
 * ========================================================================== */

static bool g_is_mouse_down = false;
static Vec2 g_mouse = {0.0f, 0.0f};

f32 torusSdf(Vec3 p, f32 r, f32 R) {
  const Vec2 q = vec2(length_vec2(vec2(p.x, p.y)) - r, p.z);
  return length_vec2(q) - R;
}

typedef struct {
  Vec3 pos;
  f32 radius;
} Sphere_Local;

f32 distanceFunction(Vec3 p, f32 time) {
  AABB box = build_aabb(vec3(-0.5f, -0.5f, -0.5f), vec3(0.5f, 0.5f, 0.5f));
  Sphere_Local sphere = {pos : vec3(0.0f, 0.0f, 0.0f), radius : 0.65f};
  f32 tau = (sinf(2 * M_PI * 0.25f * (time - 1)) + 1) / 2;
  const cube = fmaxf(distance_AABB(box, p),
                     -(length_vec3(sub_vec3(sphere.pos, p)) - sphere.radius));
  const torus = torusSdf(p, 0.5f, 0.25f);
  return tau * torus + (1 - tau) * cube;
}

Vec3 normalFunction(Vec3 p, f32 time) {
  const f32 epsilon = 1e-3f;
  const f32 f = distanceFunction(p, time);
  const Vec3 n =
      vec3(distanceFunction(add_vec3(p, vec3(epsilon, 0, 0)), time) - f,
           distanceFunction(add_vec3(p, vec3(0, epsilon, 0)), time) - f,
           distanceFunction(add_vec3(p, vec3(0, 0, epsilon)), time) - f);
  Vec3 result = {0};
  normalize_vec3(n, &result);
  return result;
}

const rayScene = (ray, {time, lightPosSerial}) => {
  const maxIte = 100;
  const maxDist = 10;
  const epsilon = 1e-3;
  const lightPos = Vec.fromArray(lightPosSerial);
  const {init} = ray;
  let p = init;
  let t = distanceFunction(p, time);
  for (let i = 0; i < maxIte; i++) {
    p = ray.trace(t);
    const d = distanceFunction(p, time);
    t += d;
    if (d < epsilon) {
      const shade =
          Math.max(0, normalFunction(p, time).dot(lightPos.sub(p).normalize()));
      return Color.ofRGB(shade, 0, 0);
    }
    if (d > maxDist)
      return Color.ofRGB(0, 0, i / maxIte);
  }
  return Color.BLACK;
};

loop(async({dt, time}) = > {
  const lightPos = Vec3(Math.cos(time), Math.sin(time), 1).scale(2);
  // camera.rayMap(ray => rayScene(ray, {time, lightPosSerial:
  // lightPos.toArray()})).to(canvas).paint()
  (await camera
       .rayMapParallel(rayScene, [ torusSdf, distanceFunction, normalFunction ])
       .to(canvas, {time, lightPosSerial : lightPos.toArray()}))
      .paint();
  logger.print(`FPS : $ { (1 / dt).toFixed(2) }`);
}).play();

typedef struct {
  f32 time;
} RaySceneContext;

Color ray_scene(Ray ray, void *context) {
  RaySceneContext *ray_scene_context = (RaySceneContext *)context;
  return rayScene(ray, ray_scene_context);
}

void anime_lambda(f32 dt, f32 time, void *context) {
  AppContext *app_context = (AppContext *)context;
  Camera camera = app_context->camera;
  Tela *tela = app_context->tela;
  RaySceneContext ray_scene_context = {time};

  ray_map_camera(&camera, tela, ray_scene, &ray_scene_context);
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
  Camera camera = app_context->camera;
  const Vec2 delta = sub_vec2(new_mouse, g_mouse);
  Vec3 orbit_coords = get_camera_orbit(&camera);
  orbit_coords = add_vec3(orbit_coords, vec3(0, -2 * M_PI * (delta.x / WIDTH),
                                             -2 * M_PI * (delta.y / HEIGHT)));
  set_orbit_camera(&camera, orbit_coords.x, orbit_coords.y,
                   orbit_coords.z); // reset camera to default position
  g_mouse = new_mouse;
};

void on_mouse_scroll_window(Window *window, i32 deltaY, void *context) {
  get_camera_orbit(&camera);
  camera.orbit((coords) = > coords.add(Vec3(deltaY * 0.001, 0, 0)));
});

/**
 * Registers all mouse event handlers
 */
void register_event_handlers(Window *window, AppContext *app) {
  on_mouse_down_window(window, on_mouse_down, app);
  on_mouse_up_window(window, on_mouse_up, app);
  on_mouse_move_window(window, on_mouse_move, app);
}

int main() {
  // Initialize graphics
  Tela *tela = new_tela(WIDTH, HEIGHT);
  Window *window = new_window(WIDTH, HEIGHT, "SDF Test");
  Camera camera = {0};
  set_orbit_camera(&camera, 3.0f, 0, 0);

  // Initialize application state
  AppContext app = {tela, window, camera};

  // Setup animation loop
  Loop *animation_loop = loop(anime_lambda, &app);
  on_close_window(window, on_close_lambda, animation_loop);

  // Register input handlers
  register_event_handlers(window, &app);

  // Start the application (must be last)
  play_loop(animation_loop);

  return 0;
}
