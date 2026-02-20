/**
 * Point Cloud
 *
 * Renders a mesh as a point cloud (spheres at each vertex).
 * Features interactive orbit camera controls via mouse.
 */

#include "../src/index.c"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

 /* =============================================================================
  * Constants
  * ========================================================================== */

static const u32 WIDTH = 640;
static const u32 HEIGHT = 480;
static const f32 SPHERE_RADIUS = 0.02f;

/* =============================================================================
 * Types
 * ========================================================================== */

typedef struct {
    Tela* tela;
    Window* window;
    Camera* camera;
    NaiveScene* scene;
} App;

/* =============================================================================
 * Global State (for input handling)
 * ========================================================================== */

static bool g_mouse_down = false;
static Vec2 g_mouse_pos = { 0 };

/* =============================================================================
 * Animation Loop
 * ========================================================================== */

static void on_frame(f32 dt, f32 time, void* ctx) {
    App* app = (App*)ctx;

    raster_scene(
        app->scene,
        (RasterParams) {
        .camera = app->camera,
            .tela = app->tela,
            .cull_backfaces = true,
            .bilinear_texture = false,
            .clip_camera_plane = true,
            .clear_screen = true,
            .background_color = (Color){ 0.0f, 0.0f, 0.0f, 1.0f },
            .perspective_correct = true
    });

    set_window_title(app->window, format_string("FPS: %.2f", 1.0f / dt));
    paint_window(app->window, app->tela);
}

static void on_close(Window* window, void* ctx) {
    Loop* animation = (Loop*)ctx;
    stop_loop(animation);
}

/* =============================================================================
 * Input Handlers
 * ========================================================================== */

static void on_mouse_down(Window* window, i32 x, i32 y, u32 button, void* ctx) {
    g_mouse_down = true;
    g_mouse_pos = vec2((f32)x, (f32)y);
}

static void on_mouse_up(Window* window, i32 x, i32 y, u32 button, void* ctx) {
    g_mouse_down = false;
}

static void on_mouse_move(Window* window, i32 x, i32 y, void* ctx) {
    if (!g_mouse_down)
        return;

    Vec2 new_pos = vec2((f32)x, (f32)y);
    if (equals_vec2(new_pos, g_mouse_pos))
        return;

    App* app = (App*)ctx;
    Vec2 delta = sub_vec2(new_pos, g_mouse_pos);

    Vec3 orbit = get_camera_orbit(app->camera);
    f32 theta_delta = -2.0f * M_PI * (delta.x / WIDTH);
    f32 phi_delta = -2.0f * M_PI * (delta.y / HEIGHT);

    set_orbit_camera(
        app->camera, orbit.x, orbit.y + theta_delta, orbit.z + phi_delta
    );

    g_mouse_pos = new_pos;
}

static void on_mouse_scroll(Window* window, i32 delta_y, void* ctx) {
    App* app = (App*)ctx;

    Vec3 orbit = get_camera_orbit(app->camera);
    f32 new_radius = orbit.x + delta_y * 0.1f;

    set_orbit_camera(app->camera, new_radius, orbit.y, orbit.z);
}

static void register_input_handlers(Window* window, App* app) {
    on_mouse_down_window(window, on_mouse_down, app);
    on_mouse_up_window(window, on_mouse_up, app);
    on_mouse_move_window(window, on_mouse_move, app);
    on_mouse_scroll_window(window, on_mouse_scroll, app);
}

/* =============================================================================
 * Mesh Transforms
 * ========================================================================== */

typedef struct {
    Vec3 center;
    f32 scale_inv;
} NormalizeContext;

Vec3 normalize_transform(Vec3 v, void* ctx) {
    NormalizeContext* c = (NormalizeContext*)ctx;
    return scale_vec3(sub_vec3(v, c->center), c->scale_inv);
}

Vec3 rotate_xy(Vec3 v, void* ctx) {
    return vec3(-v.y, v.x, v.z);
}

Vec3 rotate_xz(Vec3 v, void* ctx) {
    return vec3(v.z, v.y, -v.x);
}

f32 color_map_func(f32 x) {
    return clamp(0.5f * (x + 1.0f), 0.0f, 1.0f);
}

Color position_to_color(Vec3 v, void* ctx) {
    v = map_vec3(v, color_map_func);
    return (Color) {
        v.x,
            v.y,
            v.z,
            1.0f
    };
}

/* =============================================================================
 * Main
 * ========================================================================== */

int main(void) {
    Tela* tela = new_tela(WIDTH, HEIGHT);
    Window* window = new_window(WIDTH, HEIGHT, "Point Cloud");

    Camera camera = create_camera(vec3(5.0f, 0.0f, 0.0f), vec3(0, 0, 0), 1.0f);

    // Load mesh
    String obj = io_read_file("./assets/statue.obj");
    Mesh mesh = read_obj_mesh(obj, "bunny");

    // Normalize to [-1, 1]
    AABB box = get_bounding_box_mesh(&mesh);
    f32 scale_inv = 2.0f / max_comp_vec3(box.diagonal);
    map_vertices_mesh(&mesh, normalize_transform, &(NormalizeContext){.center = box.center, .scale_inv = scale_inv });
    map_vertices_mesh(&mesh, rotate_xy, NULL);
    map_vertices_mesh(&mesh, rotate_xz, NULL);

    // Map vertex positions to colors: color = 0.5 * (position + 1)
    map_colors_mesh(&mesh, position_to_color, NULL);

    // Build scene from spheres
    NaiveScene scene = { 0 };
    add_spheres_nscene(&scene, get_spheres_mesh(&mesh, SPHERE_RADIUS));

    App app = {
        .tela = tela,
        .window = window,
        .camera = &camera,
        .scene = &scene
    };

    Loop* animation = loop(on_frame, &app);

    on_close_window(window, on_close, animation);
    register_input_handlers(window, &app);

    play_loop(animation);

    return 0;
}
