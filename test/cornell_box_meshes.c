/**
 * Cornell Box
 *
 * A path-traced Cornell box scene with diffuse, metallic, dielectric, and
 * alpha materials. Features interactive orbit camera controls via mouse.
 *
 */

#include "../src/index.c"

 /* =============================================================================
  * Constants
  * ========================================================================== */

static const u32 WIDTH = 640 / 2;
static const u32 HEIGHT = 480 / 2;

/* =============================================================================
 * Mesh Table
 * ========================================================================== */

typedef struct {
  const char* mesh_path;
  const char* texture_path; /* NULL if no texture */
} MeshEntry;

static const MeshEntry MESH_TABLE[] = {
    { "./assets/spot.obj",       "./assets/spot.png"    },
    { "./assets/megaman.obj",    "./assets/megaman.png" },
    { "./assets/bunny_orig.obj", NULL                   },
    { "./assets/riku.obj",       "./assets/riku.png"    },
    { "./assets/oil.obj",        "./assets/oil.png"     },
    { "./assets/earth.obj",        "./assets/earth.jpg"     },
    { "./assets/moses.obj", NULL },
    { "./assets/dragonHD.obj", NULL },
    { "./assets/statue.obj",     "./assets/statue.jpg"  },
    { "./assets/burger.obj",     "./assets/burger.jpg"  },
    { "./assets/rocks.obj",     "./assets/rocks.jpg"   },
    { "./assets/JesusMary.obj", "./assets/JesusMary.jpg" },
};

static const u32 MESH_COUNT = sizeof(MESH_TABLE) / sizeof(MESH_TABLE[0]);

/* =============================================================================
 * Types
 * ========================================================================== */

typedef struct {
  Tela* tela;
  Window* window;
  Camera* camera;
  Scene* scene;
  i32 current_mesh;
  i32 pending_mesh;
} App;

/* Forward declarations */
static void load_mesh(App* app, i32 index);
static void build_cornell_box(Scene* scene);

/* =============================================================================
 * Global State
 * ========================================================================== */

static bool g_mouse_down = false;
static Vec2 g_mouse_pos = { 0 };
static Tela* g_background = NULL;

/* =============================================================================
 * Animation / Render Loop
 * ========================================================================== */

static inline Color render_background(Ray ray, void* ctx) {
  Tela* background = (Tela*)ctx;
  if (!background) {
    return (Color){ 0.0f, 0.0f, 0.0f, 1.0f };
  }
Vec3 dir = ray.dir;
  f32 theta = atan2f(dir.y, dir.x) / (2.0f * PI) + 0.5f;
  f32 alpha = acosf(fminf(1.0f, fmaxf(-1.0f, -dir.z))) / PI;
  return get_pxl_tela(background,
    (u32)(theta * background->width),
    (u32)(alpha * background->height));
}

static void on_frame(f32 dt, f32 time, void* ctx) {
  App* app = (App*)ctx;

  /* Handle deferred mesh switch */
  if (app->pending_mesh != app->current_mesh) {
    load_mesh(app, app->pending_mesh);
  }

  Scene* scene = app->scene;
  Camera* camera = app->camera;
  Tela* exposed = app->tela;

  RaytraceParams params = {
    .samples_per_pixel = 3,
    .bounces = 10,
    .variance = 0.001f,
    .gamma = 0.5f,
    .bilinear_texture = true,
    .is_biased = false,
    .camera = camera,
    .render_background = render_background,
    .render_background_context = g_background,
    .exposed_tela = exposed,
  };

  ray_trace_scene_parallel(scene, &params);

  set_window_title(app->window,
    format_string("Cornell Box | [%d/%d] %s | FPS: %.2f | Left/Right to switch",
      app->current_mesh + 1, (i32)MESH_COUNT,
      MESH_TABLE[app->current_mesh].mesh_path,
      1.0f / dt));
  paint_window(app->window, app->tela);
}

static void on_close(Window* window, void* ctx) {
  Loop* animation = (Loop*)ctx;
  stop_loop(animation);
}

/* =============================================================================
 * Input Handlers
 * ========================================================================== */

static void on_mouse_down(Window* w, i32 x, i32 y, u32 button, void* ctx) {
  g_mouse_down = true;
  g_mouse_pos = vec2((f32)x, (f32)y);
}

static void on_mouse_up(Window* w, i32 x, i32 y, u32 button, void* ctx) {
  g_mouse_down = false;
}

