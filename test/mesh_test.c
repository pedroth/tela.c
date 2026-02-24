/**
 * SDF Test Window
 *
 * A raymarching demo that renders a morphing torus-cube using signed distance
 * functions. Features interactive orbit camera controls via mouse.
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
 * Global State (for input handling)
 * ========================================================================== */

static bool g_mouse_down = false;
static Vec2 g_mouse_pos = { 0 };

/* =============================================================================
 * Animation Loop
 * ========================================================================== */

static void on_frame(f32 dt, f32 time, void* ctx) {
    App* app = (App*)ctx;

    Scene* scene = app->scene;

    // Render
    raster_scene(
        scene,
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

    // Update window title with FPS
    set_window_title(app->window, format_string("FPS: %.2f", 1.0f / dt));

    // Present the frame
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
 * Main
 * ========================================================================== */
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
    return vec3(-v.y, v.x, v.z);
}

Vec3 third_transform(Vec3 v, void* ctx) {
    return vec3(v.z, v.y, -v.x);
}


int main(void) {
    // Create window and canvas
    Tela* tela = new_tela(WIDTH, HEIGHT);
    Window* window = new_window(WIDTH, HEIGHT, "Mesh Test");

    // Setup camera
    Camera camera = create_camera(vec3(3.0f, 0.0f, 0.0f), vec3(0, 0, 0), 1.0f);

    char* obj_files[] = {
        "./assets/megaman.obj",
        "./assets/statue.obj",
        "./assets/JesusMary.obj",
        "./assets/spot.obj",
        "./assets/riku.obj",
        "./assets/burger.obj",
        "./assets/little_tokyo.obj",
    };
    char* texture_files[] = {
        "./assets/megaman.png",
        "./assets/statue.jpg",
        "./assets/JesusMary.jpg",
        "./assets/spot.png",
        "./assets/riku.png",
        "./assets/burger.jpg",
        "./assets/little_tokyo.jpg"
    };

    u32 mesh_index = 6;
    String obj = io_read_file(obj_files[mesh_index]);
    Mesh mesh = read_obj_mesh(obj, "mesh");
    AABB box = get_bounding_box_mesh(&mesh);
    f32 scale_inv = 2.0f / max_comp_vec3(box.diagonal);
    map_vertices_mesh(&mesh, first_transform, &(FirstTransformContext){ .center = box.center, .scale_inv = scale_inv });
    map_vertices_mesh(&mesh, second_transform, NULL);
    map_vertices_mesh(&mesh, third_transform, NULL);
    add_texture_mesh(&mesh, io_read_image(texture_files[mesh_index]));
    
    Scene scene = new_naive_scene();
    add_triangles_scene(&scene, get_triangles_mesh(&mesh));
    // Application state
    App app = {
      .tela = tela,
      .window = window,
      .camera = &camera,
      .scene = &scene
    };

    // Animation loop
    Loop* animation = loop(on_frame, &app);

    // Event handlers
    on_close_window(window, on_close, animation);
    register_input_handlers(window, &app);

    // Run
    play_loop(animation);

    return 0;
}
