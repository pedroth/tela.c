#include "../src/index.c"
#include <ctype.h>

/* =============================================================================
 * Constants
 * ========================================================================== */

static const char* SCENE_FILE = "assets/staircase.scene";
static const char* SCENE_BASE_DIR = "assets/";

// scene file resolution is portrait (640x800); render landscape instead
static const u32 WIDTH = 800 / 2;
static const u32 HEIGHT = 640 / 2;

/* =============================================================================
 * Scene file (GLSL-PathTracer .scene format) parsing
 * ========================================================================== */

typedef struct {
  char name[128];
  bool has_color;
  Color color;
  bool has_texture;
  char texture_path[256];
  f32 roughness;
  f32 metallic;
  f32 spectrans;
  f32 ior;
} SceneMaterialDef;

typedef struct {
  char file_path[256];
  char material_name[128];
  Vec3 scale;
  Vec3 translate;
} SceneMeshDef;

typedef struct {
  Vec3 position;
  Vec3 v1;
  Vec3 v2;
  Color emission;
  char type[32];
} SceneLightDef;

typedef struct {
  Array tokens;  // char*
  u32 pos;
} SceneParser;

static Array tokenize_scene_file(char* buffer) {
  Array tokens = new_array(256, sizeof(char*));
  char* p = buffer;
  while (*p) {
    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p) break;
    char* start = p;
    while (*p && !isspace((unsigned char)*p)) p++;
    if (*p) {
      *p = '\0';
      p++;
    }
    push_array(&tokens, &start);
  }
  return tokens;
}

static char* next_token(SceneParser* parser) {
  if (parser->pos >= parser->tokens.length)
    return NULL;
  char* t = *(char**)get_array_element(&parser->tokens, parser->pos);
  parser->pos++;
  return t;
}

static f32 next_float(SceneParser* parser) {
  char* t = next_token(parser);
  return t ? (f32)atof(t) : 0.0f;
}

static Vec3 next_vec3(SceneParser* parser) {
  f32 x = next_float(parser);
  f32 y = next_float(parser);
  f32 z = next_float(parser);
  return vec3(x, y, z);
}

static void skip_generic_block(SceneParser* parser) {
  char* t;
  while ((t = next_token(parser)) != NULL && strcmp(t, "}") != 0) {
    // unrecognized/unused block (e.g. renderer settings) - discard tokens
  }
}

static void parse_camera_block(
    SceneParser* parser, Vec3* out_pos, Vec3* out_lookat, f32* out_fov
) {
  char* t;
  while ((t = next_token(parser)) != NULL && strcmp(t, "}") != 0) {
    if (strcmp(t, "position") == 0)
      *out_pos = next_vec3(parser);
    else if (strcmp(t, "lookat") == 0)
      *out_lookat = next_vec3(parser);
    else if (strcmp(t, "fov") == 0)
      *out_fov = next_float(parser);
  }
}

static void parse_material_block(SceneParser* parser, SceneMaterialDef* def) {
  char* t;
  while ((t = next_token(parser)) != NULL && strcmp(t, "}") != 0) {
    if (strcmp(t, "color") == 0) {
      f32 r = next_float(parser);
      f32 g = next_float(parser);
      f32 b = next_float(parser);
      def->color = (Color){ r, g, b, 1.0f };
      def->has_color = true;
    } else if (strcmp(t, "albedotexture") == 0) {
      char* path = next_token(parser);
      snprintf(
          def->texture_path, sizeof(def->texture_path), "%s", path ? path : ""
      );
      def->has_texture = true;
    } else if (strcmp(t, "roughness") == 0) {
      def->roughness = next_float(parser);
    } else if (strcmp(t, "metallic") == 0) {
      def->metallic = next_float(parser);
    } else if (strcmp(t, "spectrans") == 0) {
      def->spectrans = next_float(parser);
    } else if (strcmp(t, "ior") == 0) {
      def->ior = next_float(parser);
    }
    // else: malformed/unknown line (e.g. WoodLamp's stray value) - ignore
  }
}

