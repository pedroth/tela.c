#include "../src/index.c"

// gcc -O3 -fopenmp -o app test/cornell_box_mesh_video.c -lSDL2 -lm && ./app

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
  u32 bunny_start_index;
  u32 bunny_triangle_count;
  Array bunny_base_positions;
  Camera* camera;
  Scene* scene;
} App;

typedef struct {
  Vec3 positions[3];
} TrianglePositions;

typedef struct {
  Tela* tela;
} DenoiserContext;

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

static inline Vec3 rotate_z_around(Vec3 p, Vec3 pivot, f32 angle) {
  f32 s = sinf(angle);
  f32 c = cosf(angle);
  Vec3 r = sub_vec3(p, pivot);
  return add_vec3(vec3(r.x * c - r.y * s, r.x * s + r.y * c, r.z), pivot);
}

Color denoiser(u32 x, u32 y, void const* ctx) {
  DenoiserContext* c = (DenoiserContext*)ctx;
  Tela* tela = c->tela;
  u32 w = tela->width;
  u32 h = tela->height;
  // box filter 3x3
  f32 r = 0, g = 0, b = 0, a = 0;
  for (i32 dy = -1; dy <= 1; dy++) {
    for (i32 dx = -1; dx <= 1; dx++) {
      u32 nx = (u32)((i32)x + dx < 0 ? 0 : ((i32)x + dx >= (i32)w ? w - 1 : (i32)x + dx));
      u32 ny = (u32)((i32)y + dy < 0 ? 0 : ((i32)y + dy >= (i32)h ? h - 1 : (i32)y + dy));
      Color col = get_pxl_tela(tela, nx, ny);
      r += col.red;
      g += col.green;
      b += col.blue;
      a += col.alpha;
    }
  }
  f32 inv9 = 1.0f / 9.0f;
  return (Color){ r * inv9, g * inv9, b * inv9, a * inv9 };
}

static void on_frame(f32 dt, f32 time, void* ctx) {
  App* app = (App*)ctx;
  Scene* scene = app->scene;
  Camera* camera = app->camera;
  Tela* exposed = app->tela;
  Vec3 pivot = vec3(1.5f, 1.5f, 1.0f);
  for (u32 i = 0; i < app->bunny_triangle_count; i++) {
    SceneElem* elem = get_array_element(
        get_scene_elems_scene(scene), app->bunny_start_index + i
    );
    TrianglePositions* base = get_array_element(&app->bunny_base_positions, i);
    for (u32 j = 0; j < 3; j++) {
      elem->as.triangle.positions[j] =
          rotate_z_around(base->positions[j], pivot, time);
    }
  }

  // KScene stores internal leaf copies; rebuild so updated triangle positions
  // are used.
  scene->vtable->rebuild_scene(scene);

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

  DenoiserContext denoiser_context = {
    .tela = exposed,
  };
  map_tela_parallel(exposed, denoiser, &denoiser_context);
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

typedef struct {
  Vec3 center;
  f32 scale_inv;
} NormalizeCtx;

static Vec3 transform_normalize(Vec3 v, void* ctx) {
  NormalizeCtx* c = (NormalizeCtx*)ctx;
  return scale_vec3(sub_vec3(v, c->center), c->scale_inv);
}

static Vec3 transform_rotate(Vec3 v, void* ctx) {
  return vec3(-v.z, -v.x, v.y);
}

static Vec3 transform_translate(Vec3 v, void* ctx) {
  return add_vec3(v, vec3(1.5f, 1.5f, 1.0f));
}

static Color white_color_mapper(Vec3 v, void* ctx) {
  return COLOR_WHITE;
}

static Material glass_material_mapper(Face face, void* ctx) {
  return build_dielectric_material(3);
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

  Scene scene = new_kscene(20);
  build_cornell_box(&scene);

  String obj = io_read_file("./assets/bunny_orig.obj");
  Mesh bunny_mesh = read_obj_mesh(obj, "bunny");

  AABB box = get_bounding_box_mesh(&bunny_mesh);
  f32 scale_inv = 2.0f / max_comp_vec3(box.diagonal);
  NormalizeCtx nctx = { .center = box.center, .scale_inv = scale_inv };

  map_vertices_mesh(&bunny_mesh, transform_normalize, &nctx);
  map_vertices_mesh(&bunny_mesh, transform_rotate, NULL);
  map_vertices_mesh(&bunny_mesh, transform_translate, NULL);
  map_colors_mesh(&bunny_mesh, white_color_mapper, NULL);
  map_triangles_materials_mesh(&bunny_mesh, glass_material_mapper, NULL);

  u32 bunny_start_index = get_scene_elems_scene(&scene)->length;
  Array bunny_elems = triangles_to_scene_elems(get_triangles_mesh(&bunny_mesh));
  u32 bunny_triangle_count = bunny_elems.length;
  add_scene_elems_scene(&scene, bunny_elems);
  free_array(&bunny_elems);

  Array bunny_base_positions =
      new_array(bunny_triangle_count, sizeof(TrianglePositions));
  for (u32 i = 0; i < bunny_triangle_count; i++) {
    SceneElem* elem =
        get_array_element(get_scene_elems_scene(&scene), bunny_start_index + i);
    TrianglePositions tri = {
      .positions = {
        elem->as.triangle.positions[0],
        elem->as.triangle.positions[1],
        elem->as.triangle.positions[2],
      },
    };
    push_array(&bunny_base_positions, &tri);
  }

  App app = {
    .tela = tela,
    .bunny_start_index = bunny_start_index,
    .bunny_triangle_count = bunny_triangle_count,
    .bunny_base_positions = bunny_base_positions,
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
  loop_to_video(animation, "cornell_box_mesh_video.mp4", loop_params);
  u32 t1 = get_time_ms();
  printf("loop_to_video took %.3f s\n", (t1 - t0) / 1000.0);

  return 0;
}