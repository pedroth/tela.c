/**
 * Bunny Explode
 *
 * Ports the JS bunny point-cloud explosion demo to tela.c.
 * Controls:
 *   - Drag with mouse to orbit camera
 *   - Mouse wheel to zoom
 *   - R to reset the bunny
 */

#include "../src/index.c"

static const u32 WIDTH = 640;
static const u32 HEIGHT = 480;
static const f32 SPHERE_RADIUS = 0.02f;
static const f32 GRAVITY = -9.8f;
static const f32 START_DELAY = 5.0f;
static const f32 SPEED_SCALE = 5.0f;

typedef struct {
  Vec3 min;
  Vec3 diagonal;
} NormalizeContext;

typedef struct {
  Tela* tela;
  Window* window;
  Camera camera;
  Scene scene;
  Array original_positions;
  Array speeds;
  f32 elapsed;
} App;

static bool g_mouse_down = false;
static Vec2 g_mouse_pos = { 0 };

static f32 remap_position_component(f32 x) {
  return clamp(0.5f * (x + 1.0f), 0.0f, 1.0f);
}

static Vec3 normalize_mesh_vertex(Vec3 v, void* ctx) {
  NormalizeContext* normalize = (NormalizeContext*)ctx;
  Vec3 translated = sub_vec3(v, normalize->min);
  Vec3 unit = vec3(
      translated.x / normalize->diagonal.x,
      translated.y / normalize->diagonal.y,
      translated.z / normalize->diagonal.z
  );
  return sub_vec3(scale_vec3(unit, 2.0f), vec3(1.0f, 1.0f, 1.0f));
}

static Vec3 rotate_xy(Vec3 v, void* ctx) {
  (void)ctx;
  return vec3(-v.y, v.x, v.z);
}

static Vec3 rotate_xz(Vec3 v, void* ctx) {
  (void)ctx;
  return vec3(v.z, v.y, -v.x);
}

static Vec3 translate_bunny(Vec3 v, void* ctx) {
  (void)ctx;
  return add_vec3(v, vec3(0.0f, 0.0f, 3.0f));
}

static Color position_to_color(Vec3 v, void* ctx) {
  (void)ctx;
  Vec3 mapped = map_vec3(v, remap_position_component);
  return (Color){ mapped.x, mapped.y, mapped.z, 1.0f };
}

static Vec3 random_speed(void) {
  return vec3(
      SPEED_SCALE * (2.0f * (f32)random_double() - 1.0f),
      SPEED_SCALE * (2.0f * (f32)random_double() - 1.0f),
      SPEED_SCALE * (2.0f * (f32)random_double() - 1.0f)
  );
}

static Array* get_bunny_points(App* app) {
  return get_scene_elems_scene(&app->scene);
}

static void reset_bunny(App* app) {
  Array* bunny_points = get_bunny_points(app);
  for (u32 i = 0; i < bunny_points->length; i++) {
    SceneElem* point = (SceneElem*)get_array_element(bunny_points, i);
    Vec3* original_position = (Vec3*)get_array_element(&app->original_positions, i);
    Vec3* speed = (Vec3*)get_array_element(&app->speeds, i);
    point->as.sphere.position = *original_position;
    *speed = random_speed();
  }
  app->elapsed = 0.0f;
}

static void bunny_physics(App* app, f32 dt) {
  Array* bunny_points = get_bunny_points(app);
  Vec3 acceleration = vec3(0.0f, 0.0f, GRAVITY);

  for (u32 i = 0; i < bunny_points->length; i++) {
    SceneElem* point = (SceneElem*)get_array_element(bunny_points, i);
    Sphere* sphere = &point->as.sphere;
    Vec3* speed = (Vec3*)get_array_element(&app->speeds, i);

    if (sphere->position.z <= 0.0f) {
      *speed = vec3(0.0f, 0.0f, -sphere->position.z);
    } else {
      *speed = add_vec3(*speed, scale_vec3(acceleration, dt));
    }

    sphere->position = add_vec3(sphere->position, scale_vec3(*speed, dt));
  }
}

static void on_frame(f32 dt, f32 time, void* ctx) {
  (void)time;
  App* app = (App*)ctx;

  if (app->elapsed > START_DELAY) {
    bunny_physics(app, dt);
  }

  raster_scene(
      &app->scene,
      (RasterParams) {
          .camera = &app->camera,
          .tela = app->tela,
          .cull_backfaces = true,
          .bilinear_texture = false,
          .clip_camera_plane = true,
          .clear_screen = true,
          .background_color = (Color){ 0.0f, 0.0f, 0.0f, 1.0f },
          .perspective_correct = true,
      }
  );

  set_window_title(
      app->window,
      format_string("Bunny Explode | R: reset | FPS: %.0f", dt > 0.0f ? 1.0f / dt : 0.0f)
  );
  paint_window(app->window, app->tela);

  app->elapsed += dt;
}

