/**
 * Cornell Box + Wave Mesh
 *
 * Path-traced Cornell box with a dynamic wave triangle mesh.
 * The wave mesh uses a dielectric material.
 */

#include "../src/index.c"

// gcc -O3 -fopenmp -o app test/cornell_box_video_wave.c -lSDL2 -lm

/* =============================================================================
 * Constants
 * ========================================================================== */

static const u32 WIDTH = 640;
static const u32 HEIGHT = 480;

#define WAVE_GRID 24
#define WAVE_INIT_AMPLITUDE 1.5f
#define WAVE_SPEED 7.0f
#define WAVE_FRICTION 0.5f
#define WAVE_SPREAD 80.0f

static const f32 WAVE_X_MIN = 0.45f;
static const f32 WAVE_X_MAX = 2.55f;
static const f32 WAVE_Y_MIN = 0.45f;
static const f32 WAVE_Y_MAX = 2.55f;
static const f32 WAVE_BASE_Z = 1.0f;
static const f32 WAVE_HEIGHT_SCALE = 1.0f;

/* =============================================================================
 * Types
 * ========================================================================== */

typedef struct {
  Tela* tela;
  Camera* camera;
  Scene* scene;
  Material* mat_diffuse;
  Material* mat_emissive;
  Material* mat_wave_dielectric;
} App;

static f32 g_wave_height[WAVE_GRID][WAVE_GRID];
static f32 g_wave_velocity[WAVE_GRID][WAVE_GRID];

static void build_cornell_box(App* app);
static void add_wave_mesh(App* app);

static inline u32 wrap_grid(i32 n) {
  return (u32)(((n % WAVE_GRID) + WAVE_GRID) % WAVE_GRID);
}

static inline f32 wave_world_x(u32 i) {
  f32 t = (f32)i / (f32)(WAVE_GRID - 1);
  return WAVE_X_MIN + t * (WAVE_X_MAX - WAVE_X_MIN);
}

static inline f32 wave_world_y(u32 j) {
  f32 t = (f32)j / (f32)(WAVE_GRID - 1);
  return WAVE_Y_MIN + t * (WAVE_Y_MAX - WAVE_Y_MIN);
}

static inline f32 wave_world_z(u32 i, u32 j) {
  return WAVE_BASE_Z + WAVE_HEIGHT_SCALE * g_wave_height[j][i];
}

static void initialize_wave(void) {
  for (u32 j = 0; j < WAVE_GRID; j++) {
    for (u32 i = 0; i < WAVE_GRID; i++) {
      f32 x = ((f32)i - (f32)WAVE_GRID * 0.5f) / (f32)WAVE_GRID;
      f32 y = ((f32)j - (f32)WAVE_GRID * 0.5f) / (f32)WAVE_GRID;

      f32 bump1 =
          WAVE_INIT_AMPLITUDE * expf(-WAVE_SPREAD * ((x - 0.22f) * (x - 0.22f) + y * y));
      f32 bump2 =
          WAVE_INIT_AMPLITUDE * expf(-WAVE_SPREAD * ((x + 0.22f) * (x + 0.22f) + y * y));
      f32 bump3 =
          WAVE_INIT_AMPLITUDE * expf(-WAVE_SPREAD * (x * x + (y - 0.22f) * (y - 0.22f)));

      g_wave_height[j][i] = bump1 + bump2 + bump3;
      g_wave_velocity[j][i] = 0.0f;
    }
  }
}

static void update_wave_simulation(f32 dt) {
  for (u32 j = 0; j < WAVE_GRID; j++) {
    for (u32 i = 0; i < WAVE_GRID; i++) {
      f32 laplacian =
          g_wave_height[j][wrap_grid((i32)i + 1)] +
          g_wave_height[j][wrap_grid((i32)i - 1)] +
          g_wave_height[wrap_grid((i32)j + 1)][i] +
          g_wave_height[wrap_grid((i32)j - 1)][i] -
          4.0f * g_wave_height[j][i];

      f32 acceleration = WAVE_SPEED * laplacian - WAVE_FRICTION * g_wave_velocity[j][i];
      g_wave_velocity[j][i] += dt * acceleration;
      g_wave_height[j][i] += dt * g_wave_velocity[j][i];
    }
  }
}