static void parse_mesh_block(SceneParser* parser, SceneMeshDef* def) {
  char* t;
  while ((t = next_token(parser)) != NULL && strcmp(t, "}") != 0) {
    if (strcmp(t, "file") == 0) {
      char* path = next_token(parser);
      snprintf(def->file_path, sizeof(def->file_path), "%s", path ? path : "");
    } else if (strcmp(t, "material") == 0) {
      char* name = next_token(parser);
      snprintf(
          def->material_name, sizeof(def->material_name), "%s", name ? name : ""
      );
    } else if (strcmp(t, "scale") == 0) {
      def->scale = next_vec3(parser);
    } else if (strcmp(t, "translate") == 0) {
      def->translate = next_vec3(parser);
    }
  }
}

static void parse_light_block(SceneParser* parser, SceneLightDef* def) {
  char* t;
  while ((t = next_token(parser)) != NULL && strcmp(t, "}") != 0) {
    if (strcmp(t, "position") == 0)
      def->position = next_vec3(parser);
    else if (strcmp(t, "v1") == 0)
      def->v1 = next_vec3(parser);
    else if (strcmp(t, "v2") == 0)
      def->v2 = next_vec3(parser);
    else if (strcmp(t, "emission") == 0) {
      f32 r = next_float(parser);
      f32 g = next_float(parser);
      f32 b = next_float(parser);
      def->emission = (Color){ r, g, b, 1.0f };
    } else if (strcmp(t, "type") == 0) {
      char* type = next_token(parser);
      snprintf(def->type, sizeof(def->type), "%s", type ? type : "");
    }
  }
}

static void parse_scene(
    char* buffer,
    Array* materials,
    Array* meshes,
    Array* lights,
    Vec3* cam_pos,
    Vec3* cam_lookat,
    f32* cam_fov
) {
  Array tokens = tokenize_scene_file(buffer);
  SceneParser parser = { tokens, 0 };
  char* t;
  while ((t = next_token(&parser)) != NULL) {
    if (strcmp(t, "renderer") == 0) {
      next_token(&parser);  // "{"
      skip_generic_block(&parser);
    } else if (strcmp(t, "camera") == 0) {
      next_token(&parser);  // "{"
      parse_camera_block(&parser, cam_pos, cam_lookat, cam_fov);
    } else if (strcmp(t, "material") == 0) {
      SceneMaterialDef def = { 0 };
      def.roughness = 0.5f;
      char* name = next_token(&parser);
      snprintf(def.name, sizeof(def.name), "%s", name ? name : "");
      next_token(&parser);  // "{"
      parse_material_block(&parser, &def);
      push_array(materials, &def);
    } else if (strcmp(t, "mesh") == 0) {
      SceneMeshDef def = { 0 };
      def.scale = vec3(1.0f, 1.0f, 1.0f);
      next_token(&parser);  // "{"
      parse_mesh_block(&parser, &def);
      push_array(meshes, &def);
    } else if (strcmp(t, "light") == 0) {
      SceneLightDef def = { 0 };
      next_token(&parser);  // "{"
      parse_light_block(&parser, &def);
      push_array(lights, &def);
    }
    // else: stray token outside any known block - ignore
  }
  free_array(&tokens);
}

static SceneMaterialDef* find_material_def(Array* materials, const char* name) {
  for (u32 i = 0; i < materials->length; i++) {
    SceneMaterialDef* def = (SceneMaterialDef*)get_array_element(materials, i);
    if (strcmp(def->name, name) == 0)
      return def;
  }
  return NULL;
}

/* =============================================================================
 * Mesh / material building
 * ========================================================================== */

static Vec3 scale_vertex_mapper(Vec3 v, void* ctx) {
  Vec3* s = (Vec3*)ctx;
  return vec3(v.x * s->x, v.y * s->y, v.z * s->z);
}

static Vec3 translate_vertex_mapper(Vec3 v, void* ctx) {
  Vec3* t = (Vec3*)ctx;
  return add_vec3(v, *t);
}

static Color uniform_color_mapper(Vec3 v, void* ctx) {
  (void)v;
  return *(Color*)ctx;
}

