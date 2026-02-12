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

static const u32 MAX_RAYMARCH_ITERATIONS = 100;
static const f32 MAX_RAYMARCH_DISTANCE = 10.0f;
static const f32 RAYMARCH_EPSILON = 1e-3f;
static const f32 NORMAL_EPSILON = 1e-3f;

/* =============================================================================
 * Types
 * ========================================================================== */

typedef struct {
    Tela* tela;
    Window* window;
    Camera* camera;
    NaiveScene* scene;
} App;

typedef struct {
    f32 time;
    Vec3 light_pos;
} SceneContext;

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
    Tela* tela = raster_scene(
        scene,
        (RasterParams) {
        .camera = app->camera,
            .tela = app->tela,
            .cull_backfaces = true,
            .bilinear_texture = false,
            .clip_camera_plane = true,
            .clear_screen = true,
            .background_color = (Color){ 0.1f, 0.1f, 0.1f, 1.0f },
            .perspective_correct = true
    });
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

typedef struct {
    Color colors[3];
} TriangleProps;

int main(void) {
    // Create window and canvas
    Tela* tela = new_tela(WIDTH, HEIGHT);
    Window* window = new_window(WIDTH, HEIGHT, "Triangle Test");

    // Setup camera
    Camera camera = create_camera(vec3(3.0f, 0.0f, 0.0f), vec3(0, 0, 0), 1.0f);

    NaiveScene scene = { 0 };
    add_triangle_nscene(
        &scene,
        (Triangle) {
        .positions = { vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1) },
            .props = &(TriangleProps) {
            .colors = { { 1, 0, 0, 1 }, { 0, 1, 0, 1 }, { 0, 0, 1, 1 } }
        }
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