static void on_close(Window* window, void* ctx) {
  (void)window;
  Loop* animation = (Loop*)ctx;
  stop_loop(animation);
}

static void on_mouse_down(Window* window, i32 x, i32 y, u32 button, void* ctx) {
  (void)window;
  (void)button;
  (void)ctx;
  g_mouse_down = true;
  g_mouse_pos = vec2((f32)x, (f32)y);
}

static void on_mouse_up(Window* window, i32 x, i32 y, u32 button, void* ctx) {
  (void)window;
  (void)x;
  (void)y;
  (void)button;
  (void)ctx;
  g_mouse_down = false;
  g_mouse_pos = vec2(0.0f, 0.0f);
}

static void on_mouse_move(Window* window, i32 x, i32 y, void* ctx) {
  (void)window;
  App* app = (App*)ctx;
  Vec2 new_mouse = vec2((f32)x, (f32)y);
  if (!g_mouse_down || equals_vec2(new_mouse, g_mouse_pos)) {
    return;
  }

  Vec2 delta = sub_vec2(new_mouse, g_mouse_pos);
  Vec3 orbit = get_camera_orbit(&app->camera);
  set_orbit_camera(
      &app->camera,
      orbit.x,
      orbit.y - 2.0f * PI * (delta.x / (f32)app->window->width),
      orbit.z - 2.0f * PI * (delta.y / (f32)app->window->height)
  );
  g_mouse_pos = new_mouse;
}

static void on_mouse_scroll(Window* window, i32 delta_y, void* ctx) {
  (void)window;
  App* app = (App*)ctx;
  Vec3 orbit = get_camera_orbit(&app->camera);
  set_orbit_camera(&app->camera, orbit.x + (f32)delta_y * 0.1f, orbit.y, orbit.z);
}

static void on_key_down(Window* window, u32 keycode, void* ctx) {
  (void)window;
  App* app = (App*)ctx;
  if (keycode == SDLK_r) {
    reset_bunny(app);
  }
}

static void register_input_handlers(Window* window, App* app) {
  on_mouse_down_window(window, on_mouse_down, app);
  on_mouse_up_window(window, on_mouse_up, app);
  on_mouse_move_window(window, on_mouse_move, app);
  on_mouse_scroll_window(window, on_mouse_scroll, app);
  on_key_down_window(window, on_key_down, app);
}

static void build_bunny(App* app) {
  String obj = io_read_file("./assets/bunny.obj");
  Mesh mesh = read_obj_mesh(obj, "bunny");

  AABB box = get_bounding_box_mesh(&mesh);
  NormalizeContext normalize = {
      .min = box.min,
      .diagonal = vec3(
          fmaxf(box.diagonal.x, 1e-6f),
          fmaxf(box.diagonal.y, 1e-6f),
          fmaxf(box.diagonal.z, 1e-6f)
      ),
  };

  map_vertices_mesh(&mesh, normalize_mesh_vertex, &normalize);
  map_vertices_mesh(&mesh, rotate_xy, NULL);
  map_vertices_mesh(&mesh, rotate_xz, NULL);
  map_vertices_mesh(&mesh, translate_bunny, NULL);
  map_colors_mesh(&mesh, position_to_color, NULL);

  Array spheres = get_spheres_mesh(&mesh, SPHERE_RADIUS);
  Array elems = spheres_to_scene_elems(spheres);
  add_scene_elems_scene(&app->scene, elems);

  Array* bunny_points = get_bunny_points(app);
  app->original_positions = new_array(bunny_points->length, sizeof(Vec3));
  app->speeds = new_array(bunny_points->length, sizeof(Vec3));
  for (u32 i = 0; i < bunny_points->length; i++) {
    SceneElem* point = (SceneElem*)get_array_element(bunny_points, i);
    Vec3 position = point->as.sphere.position;
    Vec3 speed = random_speed();
    push_array(&app->original_positions, &position);
    push_array(&app->speeds, &speed);
  }

  free_array(&spheres);
  free_array(&elems);
}

int main(void) {
  Tela* tela = new_tela(WIDTH, HEIGHT);
  Window* window = new_window(WIDTH, HEIGHT, "Bunny Explode");

  App app = {
      .tela = tela,
      .window = window,
      .camera = create_camera(vec3(10.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 1.0f),
      .scene = new_kscene(20),
      .original_positions = { 0 },
      .speeds = { 0 },
      .elapsed = 0.0f,
  };

  set_orbit_camera(&app.camera, 10.0f, 0.0f, PI / 6.0f);
  build_bunny(&app);

  Loop* animation = loop(on_frame, &app);
  on_close_window(window, on_close, animation);
  register_input_handlers(window, &app);
  play_loop(animation);

  free_array(&app.original_positions);
  free_array(&app.speeds);
  free_scene(&app.scene);

  return 0;
}