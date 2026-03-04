/**
 * Meshes Sky
 *
 * Interactive 3D mesh viewer with a procedural sky background and sun.
 * Supports multiple meshes with optional textures.
 * Controls:
 *   Mouse drag       - orbit camera
 *   Mouse wheel      - zoom
 *   Left/Right arrow - cycle through meshes
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
  Triangle floor_tris[2];
  i32      current_mesh;
  i32      pending_mesh;
} App;

/* =============================================================================
 * Global State
 * ========================================================================== */

static bool g_mouse_down = false;
static Vec2  g_mouse_pos = { 0 };
static DirectionalLightParams* g_directional_light = NULL;

/* =============================================================================
 * Sky / Background Rendering
 * ========================================================================== */

static Color render_sky(Ray ray, void* ctx) {
  (void)ctx;
  Vec3 dir = ray.dir;
  normalize_vec3(dir, &dir);

  /* Sky gradient: horizon (light blue) → zenith (dark blue) */
  Color sky_horizon = { 0.5f, 0.7f, 1.0f, 1.0f };
  Color sky_zenith = { 0.1f, 0.2f, 0.4f, 1.0f };
  f32 blend = powf(fmaxf(0.0f, dir.z), 0.5f);
  Color sky_color = add_color(
    scale_color(sky_horizon, 1.0f - blend),
    scale_color(sky_zenith, blend)
  );

  /* Sun effect */
  Vec3 sun_dir = vec3(0.7f, 0.3f, 0.5f);
  normalize_vec3(sun_dir, &sun_dir);

  f32 sun_dot = fmaxf(0.0f, dot_vec3(dir, sun_dir));
  f32 sun_glow = powf(sun_dot, 200.0f);
  f32 atm_glow = powf(sun_dot, 5.0f);

  Color sun_color = { 1.0f, 0.8f, 0.5f, 1.0f };
  Color sun_effect = add_color(
    scale_color(sun_color, sun_glow * 2.0f),
    scale_color(sun_color, atm_glow * 0.5f)
  );

  return add_color(sky_color, sun_effect);
}

/* =============================================================================
 * Mesh Transform Helpers
 * ========================================================================== */

typedef struct { Vec3 center; f32 scale_inv; } NormalizeCtx;

static Vec3 transform_normalize(Vec3 v, void* ctx) {
  NormalizeCtx* c = (NormalizeCtx*)ctx;
  return scale_vec3(sub_vec3(v, c->center), c->scale_inv);
}

/*
 * Compose the three JS rotations in order:
 *   1) Vec3(-v.y, v.x, v.z)
 *   2) Vec3(v.z, v.y, -v.x)   applied to result of 1
 *   3) Vec3(-v.x, -v.y, v.z)  applied to result of 2
 * Net result for input (x, y, z): (-z, -x, y)
 */
static Vec3 transform_rotate(Vec3 v, void* ctx) {
  (void)ctx;
  return vec3(-v.z, -v.x, v.y);
}

static Vec3 transform_translate(Vec3 v, void* ctx) {
  (void)ctx;
  return add_vec3(v, vec3(1.5f, 1.5f, 1.0f));
}

static Color white_color_mapper(Vec3 v, void* ctx) {
  (void)v; (void)ctx;
  return COLOR_WHITE;
}

static Material diffuse_material_mapper(Face face, void* ctx) {
  (void)face; (void)ctx;
  return build_diffuse_material();
}

/* =============================================================================
 * Floor Triangles
 * ========================================================================== */

static void build_floor(App* app) {
  Material* mat = (Material*)malloc(sizeof(Material));
  *mat = build_diffuse_material();

  /* bottom-1: (0,0,0) → (3,0,0) → (3,3,0) */
  app->floor_tris[0] = build_triangle(vec3(0, 0, 0), vec3(3, 0, 0), vec3(3, 3, 0));
  {
    RasterTriangleProps* p = (RasterTriangleProps*)malloc(sizeof(RasterTriangleProps));
    p->colors[0] = COLOR_RED; p->colors[1] = COLOR_RED; p->colors[2] = COLOR_RED;
    p->tex_coords[0] = vec2(0, 0); p->tex_coords[1] = vec2(1, 0); p->tex_coords[2] = vec2(0, 1);
    p->texture = NULL;
    p->material = mat;
    app->floor_tris[0].props = p;
  }

  /* bottom-2: (3,3,0) → (0,3,0) → (0,0,0) */
  app->floor_tris[1] = build_triangle(vec3(3, 3, 0), vec3(0, 3, 0), vec3(0, 0, 0));
  {
    RasterTriangleProps* p = (RasterTriangleProps*)malloc(sizeof(RasterTriangleProps));
    p->colors[0] = COLOR_RED; p->colors[1] = COLOR_RED; p->colors[2] = COLOR_RED;
    p->tex_coords[0] = vec2(0, 0); p->tex_coords[1] = vec2(1, 0); p->tex_coords[2] = vec2(0, 1);
    p->texture = NULL;
    p->material = mat;
    app->floor_tris[1].props = p;
  }
}

