/**
 * Glass Cornell Box
 *
 * A path-traced Cornell box scene with switchable bunny and crystal glass meshes.
 * Features interactive orbit camera controls via mouse and mesh selection via
 * the left/right arrow keys.
 *
 * Compile with:
 * gcc -O3 -fopenmp -o glass test/glass.c -lSDL2 -lm
 *
 * Run with:
 * ./glass
 */

#include "../src/index.c"

 /* =============================================================================
  * Constants
  * ========================================================================== */

static const u32 WIDTH = 640 / 2;
static const u32 HEIGHT = 480 / 2;

typedef struct {
    Vec3 center;
    f32 scale_inv;
} FirstTransformContext;

typedef Vec3 (*VertexTransform)(Vec3, void*);

static Vec3 first_transform(Vec3 v, void* ctx);
static Vec3 second_transform(Vec3 v, void* ctx);
static Vec3 third_transform(Vec3 v, void* ctx);
static Vec3 fourth_transform(Vec3 v, void* ctx);

static const VertexTransform BUNNY_TRANSFORMS[] = {
    first_transform, second_transform, third_transform, fourth_transform,
};

static const VertexTransform CRYSTAL_GLASS_TRANSFORMS[] = {
    first_transform, fourth_transform,
};

typedef struct {
    const char* name;
    const char* mesh_path;
    const VertexTransform* transforms;
    u32 transform_count;
} MeshEntry;

