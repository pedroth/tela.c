/**
 * Triangle Test Window
 *
 * A simple triangle rasterization demo with interactive orbit camera controls.
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

    NaiveScene* scene = app->scene;

    // Render
    raster_scene(
        scene,
        (RasterParams) {
        .camera = app->camera,
            .tela = app->tela,
            .cull_backfaces = false,
            .bilinear_texture = false,
            .clip_camera_plane = true,
            .clear_screen = true,
            .background_color = (Color){ 0.1f, 0.1f, 0.1f, 1.0f },
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
    f32 new_radius = orbit.x + delta_y * 0.001f;

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

int main(void) {
    // Create window and canvas
    Tela* tela = new_tela(WIDTH, HEIGHT);
    Window* window = new_window(WIDTH, HEIGHT, "Triangle Test");

    // Setup camera
    Camera camera = create_camera(vec3(3.0f, 0.0f, 0.0f), vec3(0, 0, 0), 1.0f);

    static RasterTriangleProps tri_props = {
        .colors = { { 1, 0, 0, 1 }, { 0, 1, 0, 1 }, { 0, 0, 1, 1 } },
        .tex_coords = { { 0, 0 }, { 1, 0 }, { 0, 1 } },
        .texture = NULL
    };

    NaiveScene scene = { 0 };
    add_triangle_nscene(
        &scene,
        (Triangle) {
        .positions = { vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1) },
            .props = &tri_props
        }
    );

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