// .scene files are Y-up (GLSL-PathTracer); this engine's camera orbit math is Z-up
static Vec3 to_engine_coords(Vec3 v) {
  return vec3(v.x, -v.z, v.y);
}

static Vec3 axis_swap_vertex_mapper(Vec3 v, void* ctx) {
  (void)ctx;
  return to_engine_coords(v);
}

static Material scene_material_mapper(Face face, void* ctx) {
  (void)face;
  return *(Material*)ctx;
}

static void load_scene_mesh(Scene* scene, SceneMeshDef* mesh_def, Array* materials) {
  char full_path[512];
  snprintf(full_path, sizeof(full_path), "%s%s", SCENE_BASE_DIR, mesh_def->file_path);

  String obj_text = io_read_file(full_path);
  if (!obj_text.data) {
    fprintf(stderr, "scene_raytrace: failed to read mesh '%s'\n", full_path);
    return;
  }

  Mesh mesh = read_obj_mesh(obj_text, mesh_def->material_name);
  free(obj_text.data);

  map_vertices_mesh(&mesh, scale_vertex_mapper, &mesh_def->scale);
  if (!equals_vec3(mesh_def->translate, vec3(0, 0, 0)))
    map_vertices_mesh(&mesh, translate_vertex_mapper, &mesh_def->translate);
  map_vertices_mesh(&mesh, axis_swap_vertex_mapper, NULL);

  SceneMaterialDef* mat_def = find_material_def(materials, mesh_def->material_name);

  Material material;
  if (mat_def != NULL && mat_def->spectrans > 0.0f) {
    material = build_dielectric_material(mat_def->ior > 0.0f ? mat_def->ior : 1.5f);
  } else if (mat_def != NULL && mat_def->metallic > 0.5f) {
    material = build_metallic_material(mat_def->roughness);
  } else {
    material = build_diffuse_material();
  }

  if (mat_def != NULL && mat_def->has_texture) {
    char texture_path[512];
    snprintf(
        texture_path, sizeof(texture_path), "%s%s", SCENE_BASE_DIR, mat_def->texture_path
    );
    Tela* texture = io_read_image(texture_path);
    if (texture) {
      add_texture_mesh(&mesh, texture);
    } else {
      fprintf(stderr, "scene_raytrace: failed to load texture '%s'\n", texture_path);
    }
  } else {
    Color color = (mat_def != NULL && mat_def->has_color) ? mat_def->color : COLOR_WHITE;
    map_colors_mesh(&mesh, uniform_color_mapper, &color);
  }

  map_triangles_materials_mesh(&mesh, scene_material_mapper, &material);

  Array triangles = get_triangles_mesh(&mesh);
  Array elems = triangles_to_scene_elems(triangles);
  add_scene_elems_scene(scene, elems);
  free_array(&elems);
  free_array(&triangles);
}

static void load_scene_light(Scene* scene, SceneLightDef* light_def) {
  if (strcmp(light_def->type, "quad") != 0) {
    fprintf(stderr, "scene_raytrace: unsupported light type '%s'\n", light_def->type);
    return;
  }

  // position, v1, v2 are absolute corners of a parallelogram (GLSL-PathTracer convention)
  Vec3 p0 = to_engine_coords(light_def->position);
  Vec3 p1 = to_engine_coords(light_def->v1);
  Vec3 p2 = to_engine_coords(light_def->v2);
  Vec3 p3 = sub_vec3(add_vec3(p1, p2), p0);

  Mesh mesh = { 0 };
  mesh.name = create_string("scene_light");
  mesh.vertices = new_array(4, sizeof(Vec3));
  push_array(&mesh.vertices, &p0);
  push_array(&mesh.vertices, &p1);
  push_array(&mesh.vertices, &p3);
  push_array(&mesh.vertices, &p2);
  mesh.tex_coords = new_array(0, sizeof(Vec2));
  mesh.normals = new_array(0, sizeof(Vec3));

  mesh.colors = new_array(4, sizeof(Color));
  for (u32 i = 0; i < 4; i++) push_array(&mesh.colors, &light_def->emission);

  mesh.faces = new_array(2, sizeof(Face));
  Face face0 = { .vertex_indices = { 0, 1, 2 } };
  Face face1 = { .vertex_indices = { 0, 2, 3 } };
  push_array(&mesh.faces, &face0);
  push_array(&mesh.faces, &face1);

  Material emissive = build_emissive_material();
  map_triangles_materials_mesh(&mesh, scene_material_mapper, &emissive);

  Array triangles = get_triangles_mesh(&mesh);
  Array elems = triangles_to_scene_elems(triangles);
  add_scene_elems_scene(scene, elems);
  free_array(&elems);
  free_array(&triangles);
}

