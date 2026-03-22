/**
 * Kakashi Point Cloud Animation
 *
 * Loads an image and creates a point cloud where each pixel becomes a sphere.
 * Animates between two states using Anima:
 *   1. Grid layout (pixel positions)
 *   2. Color space layout (RGB → XYZ)
 * Features interactive orbit camera controls via mouse.
 *
 * Port from tela.js kakashi demo.
 *
 * gcc -O3 -fopenmp -o app test/kakashi.c -lSDL2 -lm
 */

#include "../src/index.c"

/* =============================================================================
 * Constants
 * ========================================================================== */

static const u32 WIDTH = 640;
static const u32 HEIGHT = 480;
static const f32 SPHERE_RADIUS = 1e-5f;
static const f32 ANIM_DURATION = 2.0f;

/* =============================================================================
 * Types
 * ========================================================================== */

typedef struct {
  Vec3 init_pos;  // initial grid position
  Vec3 color_pos; // position derived from color (r, g, b)
} PixelData;

typedef struct {
  Tela *tela;
  Window *window;
  Camera *camera;
  Scene *scene;
  Array *elems;       // Array of SceneElem (owned by scene)
  PixelData *pixels;  // per-sphere metadata
  u32 num_pixels;
} App;

/* =============================================================================
 * Global State (for input handling)
 * ========================================================================== */

static bool g_mouse_down = false;
static Vec2 g_mouse_pos = {0};

/* =============================================================================
 * Animation Behaviors
 * ========================================================================== */

/**
 * Behavior 1: interpolate spheres from current position toward init_pos.
 * tau goes from 0 to duration.
 */
static void move_to_grid(f32 tau, f32 dt, void *ctx) {
  App *app = (App *)ctx;
  Array *elems = app->elems;
  f32 t = tau / ANIM_DURATION; // normalize to [0, 1]

  for (u32 k = 0; k < elems->length; k++) {
    SceneElem *elem = (SceneElem *)get_array_element(elems, k);
    Sphere *s = elem->as.sphere;
    Vec3 target = app->pixels[k].init_pos;
    Vec3 speed = sub_vec3(target, s->position);
    s->position = add_vec3(s->position, scale_vec3(speed, t));
  }
}

/**
 * Behavior 2: interpolate spheres from current position toward color_pos.
 * tau goes from 0 to duration.
 */
static void move_to_color(f32 tau, f32 dt, void *ctx) {
  App *app = (App *)ctx;
  Array *elems = app->elems;
  f32 t = tau / ANIM_DURATION; // normalize to [0, 1]

  for (u32 k = 0; k < elems->length; k++) {
    SceneElem *elem = (SceneElem *)get_array_element(elems, k);
    Sphere *s = elem->as.sphere;
    Vec3 target = app->pixels[k].color_pos;
    Vec3 speed = sub_vec3(target, s->position);
    s->position = add_vec3(s->position, scale_vec3(speed, t));
  }
}

/* =============================================================================
 * Animation Loop
 * ========================================================================== */

static Anima g_anima;

static void on_frame(f32 dt, f32 time, void *ctx) {
  App *app = (App *)ctx;

  // Raster the point cloud
  raster_scene(app->scene, (RasterParams){
    .camera = app->camera,
    .tela = app->tela,
    .cull_backfaces = true,
    .bilinear_texture = false,
    .clip_camera_plane = true,
    .clear_screen = true,
    .background_color = (Color){0.0f, 0.0f, 0.0f, 1.0f},
    .perspective_correct = true,
  });

  // Run animation
  anima_loop(&g_anima, time, dt, app);

  // Display
  set_window_title(app->window,
                   format_string("Img2RGB | FPS: %.1f", 1.0f / dt));
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
  if (new_radius < 0.1f)
    new_radius = 0.1f;

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
  // Load image
  Tela *img = io_read_image("./assets/kakashi.jpg");
  if (!img) {
    fprintf(stderr, "Failed to load ./assets/kakashi.jpg\n");
    return 1;
  }

  u32 img_w = img->width;
  u32 img_h = img->height;
  u32 num_pixels = img_w * img_h;
  Scene scene = new_naive_scene();

  // Build per-pixel data
  PixelData *pixels = (PixelData *)malloc(num_pixels * sizeof(PixelData));

  for (u32 k = 0; k < num_pixels; k++) {
    u32 i = k / img_w;
    u32 j = k % img_w;

    // Initial position: flat grid on YZ plane at x=0, mapped to [0,1]
    Vec3 init_pos = vec3(0.0f, (f32)j / (f32)img_w, (f32)i / (f32)img_h);

    // Get pixel color
    Color color = get_pxl_tela(img, j, i);

    // Color position: RGB → XYZ
    Vec3 color_pos = vec3(color.red, color.green, color.blue);

    pixels[k].init_pos = init_pos;
    pixels[k].color_pos = color_pos;

    // Build sphere at initial position with pixel color
    Sphere sphere = build_sphere(init_pos, SPHERE_RADIUS);
    RasterSphereProps *props =
        (RasterSphereProps *)malloc(sizeof(RasterSphereProps));
    props->color = color;
    props->tex_coord = vec2(0, 0);
    props->texture = NULL;
    props->material = NULL;
    sphere.props = props;

    add_scene_elem_scene(&scene, build_scene_elem_sphere(sphere));
  }

  // Create window and canvas
  Tela *tela = new_tela(WIDTH, HEIGHT);
  Window *window = new_window(WIDTH, HEIGHT, "Img2RGB");

  // Setup camera looking at center of grid (0.5, 0.5, 0.5)
  Camera camera =
      create_camera(vec3(5.0f, 0.5f, 0.5f), vec3(0.5f, 0.5f, 0.5f), 1.0f);

  // Application state
  App app = {
      .tela = tela,
      .window = window,
      .camera = &camera,
      .scene = &scene,
        .elems = get_scene_elems_scene(&scene),
      .pixels = pixels,
      .num_pixels = num_pixels,
  };

  // Build Anima sequence: grid → wait → color → wait (loops)
  Array behaviors = new_array(4, sizeof(AnimaBehavior));
  AnimaBehavior b1 = anima_behavior(move_to_grid, ANIM_DURATION);
  AnimaBehavior b2 = anima_wait(ANIM_DURATION);
  AnimaBehavior b3 = anima_behavior(move_to_color, ANIM_DURATION);
  AnimaBehavior b4 = anima_wait(ANIM_DURATION);
  push_array(&behaviors, &b1);
  push_array(&behaviors, &b2);
  push_array(&behaviors, &b3);
  push_array(&behaviors, &b4);
  g_anima = new_anima(behaviors);

  // Animation loop
  Loop *animation = loop(on_frame, &app);

  // Event handlers
  on_close_window(window, on_close, animation);
  register_input_handlers(window, &app);

  // Run
  play_loop(animation);

  // Cleanup
  free(pixels);
  free_anima(&g_anima);

  return 0;
}