/* =============================================================================
 * Animation / Render Loop
 * ========================================================================== */

static inline Color render_background(Ray ray, void* ctx) {
  (void)ray;
  (void)ctx;
  return COLOR_BLACK;
}

static void on_frame(f32 dt, f32 time, void* ctx) {
  (void)time;
  App* app = (App*)ctx;

  update_wave_simulation(dt);
  clear_scene_elems_scene(app->scene);
  build_cornell_box(app);
  add_wave_mesh(app);
  app->scene->vtable->rebuild_scene(app->scene);

  RaytraceParams params = {
    .samples_per_pixel = 250,
    .bounces = 10,
    .variance = 0.001f,
    .gamma = 0.5f,
    .bilinear_texture = false,
    .is_biased = true,
    .camera = app->camera,
    .render_background = render_background,
    .exposed_tela = app->tela,
  };

  ray_trace_scene_parallel(app->scene, &params);
  app->tela->iterations = 1;
}


/* =============================================================================
 * Build Scene
 * ========================================================================== */

static RasterTriangleProps* new_tri_props(Color c, Material* mat) {
  RasterTriangleProps* p =
      (RasterTriangleProps*)malloc(sizeof(RasterTriangleProps));
  p->colors[0] = c;
  p->colors[1] = c;
  p->colors[2] = c;
  p->tex_coords[0] = vec2(0, 0);
  p->tex_coords[1] = vec2(1, 0);
  p->tex_coords[2] = vec2(0, 1);
  p->texture = NULL;
  p->material = mat;
  return p;
}

static RasterSphereProps* new_sph_props(Color c, Material* mat) {
  RasterSphereProps* p = (RasterSphereProps*)malloc(sizeof(RasterSphereProps));
  p->color = c;
  p->tex_coord = vec2(0, 0);
  p->texture = NULL;
  p->material = mat;
  return p;
}

static void add_wave_mesh(App* app) {
  for (u32 j = 0; j + 1 < WAVE_GRID; j++) {
    for (u32 i = 0; i + 1 < WAVE_GRID; i++) {
      Vec3 p00 = vec3(wave_world_x(i), wave_world_y(j), wave_world_z(i, j));
      Vec3 p10 = vec3(wave_world_x(i + 1), wave_world_y(j), wave_world_z(i + 1, j));
      Vec3 p01 = vec3(wave_world_x(i), wave_world_y(j + 1), wave_world_z(i, j + 1));
      Vec3 p11 = vec3(wave_world_x(i + 1), wave_world_y(j + 1), wave_world_z(i + 1, j + 1));

      Triangle t1 = build_triangle(p00, p10, p01);
      t1.props = new_tri_props(
          (Color){ 0.92f, 0.96f, 1.0f, 1.0f }, app->mat_wave_dielectric
      );
      add_scene_elem_scene(app->scene, build_scene_elem_triangle(t1));

      Triangle t2 = build_triangle(p10, p11, p01);
      t2.props = new_tri_props(
          (Color){ 0.92f, 0.96f, 1.0f, 1.0f }, app->mat_wave_dielectric
      );
      add_scene_elem_scene(app->scene, build_scene_elem_triangle(t2));
    }
  }
}