static Scene load_scene_file(
    const char* scene_path, Vec3* out_cam_pos, Vec3* out_cam_lookat, f32* out_cam_fov
) {
  String scene_text = io_read_file(scene_path);
  if (!scene_text.data) {
    fprintf(stderr, "scene_raytrace: failed to read scene file '%s'\n", scene_path);
    exit(1);
  }

  Array materials = new_array(32, sizeof(SceneMaterialDef));
  Array meshes = new_array(32, sizeof(SceneMeshDef));
  Array lights = new_array(4, sizeof(SceneLightDef));

  *out_cam_pos = vec3(0, 0, 3);
  *out_cam_lookat = vec3(0, 0, 0);
  *out_cam_fov = 60.0f;

  parse_scene(
      scene_text.data, &materials, &meshes, &lights, out_cam_pos, out_cam_lookat, out_cam_fov
  );
  free(scene_text.data);

  Scene scene = new_kscene(20);
  for (u32 i = 0; i < meshes.length; i++) {
    SceneMeshDef* mesh_def = (SceneMeshDef*)get_array_element(&meshes, i);
    load_scene_mesh(&scene, mesh_def, &materials);
  }
  for (u32 i = 0; i < lights.length; i++) {
    SceneLightDef* light_def = (SceneLightDef*)get_array_element(&lights, i);
    load_scene_light(&scene, light_def);
  }
  scene.vtable->rebuild_scene(&scene);

  free_array(&materials);
  free_array(&meshes);
  free_array(&lights);

  return scene;
}

/* =============================================================================
 * Types
 * ========================================================================== */

typedef struct {
  Tela* tela;
  Window* window;
  Camera* camera;
  Scene* scene;
  Vec3 cam_speed;
} App;

/* =============================================================================
 * Global State (for input handling)
 * ========================================================================== */

static bool g_mouse_down = false;
static Vec2 g_mouse_pos = { 0 };

/* =============================================================================
 * Animation Loop
 * ========================================================================== */

static inline Color render_background_ambient(Ray ray, void* ctx) {
  (void)ray;
  (void)ctx;
  return (Color){ 0.05f, 0.05f, 0.05f, 1.0f };
}

static void on_frame(f32 dt, f32 time, void* ctx) {
  App* app = (App*)ctx;
  (void)time;

  // Update camera position from WASD movement
  Vec3 world_speed = to_world_coord_camera(app->camera, app->cam_speed);
  app->camera->position = add_vec3(app->camera->position, scale_vec3(world_speed, dt));
  app->camera->look_at = add_vec3(app->camera->look_at, scale_vec3(world_speed, dt));
  if (!equals_vec3(app->cam_speed, vec3(0, 0, 0)))
    app->tela->iterations = 1;

  // Render
  RaytraceParams params = {
    .samples_per_pixel = 3,
    .bounces = 5,
    .variance = 0.001f,
    .gamma = 0.5f,
    .bilinear_texture = true,
    .is_biased = true,
    .camera = app->camera,
    .render_background = render_background_ambient,
    .render_background_context = NULL,
    .exposed_tela = app->tela,
    .directional_light = NULL,
  };

  ray_trace_scene_parallel(app->scene, &params);

  // Update window title with FPS
  set_window_title(app->window, format_string("FPS: %.2f", 1.0f / dt));

  // Present the frame
  paint_window(app->window, app->tela);
}

static void on_close(Window* window, void* ctx) {
  (void)window;
  Loop* animation = (Loop*)ctx;
  stop_loop(animation);
}

