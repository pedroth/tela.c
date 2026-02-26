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
 * Types
 * ========================================================================== */

typedef struct {
  Tela* tela;
  Window* window;
  Camera* camera;
  Scene* scene;
} App;

/* =============================================================================
 * Global State
 * ========================================================================== */

static bool g_mouse_down = false;
static Vec2 g_mouse_pos = { 0 };
static Tela* g_background = NULL;

/* =============================================================================
 * Animation / Render Loop
 * ========================================================================== */

static inline Color default_render_background(Ray ray, void* ctx) {
  return COLOR_BLACK;
}

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
  Scene* scene = app->scene;
  Camera* camera = app->camera;
  Tela* exposed = app->tela;

  RaytraceParams params = {
    .samples_per_pixel = 1,
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
    format_string("Cornell Box | FPS: %.2f", 1.0f / dt));
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

static void register_input_handlers(Window* window, App* app) {
  on_mouse_down_window(window, on_mouse_down, app);
  on_mouse_up_window(window, on_mouse_up, app);
  on_mouse_move_window(window, on_mouse_move, app);
  on_mouse_scroll_window(window, on_mouse_scroll, app);
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

  // left-1
  Triangle left_1 = build_triangle(vec3(3, 0, 3), vec3(3, 0, 0), vec3(0, 0, 0));
  left_1.props = new_tri_props(COLOR_RED, mat_diffuse);
  add_triangle_scene(scene, left_1);

  // left-2
  Triangle left_2 = build_triangle(vec3(0, 0, 0), vec3(0, 0, 3), vec3(3, 0, 3));
  left_2.props = new_tri_props(COLOR_RED, mat_diffuse);
  add_triangle_scene(scene, left_2);

  // right-1
  Triangle right_1 = build_triangle(vec3(0, 3, 0), vec3(3, 3, 0), vec3(3, 3, 3));
  right_1.props = new_tri_props(COLOR_GREEN, mat_diffuse);
  add_triangle_scene(scene, right_1);

  // right-2
  Triangle right_2 = build_triangle(vec3(3, 3, 3), vec3(0, 3, 3), vec3(0, 3, 0));
  right_2.props = new_tri_props(COLOR_GREEN, mat_diffuse);
  add_triangle_scene(scene, right_2);

  // bottom-1
  Triangle bottom_1 = build_triangle(vec3(0, 0, 0), vec3(3, 0, 0), vec3(3, 3, 0));
  bottom_1.props = new_tri_props(COLOR_WHITE, mat_diffuse);
  add_triangle_scene(scene, bottom_1);

  // bottom-2
  Triangle bottom_2 = build_triangle(vec3(3, 3, 0), vec3(0, 3, 0), vec3(0, 0, 0));
  bottom_2.props = new_tri_props(COLOR_WHITE, mat_diffuse);
  add_triangle_scene(scene, bottom_2);

  // top-1
  Triangle top_1 = build_triangle(vec3(3, 3, 3), vec3(3, 0, 3), vec3(0, 0, 3));
  top_1.props = new_tri_props(COLOR_WHITE, mat_diffuse);
  add_triangle_scene(scene, top_1);

  // top-2
  Triangle top_2 = build_triangle(vec3(0, 0, 3), vec3(0, 3, 3), vec3(3, 3, 3));
  top_2.props = new_tri_props(COLOR_WHITE, mat_diffuse);
  add_triangle_scene(scene, top_2);

  // back-1
  Triangle back_1 = build_triangle(vec3(0, 0, 0), vec3(0, 3, 0), vec3(0, 3, 3));
  back_1.props = new_tri_props(COLOR_WHITE, mat_diffuse);
  add_triangle_scene(scene, back_1);

  // back-2
  Triangle back_2 = build_triangle(vec3(0, 3, 3), vec3(0, 0, 3), vec3(0, 0, 0));
  back_2.props = new_tri_props(COLOR_WHITE, mat_diffuse);
  add_triangle_scene(scene, back_2);

  // light-1
  Triangle light_1 = build_triangle(vec3(1, 1, 2.9), vec3(2, 1, 2.9), vec3(2, 2, 2.9));
  light_1.props = new_tri_props(COLOR_WHITE, mat_emissive);
  add_triangle_scene(scene, light_1);

  // light-2
  Triangle light_2 = build_triangle(vec3(2, 2, 2.9), vec3(1, 2, 2.9), vec3(1, 1, 2.9));
  light_2.props = new_tri_props(COLOR_WHITE, mat_emissive);
  add_triangle_scene(scene, light_2);

}

typedef struct {
    Vec3 center;
    f32 scale_inv;
} FirstTransformContext;

Vec3 first_transform(Vec3 v, void* ctx) {
    Vec3 center = ((FirstTransformContext*)ctx)->center;
    f32 scale_inv = ((FirstTransformContext*)ctx)->scale_inv;
    return scale_vec3(sub_vec3(v, center), scale_inv);
}

Vec3 second_transform(Vec3 v, void* ctx) {
    return vec3(-v.z, -v.x, v.y);
}

Vec3 third_transform(Vec3 v, void* ctx) {
    return add_vec3(v, vec3(1.5f, 1.5f, 1.0f));
}

Material material_mapper(Face face, void* ctx) {
  return build_diffuse_material();
}

void add_mesh_scene(Scene* scene) {
  char* obj_files[] = {
        "./assets/megaman.obj",
        "./assets/statue.obj",
        "./assets/JesusMary.obj",
        "./assets/spot.obj",
        "./assets/oil.obj",
        "./assets/riku.obj",
        "./assets/burger.obj",
    };
    char* texture_files[] = {
        "./assets/megaman.png",
        "./assets/statue.jpg",
        "./assets/JesusMary.jpg",
        "./assets/spot.png",
        "./assets/oil.png",
        "./assets/riku.png",
        "./assets/burger.jpg",
    };

    u32 mesh_index = 3;
    String obj = io_read_file(obj_files[mesh_index]);
    Mesh mesh = read_obj_mesh(obj, "mesh");
    AABB box = get_bounding_box_mesh(&mesh);
    f32 scale_inv = 2.0f / max_comp_vec3(box.diagonal);
    map_vertices_mesh(&mesh, first_transform, &(FirstTransformContext){ .center = box.center, .scale_inv = scale_inv });
    map_vertices_mesh(&mesh, second_transform, NULL);
    map_vertices_mesh(&mesh, third_transform, NULL);
    add_texture_mesh(&mesh, io_read_image(texture_files[mesh_index]));

    map_triangles_materials_mesh(&mesh, material_mapper, NULL);

    add_triangles_scene(scene, get_triangles_mesh(&mesh));
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
  add_mesh_scene(&scene);
  scene.vtable->rebuild_scene(&scene); // force build before rendering

  App app = {
      .tela = tela,
      .window = window,
      .camera = &camera,
      .scene = &scene,
  };

  Loop* animation = loop(on_frame, &app);
  on_close_window(window, on_close, animation);
  register_input_handlers(window, &app);

  play_loop(animation);

  return 0;
}