static const MeshEntry MESH_TABLE[] = {
    {
        "Bunny", "./assets/bunny_orig.obj", BUNNY_TRANSFORMS,
        sizeof(BUNNY_TRANSFORMS) / sizeof(BUNNY_TRANSFORMS[0]),
    },
    {
        "Crystal Glass", "./assets/crystal_glass.obj",
        CRYSTAL_GLASS_TRANSFORMS,
        sizeof(CRYSTAL_GLASS_TRANSFORMS) /
            sizeof(CRYSTAL_GLASS_TRANSFORMS[0]),
    },
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

/* =============================================================================
 * Global State
 * ========================================================================== */

static bool g_mouse_down = false;
static Vec2 g_mouse_pos = { 0 };

static void build_scene(Scene* scene, i32 mesh_index);

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

    if (app->pending_mesh != app->current_mesh) {
        clear_scene_elems_scene(scene);
        build_scene(scene, app->pending_mesh);
        scene->vtable->rebuild_scene(scene);
        app->current_mesh = app->pending_mesh;
        exposed->iterations = 1;
    }

    RaytraceParams params = {
      .samples_per_pixel = 3,
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

    set_window_title(app->window,
        format_string(
            "Glass Cornell Box | %s | Left/Right: change meshes | FPS: %.2f",
            MESH_TABLE[app->current_mesh].name, 1.0f / dt
        ));
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
    (void)w;
    App* app = (App*)ctx;

    if (keycode == SDLK_RIGHT) {
        app->pending_mesh = (app->pending_mesh + 1) % MESH_COUNT;
    } else if (keycode == SDLK_LEFT) {
        app->pending_mesh =
            (app->pending_mesh - 1 + MESH_COUNT) % MESH_COUNT;
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

Vec3 first_transform(Vec3 v, void* ctx) {
    Vec3 center = ((FirstTransformContext*)ctx)->center;
    f32 scale_inv = ((FirstTransformContext*)ctx)->scale_inv;
    return scale_vec3(sub_vec3(v, center), scale_inv);
}

Vec3 second_transform(Vec3 v, void* ctx) {
    return vec3(-v.y, v.x, v.z);
}

Vec3 third_transform(Vec3 v, void* ctx) {
    return vec3(v.z, v.y, -v.x);
}

Vec3 fourth_transform(Vec3 v, void* ctx) {
    return add_vec3(v, vec3(1.0f, 1.5f, 1.5f));
}

Color white_color_mapper(Vec3 v, void* ctx) {
    return COLOR_WHITE;
}

Material glass_material_mapper(Face face, void* ctx) {
    static Material mat = { 0 };
    if (mat.data == NULL) {
        mat = build_dielectric_material(3);
    }
    return mat;
}

static void build_scene(Scene* scene, i32 mesh_index) {
    // Materials
    Material* mat_diffuse = (Material*)malloc(sizeof(Material));
    *mat_diffuse = build_diffuse_material();

    Material* mat_emissive = (Material*)malloc(sizeof(Material));
    *mat_emissive = build_emissive_material();

    const MeshEntry* mesh_entry = &MESH_TABLE[mesh_index];
    String obj = io_read_file(mesh_entry->mesh_path);
    Mesh mesh = read_obj_mesh(obj, "glass");

    AABB box = get_bounding_box_mesh(&mesh);
    f32 max_diag_inv = 2.0f / max_comp_vec3(box.diagonal);
    FirstTransformContext normalize_context = {
        .center = box.center,
        .scale_inv = max_diag_inv,
    };

    for (u32 i = 0; i < mesh_entry->transform_count; i++) {
        void* context = i == 0 ? &normalize_context : NULL;
        map_vertices_mesh(&mesh, mesh_entry->transforms[i], context);
    }

    // Set colors and materials
    map_colors_mesh(&mesh, white_color_mapper, NULL);
    map_triangles_materials_mesh(&mesh, glass_material_mapper, NULL);

    // Add triangles to scene
    Array mesh_elems = triangles_to_scene_elems(get_triangles_mesh(&mesh));
    add_scene_elems_scene(scene, mesh_elems);
    free_array(&mesh_elems);

    // Build Cornell Box walls

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

  // alpha-tri (yellow emissive)
  Color yellow = COLOR_YELLOW;
  Triangle alpha_tri = build_triangle(vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1));
  alpha_tri.props = new_tri_props(yellow, mat_emissive);
    add_scene_elem_scene(scene, build_scene_elem_triangle(alpha_tri));

  // light-1
  Triangle light_1 = build_triangle(vec3(1, 1, 2.9f), vec3(2, 1, 2.9f), vec3(2, 2, 2.9f));
  light_1.props = new_tri_props(COLOR_WHITE, mat_emissive);
    add_scene_elem_scene(scene, build_scene_elem_triangle(light_1));

  // light-2
  Triangle light_2 = build_triangle(vec3(2, 2, 2.9f), vec3(1, 2, 2.9f), vec3(1, 1, 2.9f));
  light_2.props = new_tri_props(COLOR_WHITE, mat_emissive);
    add_scene_elem_scene(scene, build_scene_elem_triangle(light_2));
}

/* =============================================================================
 * Main
 * ========================================================================== */

int main(void) {
    // Create window and canvas
    Tela* tela = new_tela(WIDTH, HEIGHT);
    Window* window = new_window(WIDTH * 2, HEIGHT * 2, "Glass Cornell Box");

    // Setup camera looking at center of box (1.5, 1.5, 1.5)
    Camera camera = create_camera(vec3(3.0f, 0.0f, 0.0f), vec3(1.5f, 1.5f, 1.5f), 1.0f);
    set_orbit_camera(&camera, 3.0f, 0.0f, 0.0f);

    Scene scene = new_kscene(10);

    // Application state
    App app = {
      .tela = tela,
      .window = window,
      .camera = &camera,
      .scene = &scene,
      .current_mesh = 0,
      .pending_mesh = 0
    };

    build_scene(&scene, app.current_mesh);
    scene.vtable->rebuild_scene(&scene); // force build before rendering

    // Animation loop
    Loop* animation = loop(on_frame, &app);

    // Event handlers
    on_close_window(window, on_close, animation);
    register_input_handlers(window, &app);

    // Run
    play_loop(animation);

    // Cleanup
    free_scene(&scene);
    free_loop(animation);
    free_tela(tela);
    free_window(window);

    return 0;
}