static void on_mouse_move(Window* w, i32 x, i32 y, void* ctx) {
  if (!g_mouse_down) return;

  Vec2 new_pos = vec2((f32)x, (f32)y);
  if (equals_vec2(new_pos, g_mouse_pos)) return;

  App* app = (App*)ctx;
  Vec2 delta = sub_vec2(new_pos, g_mouse_pos);

  Vec3 orbit = get_camera_orbit(app->camera);
  f32 theta_delta = -2.0f * PI * (delta.x / WIDTH);
  f32 phi_delta = -2.0f * PI * (delta.y / HEIGHT);

  set_orbit_camera(app->camera, orbit.x, orbit.y + theta_delta,
    orbit.z + phi_delta);

  g_mouse_pos = new_pos;
  Tela* exposed = app->tela;
  exposed->iterations = 1;
}

static void on_mouse_scroll(Window* w, i32 delta_y, void* ctx) {
  App* app = (App*)ctx;
  Vec3 orbit = get_camera_orbit(app->camera);
  f32 new_radius = orbit.x + delta_y * 0.1f;
  set_orbit_camera(app->camera, new_radius, orbit.y, orbit.z);
  Tela* exposed = app->tela;
  exposed->iterations = 1;
}

static void on_key_down(Window* w, u32 keycode, void* ctx) {
  App* app = (App*)ctx;
  if (keycode == SDLK_RIGHT) {
    app->pending_mesh = (app->current_mesh + 1) % (i32)MESH_COUNT;
  } else if (keycode == SDLK_LEFT) {
    app->pending_mesh = ((app->current_mesh - 1) + (i32)MESH_COUNT) % (i32)MESH_COUNT;
  }
}

static void register_input_handlers(Window* window, App* app) {
  on_mouse_down_window(window, on_mouse_down, app);
  on_mouse_up_window(window, on_mouse_up, app);
  on_mouse_move_window(window, on_mouse_move, app);
  on_mouse_scroll_window(window, on_mouse_scroll, app);
  on_key_down_window(window, on_key_down, app);
}



/* =============================================================================
 * Build Scene
 * ========================================================================== */

static RasterTriangleProps* new_tri_props(Color c, Material* mat) {
  RasterTriangleProps* p = (RasterTriangleProps*)malloc(sizeof(RasterTriangleProps));
  p->colors[0] = c; p->colors[1] = c; p->colors[2] = c;
  p->tex_coords[0] = vec2(0, 0);
  p->tex_coords[1] = vec2(1, 0);
  p->tex_coords[2] = vec2(0, 1);
  p->texture = NULL;
  p->material = mat;
  return p;
}

static void build_cornell_box(Scene* scene) {
  Material* mat_diffuse = (Material*)malloc(sizeof(Material));
  *mat_diffuse = build_diffuse_material();
  Material* mat_emissive = (Material*)malloc(sizeof(Material));
  *mat_emissive = build_emissive_material();
  Material* mat_metallic_025 = (Material*)malloc(sizeof(Material));
  *mat_metallic_025 = build_metallic_material(0.25);
  Material* mat_metallic_0 = (Material*)malloc(sizeof(Material));
  *mat_metallic_0 = build_metallic_material(0.0);
  Material* mat_dielectric_13 = (Material*)malloc(sizeof(Material));
  *mat_dielectric_13 = build_dielectric_material(1.3);
  Material* mat_dielectric_2 = (Material*)malloc(sizeof(Material));
  *mat_dielectric_2 = build_dielectric_material(2.0);
  Material* mat_alpha_025 = (Material*)malloc(sizeof(Material));
  *mat_alpha_025 = build_alpha_material(0.25);

  // corner emissive triangle
  Triangle corner_light = build_triangle(vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1));
  corner_light.props = new_tri_props(COLOR_YELLOW, mat_emissive);
  add_scene_elem_scene(scene, build_scene_elem_triangle(corner_light));

  // left-1
  Triangle left_1 = build_triangle(vec3(3, 0, 3), vec3(3, 0, 0), vec3(0, 0, 0));
  left_1.props = new_tri_props(COLOR_RED, mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(left_1));

  // left-2
  Triangle left_2 = build_triangle(vec3(0, 0, 0), vec3(0, 0, 3), vec3(3, 0, 3));
  left_2.props = new_tri_props(COLOR_RED, mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(left_2));

  // right-1
  Triangle right_1 = build_triangle(vec3(0, 3, 0), vec3(3, 3, 0), vec3(3, 3, 3));
  right_1.props = new_tri_props(COLOR_GREEN, mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(right_1));

  // right-2
  Triangle right_2 = build_triangle(vec3(3, 3, 3), vec3(0, 3, 3), vec3(0, 3, 0));
  right_2.props = new_tri_props(COLOR_GREEN, mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(right_2));

  // bottom-1
  Triangle bottom_1 = build_triangle(vec3(0, 0, 0), vec3(3, 0, 0), vec3(3, 3, 0));
  bottom_1.props = new_tri_props(COLOR_WHITE, mat_diffuse);
  add_scene_elem_scene(scene, build_scene_elem_triangle(bottom_1));

  // bottom-2
  Triangle bottom_2 = build_triangle(vec3(3, 3, 0), vec3(0, 3, 0), vec3(0, 0, 0));
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
  Triangle light_1 = build_triangle(vec3(1, 1, 2.9), vec3(2, 1, 2.9), vec3(2, 2, 2.9));
  light_1.props = new_tri_props(COLOR_WHITE, mat_emissive);
  add_scene_elem_scene(scene, build_scene_elem_triangle(light_1));

  // light-2
  Triangle light_2 = build_triangle(vec3(2, 2, 2.9), vec3(1, 2, 2.9), vec3(1, 1, 2.9));
  light_2.props = new_tri_props(COLOR_WHITE, mat_emissive);
  add_scene_elem_scene(scene, build_scene_elem_triangle(light_2));

}

