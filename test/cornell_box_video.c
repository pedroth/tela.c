/**
 * Cornell Box
 *
 * A path-traced Cornell box scene with diffuse, metallic, dielectric, and
 * alpha materials. Features interactive orbit camera controls via mouse.
 */

#include "../src/index.c"

// gcc -O3 -fopenmp -o app test/cornell_box_video.c -lSDL2 -lm

/* =============================================================================
 * Constants
 * ========================================================================== */

static const u32 WIDTH = 640;
static const u32 HEIGHT = 480;

/* =============================================================================
 * Types
 * ========================================================================== */

typedef struct {
  Tela* tela;
  Sphere* sphere;
  Camera* camera;
  Scene* scene;
} App;

/* =============================================================================
 * Global State
 * ========================================================================== */

static bool g_mouse_down = false;
static Vec2 g_mouse_pos = { 0 };

/* =============================================================================
 * Animation / Render Loop
 * ========================================================================== */

static inline Color render_background(Ray ray, void* ctx) {
  return COLOR_BLACK;
}

static void on_frame(f32 dt, f32 time, void* ctx) {
  App* app = (App*)ctx;
  Scene* scene = app->scene;
  Camera* camera = app->camera;
  Tela* exposed = app->tela;
  Sphere* sphere = app->sphere;

  sphere->position =
      vec3(1.5f + 0.7f * cosf(time), 1.5f + 0.7f * sinf(time), 1.5f);

  RaytraceParams params = {
    .samples_per_pixel = 1000,
    .bounces = 10,
    .variance = 0.001f,
    .gamma = 0.5f,
    .bilinear_texture = false,
    .is_biased = true,
    .camera = camera,
    .render_background = render_background,
    .exposed_tela = exposed,
  };

  ray_trace_scene_parallel(scene, &params);
  exposed->iterations = 1;
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

static void build_cornell_box(Scene* scene) {
  Material* mat_diffuse = (Material*)malloc(sizeof(Material));
  *mat_diffuse = build_diffuse_material();
  Material* mat_emissive = (Material*)malloc(sizeof(Material));
  *mat_emissive = build_emissive_material();

  // left-1
  Triangle left_1 = build_triangle(vec3(3, 0, 3), vec3(3, 0, 0), vec3(0, 0, 0));
  left_1.props = new_tri_props(COLOR_RED, mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(left_1));

  // left-2
  Triangle left_2 = build_triangle(vec3(0, 0, 0), vec3(0, 0, 3), vec3(3, 0, 3));
  left_2.props = new_tri_props(COLOR_RED, mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(left_2));

  // right-1
  Triangle right_1 =
      build_triangle(vec3(0, 3, 0), vec3(3, 3, 0), vec3(3, 3, 3));
  right_1.props = new_tri_props(COLOR_GREEN, mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(right_1));

  // right-2
  Triangle right_2 =
      build_triangle(vec3(3, 3, 3), vec3(0, 3, 3), vec3(0, 3, 0));
  right_2.props = new_tri_props(COLOR_GREEN, mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(right_2));

  // bottom-1
  Triangle bottom_1 =
      build_triangle(vec3(0, 0, 0), vec3(3, 0, 0), vec3(3, 3, 0));
  bottom_1.props = new_tri_props(COLOR_WHITE, mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(bottom_1));

  // bottom-2
  Triangle bottom_2 =
      build_triangle(vec3(3, 3, 0), vec3(0, 3, 0), vec3(0, 0, 0));
  bottom_2.props = new_tri_props(COLOR_WHITE, mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(bottom_2));

  // top-1
  Triangle top_1 = build_triangle(vec3(3, 3, 3), vec3(3, 0, 3), vec3(0, 0, 3));
  top_1.props = new_tri_props(COLOR_WHITE, mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(top_1));

  // top-2
  Triangle top_2 = build_triangle(vec3(0, 0, 3), vec3(0, 3, 3), vec3(3, 3, 3));
  top_2.props = new_tri_props(COLOR_WHITE, mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(top_2));

  // back-1
  Triangle back_1 = build_triangle(vec3(0, 0, 0), vec3(0, 3, 0), vec3(0, 3, 3));
  back_1.props = new_tri_props(COLOR_WHITE, mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(back_1));

  // back-2
  Triangle back_2 = build_triangle(vec3(0, 3, 3), vec3(0, 0, 3), vec3(0, 0, 0));
  back_2.props = new_tri_props(COLOR_WHITE, mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(back_2));

  // light-1
  Triangle light_1 =
      build_triangle(vec3(1, 1, 2.9), vec3(2, 1, 2.9), vec3(2, 2, 2.9));
  light_1.props = new_tri_props(COLOR_WHITE, mat_emissive);
  add_scene_elem_scene(scene, build_scene_elem_triangle(light_1));

  // light-2
  Triangle light_2 =
      build_triangle(vec3(2, 2, 2.9), vec3(1, 2, 2.9), vec3(1, 1, 2.9));
  light_2.props = new_tri_props(COLOR_WHITE, mat_emissive);
  add_scene_elem_scene(scene, build_scene_elem_triangle(light_2));
}

/* =============================================================================
 * Main
 * ========================================================================== */

bool until_lambda(f32 time) {
  return time < 6.0f;
}

int main(void) {
  Tela* tela = new_tela(WIDTH, HEIGHT);

  /* camera looks at center of box, orbiting at distance 3 */
  Camera camera =
      create_camera(vec3(4.0f, 0.0f, 0.0f), vec3(1.5f, 1.5f, 1.5f), 1.0f);
  set_orbit_camera(&camera, 4.0f, 0.0f, 0.0f);

  Scene scene = new_naive_scene();
  build_cornell_box(&scene);

  Material mat_dielectric_13 = build_dielectric_material(3);
  Sphere glass_sphere = build_sphere(vec3(1.0, 1.5, 1.0), 0.5);
  glass_sphere.props =
      new_sph_props((Color){ 1.0f, 1.0f, 1.0f, 1.0f }, &mat_dielectric_13);
  SceneElem sphere_elem = build_scene_elem_sphere(glass_sphere);
  Sphere* scene_sphere = sphere_elem.as.sphere;
  add_scene_elem_scene(&scene, sphere_elem);

  App app = {
    .tela = tela,
    .sphere = scene_sphere,
    .camera = &camera,
    .scene = &scene,
  };

  Loop* animation = loop(on_frame, &app);
  LoopVideoParams loop_params = {
    .until = until_lambda,
    .fps = 30,
    .tela = tela,
  };
  u32 t0 = get_time_ms();
  loop_to_video(animation, "cornell_box_video.mp4", loop_params);
  u32 t1 = get_time_ms();
  printf("loop_to_video took %.3f s\n", (t1 - t0) / 1000.0);

  return 0;
}