/* =============================================================================
 * Input Handlers
 * ========================================================================== */

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
}

static void on_mouse_move(Window* window, i32 x, i32 y, void* ctx) {
  (void)window;
  if (!g_mouse_down)
    return;

  Vec2 new_pos = vec2((f32)x, (f32)y);
  if (equals_vec2(new_pos, g_mouse_pos))
    return;

  App* app = (App*)ctx;
  Vec2 delta = sub_vec2(new_pos, g_mouse_pos);

  Vec3 orbit = get_camera_orbit(app->camera);
  f32 theta_delta = -2.0f * PI * (delta.x / WIDTH);
  f32 phi_delta = -2.0f * PI * (delta.y / HEIGHT);

  set_orbit_camera(app->camera, orbit.x, orbit.y + theta_delta, orbit.z + phi_delta);
  g_mouse_pos = new_pos;
  app->tela->iterations = 1;
}

static void on_mouse_scroll(Window* window, i32 delta_y, void* ctx) {
  (void)window;
  App* app = (App*)ctx;

  Vec3 orbit = get_camera_orbit(app->camera);
  f32 new_radius = orbit.x + delta_y * 0.1f;

  set_orbit_camera(app->camera, new_radius, orbit.y, orbit.z);
  app->tela->iterations = 1;
}

static void on_key_down(Window* window, u32 keycode, void* ctx) {
  (void)window;
  App* app = (App*)ctx;
  const f32 magnitude = 1.0f;
  if (keycode == SDLK_w) app->cam_speed = vec3(0, 0, magnitude);
  if (keycode == SDLK_s) app->cam_speed = vec3(0, 0, -magnitude);
  if (keycode == SDLK_a) app->cam_speed = vec3(-magnitude, 0, 0);
  if (keycode == SDLK_d) app->cam_speed = vec3(magnitude, 0, 0);
  app->tela->iterations = 1;
}

static void on_key_up(Window* window, u32 keycode, void* ctx) {
  (void)window;
  (void)keycode;
  App* app = (App*)ctx;
  app->cam_speed = vec3(0, 0, 0);
}

static void register_input_handlers(Window* window, App* app) {
  on_mouse_down_window(window, on_mouse_down, app);
  on_mouse_up_window(window, on_mouse_up, app);
  on_mouse_move_window(window, on_mouse_move, app);
  on_mouse_scroll_window(window, on_mouse_scroll, app);
  on_key_down_window(window, on_key_down, app);
  on_key_up_window(window, on_key_up, app);
}

/* =============================================================================
 * Main
 * ========================================================================== */

int main(void) {
  Vec3 cam_pos, cam_lookat;
  f32 cam_fov;
  Scene scene = load_scene_file(SCENE_FILE, &cam_pos, &cam_lookat, &cam_fov);
  cam_pos = to_engine_coords(cam_pos);
  cam_lookat = to_engine_coords(cam_lookat);

  Tela* tela = new_tela(WIDTH, HEIGHT);
  Window* window = new_window(WIDTH * 2, HEIGHT * 2, "Staircase");

  // fov is treated as the vertical field of view
  f32 distance_to_plane = 0.5f / tanf((cam_fov * PI / 180.0f) * 0.5f);
  Camera camera = create_camera(cam_pos, cam_lookat, distance_to_plane);

  // create_camera doesn't orient the basis towards look_at, only set_orbit_camera does
  Vec3 diff = sub_vec3(cam_pos, cam_lookat);
  f32 radius = length_vec3(diff);
  f32 phi = radius > 1e-6f ? asinf(clamp(diff.z / radius, -1.0f, 1.0f)) : 0.0f;
  f32 theta = atan2f(diff.y, diff.x);
  set_orbit_camera(&camera, radius, theta, phi);

  App app = {
    .tela = tela,
    .window = window,
    .camera = &camera,
    .scene = &scene,
    .cam_speed = vec3(0, 0, 0),
  };

  Loop* animation = loop(on_frame, &app);

  on_close_window(window, on_close, animation);
  register_input_handlers(window, &app);

  play_loop(animation);

  return 0;
}