/* =============================================================================
 * Mesh Transform Helpers
 * ========================================================================== */

typedef struct { Vec3 center; f32 scale_inv; } NormalizeCtx;

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

static Material diffuse_material_mapper(Face face, void* ctx) {
  return build_diffuse_material();
}

static void load_mesh(App* app, i32 index) {
  if (index < 0 || (u32)index >= MESH_COUNT) return;

  /* Read and parse OBJ */
  String obj = io_read_file(MESH_TABLE[index].mesh_path);
  Mesh mesh = read_obj_mesh(obj, "mesh");

  /* Normalize to fit inside a unit cube, then place at scene center */
  AABB box = get_bounding_box_mesh(&mesh);
  f32 scale_inv = 2.0f / max_comp_vec3(box.diagonal);
  NormalizeCtx nctx = { .center = box.center, .scale_inv = scale_inv };

  map_vertices_mesh(&mesh, transform_normalize, &nctx);
  map_vertices_mesh(&mesh, transform_rotate, NULL);
  map_vertices_mesh(&mesh, transform_translate, NULL);

  /* Set all vertex colors to white */
  map_colors_mesh(&mesh, white_color_mapper, NULL);

  /* Optional texture */
  if (MESH_TABLE[index].texture_path != NULL) {
    add_texture_mesh(&mesh, io_read_image(MESH_TABLE[index].texture_path));
  }

  /* Assign diffuse material to every triangle */
  map_triangles_materials_mesh(&mesh, diffuse_material_mapper, NULL);

  /* Rebuild scene: cornell box + new mesh */
  clear_scene_elems_scene(app->scene);
  build_cornell_box(app->scene);
  Array mesh_elems = triangles_to_scene_elems(get_triangles_mesh(&mesh));
  add_scene_elems_scene(app->scene, mesh_elems);
  free_array(&mesh_elems);
  app->scene->vtable->rebuild_scene(app->scene);

  /* Reset progressive accumulation */
  app->tela->iterations = 1;
  app->current_mesh = index;
}

/* =============================================================================
 * Main
 * ========================================================================== */

int main(void) {
  g_background = io_read_image("assets/sky.jpg");

  Tela* tela = new_tela(WIDTH, HEIGHT);
  Window* window = new_window(WIDTH * 2, HEIGHT * 2, "Cornell Box");

  /* camera looks at center of box, orbiting at distance 3 */
  Camera camera = create_camera(vec3(3.0f, 0.0f, 0.0f), vec3(1.5f, 1.5f, 1.5f), 1.0f);
  set_orbit_camera(&camera, 3.0f, 0.0f, 0.0f);

  Scene scene = new_kscene(20);
  build_cornell_box(&scene);

  App app = {
      .tela = tela,
      .window = window,
      .camera = &camera,
      .scene = &scene,
      .current_mesh = -1,
      .pending_mesh = 0,
  };

  load_mesh(&app, 0);

  Loop* animation = loop(on_frame, &app);
  on_close_window(window, on_close, animation);
  register_input_handlers(window, &app);

  play_loop(animation);

  return 0;
}