/* =============================================================================
 * Mesh Loading
 * ========================================================================== */

static void load_mesh(App* app, i32 index) {
  if (index < 0 || (u32)index >= MESH_COUNT) return;

  /* Read and parse OBJ */
  String obj = io_read_file(MESH_TABLE[index].mesh_path);
  Mesh   mesh = read_obj_mesh(obj, "mesh");

  /* Normalize to fit inside a unit cube, then place at scene center */
  AABB box = get_bounding_box_mesh(&mesh);
  f32  scale_inv = 2.0f / max_comp_vec3(box.diagonal);
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

  /* Rebuild scene: floor + new mesh */
  clear_scene_elems_scene(app->scene);
  add_scene_elem_scene(app->scene, build_scene_elem_triangle(app->floor_tris[0]));
  add_scene_elem_scene(app->scene, build_scene_elem_triangle(app->floor_tris[1]));
  Array mesh_elems = triangles_to_scene_elems(get_triangles_mesh(&mesh));
  add_scene_elems_scene(app->scene, mesh_elems);
  free_array(&mesh_elems);
  app->scene->vtable->rebuild_scene(app->scene);

  /* Reset progressive accumulation */
  app->tela->iterations = 1;
  app->current_mesh = index;
}

/* =============================================================================
 * Animation Loop
 * ========================================================================== */

static void on_frame(f32 dt, f32 time, void* ctx) {
  App* app = (App*)ctx;

  /* Handle deferred mesh switch */
  if (app->pending_mesh != app->current_mesh) {
    load_mesh(app, app->pending_mesh);
  }

  Vec3 light_dir = vec3(0.7f, 0.3f, 0.5f);
  normalize_vec3(light_dir, &light_dir);

  RaytraceParams params = {
      .samples_per_pixel = 1,
      .bounces = 10,
      .variance = 0.001f,
      .gamma = 0.5f,
      .bilinear_texture = false,
      .is_biased = false,
      .camera = app->camera,
      .render_background = render_sky,
      .render_background_context = NULL,
      .exposed_tela = app->tela,
      .directional_light = g_directional_light,
  };

  ray_trace_scene_parallel(app->scene, &params);

  set_window_title(app->window,
    format_string("Meshes Sky | [%d/%d] %s | FPS: %.2f | Left/Right to switch",
      app->current_mesh + 1, (i32)MESH_COUNT,
      MESH_TABLE[app->current_mesh].mesh_path,
      1.0f / dt));

  paint_window(app->window, app->tela);
  // debug_scene(app->scene, &(SceneDebugProps){.camera = app->camera, .tela = app->tela});
  // paint_window(app->window, app->tela);
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

  set_orbit_camera(app->camera,
    orbit.x, orbit.y + theta_delta, orbit.z + phi_delta);

  g_mouse_pos = new_pos;
  app->tela->iterations = 1;
}

static void on_mouse_scroll(Window* w, i32 delta_y, void* ctx) {
  (void)w;
  App* app = (App*)ctx;
  Vec3 orbit = get_camera_orbit(app->camera);
  set_orbit_camera(app->camera,
    orbit.x + delta_y * 0.1f, orbit.y, orbit.z);
  app->tela->iterations = 1;
}

static void on_key_down(Window* w, u32 keycode, void* ctx) {
  (void)w;
  App* app = (App*)ctx;
  if (keycode == SDLK_RIGHT) {
    app->pending_mesh = (app->current_mesh + 1) % (i32)MESH_COUNT;
  }
  else if (keycode == SDLK_LEFT) {
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
 * Main
 * ========================================================================== */

int main(void) {
  Vec3 light_dir = vec3(0.7f, 0.3f, 0.5f);
  normalize_vec3(light_dir, &light_dir);
  g_directional_light = &(DirectionalLightParams) { .direction = light_dir, .sharpness = 200.0f };

  Tela* tela = new_tela(WIDTH, HEIGHT);
  Window* window = new_window(WIDTH * 2, HEIGHT * 2, "Meshes Sky");

  /* Camera orbiting around the mesh center (1.5, 1.5, 1.0), radius 3 */
  Camera camera = create_camera(vec3(0, 0, 0), vec3(1.5f, 1.5f, 1.0f), 1.0f);
  set_orbit_camera(&camera, 3.0f, 0.0f, 0.0f);

  /* Scene */
  Scene scene = new_kscene(10);

  App app = {
      .tela = tela,
      .window = window,
      .camera = &camera,
      .scene = &scene,
      .current_mesh = 0,
      .pending_mesh = 0,
  };

  /* Build floor and load initial mesh */
  build_floor(&app);
  load_mesh(&app, 0);

  /* Animation loop */
  Loop* animation = loop(on_frame, &app);

  on_close_window(window, on_close, animation);
  register_input_handlers(window, &app);

  play_loop(animation);

  return 0;
}