static void build_cornell_box(App* app) {
  Scene* scene = app->scene;

  // left-1
  Triangle left_1 = build_triangle(vec3(3, 0, 3), vec3(3, 0, 0), vec3(0, 0, 0));
  left_1.props = new_tri_props(COLOR_RED, app->mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(left_1));

  // left-2
  Triangle left_2 = build_triangle(vec3(0, 0, 0), vec3(0, 0, 3), vec3(3, 0, 3));
  left_2.props = new_tri_props(COLOR_RED, app->mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(left_2));

  // right-1
  Triangle right_1 =
      build_triangle(vec3(0, 3, 0), vec3(3, 3, 0), vec3(3, 3, 3));
  right_1.props = new_tri_props(COLOR_GREEN, app->mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(right_1));

  // right-2
  Triangle right_2 =
      build_triangle(vec3(3, 3, 3), vec3(0, 3, 3), vec3(0, 3, 0));
  right_2.props = new_tri_props(COLOR_GREEN, app->mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(right_2));

  // bottom-1
  Triangle bottom_1 =
      build_triangle(vec3(0, 0, 0), vec3(3, 0, 0), vec3(3, 3, 0));
  bottom_1.props = new_tri_props(COLOR_WHITE, app->mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(bottom_1));

  // bottom-2
  Triangle bottom_2 =
      build_triangle(vec3(3, 3, 0), vec3(0, 3, 0), vec3(0, 0, 0));
  bottom_2.props = new_tri_props(COLOR_WHITE, app->mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(bottom_2));

  // top-1
  Triangle top_1 = build_triangle(vec3(3, 3, 3), vec3(3, 0, 3), vec3(0, 0, 3));
  top_1.props = new_tri_props(COLOR_WHITE, app->mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(top_1));

  // top-2
  Triangle top_2 = build_triangle(vec3(0, 0, 3), vec3(0, 3, 3), vec3(3, 3, 3));
  top_2.props = new_tri_props(COLOR_WHITE, app->mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(top_2));

  // back-1
  Triangle back_1 = build_triangle(vec3(0, 0, 0), vec3(0, 3, 0), vec3(0, 3, 3));
  back_1.props = new_tri_props(COLOR_WHITE, app->mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(back_1));

  // back-2
  Triangle back_2 = build_triangle(vec3(0, 3, 3), vec3(0, 0, 3), vec3(0, 0, 0));
  back_2.props = new_tri_props(COLOR_WHITE, app->mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(back_2));

  // light-1
  Triangle light_1 =
      build_triangle(vec3(1, 1, 2.9), vec3(2, 1, 2.9), vec3(2, 2, 2.9));
  light_1.props = new_tri_props(COLOR_WHITE, app->mat_emissive);
  add_scene_elem_scene(scene, build_scene_elem_triangle(light_1));

  // light-2
  Triangle light_2 =
      build_triangle(vec3(2, 2, 2.9), vec3(1, 2, 2.9), vec3(1, 1, 2.9));
  light_2.props = new_tri_props(COLOR_WHITE, app->mat_emissive);
  add_scene_elem_scene(scene, build_scene_elem_triangle(light_2));
}

/* =============================================================================
 * Main
 * ========================================================================== */

bool until_lambda(f32 time) {
  return time < 15.0f;
}

int main(void) {
  initialize_wave();

  Tela* tela = new_tela(WIDTH, HEIGHT);

  /* camera looks at center of box, orbiting at distance 3 */
  Camera camera =
      create_camera(vec3(4.0f, 0.0f, PI/16.f), vec3(1.5f, 1.5f, 1.5f), 1.0f);
  set_orbit_camera(&camera, 4.0f, 0.0f, PI / 16.0f);

  App app = {
    .tela = tela,
    .camera = &camera,
    .scene = NULL,
    .mat_diffuse = (Material*)malloc(sizeof(Material)),
    .mat_emissive = (Material*)malloc(sizeof(Material)),
    .mat_wave_dielectric = (Material*)malloc(sizeof(Material)),
  };

  *app.mat_diffuse = build_diffuse_material();
  *app.mat_emissive = build_emissive_material();
  *app.mat_wave_dielectric = build_dielectric_material(1.45f);

  Scene scene = new_kscene(20);
  app.scene = &scene;

  build_cornell_box(&app);
  add_wave_mesh(&app);
  app.scene->vtable->rebuild_scene(app.scene);

  Loop* animation = loop(on_frame, &app);
  LoopVideoParams loop_params = {
    .until = until_lambda,
    .fps = 30,
    .tela = tela,
  };
  u32 t0 = get_time_ms();
  loop_to_video(animation, "cornell_box_video_wave.mp4", loop_params);
  u32 t1 = get_time_ms();
  printf("loop_to_video took %.3f s\n", (t1 - t0) / 1000.0);

  return 0;
}
