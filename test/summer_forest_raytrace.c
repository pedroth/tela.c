/**
 * Summer Forest
 *
 * A rasterized 3D mesh viewer for the Summer Forest scene (from Spyro 2).
 * Features interactive orbit camera controls via mouse and WASD camera movement.
 */

#include "../src/index.c"


 /* =============================================================================
  * Constants
  * ========================================================================== */

static const u32 WIDTH = 640/2;
static const u32 HEIGHT = 480/2;

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
static Tela* g_background = NULL;
static DirectionalLightParams* g_directional_light = NULL;

/* =============================================================================
 * Animation Loop
 * ========================================================================== */

static inline Color render_background(Ray ray, void* ctx) {
    Tela* background = (Tela*)ctx;
    if (!background) {
        return (Color) { 0.0f, 0.0f, 0.0f, 1.0f };
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

    // Update camera position from WASD movement
    Vec3 world_speed = to_world_coord_camera(app->camera, app->cam_speed);
    app->camera->position = add_vec3(app->camera->position, scale_vec3(world_speed, dt));
    app->camera->look_at = add_vec3(app->camera->look_at, scale_vec3(world_speed, dt));

    // Render
    RaytraceParams params = {
     .samples_per_pixel = 3,
     .bounces = 5,
     .variance = 0.001f,
     .gamma = 0.5f,
     .bilinear_texture = true,
     .is_biased = false,
     .camera = app->camera,
     .render_background = render_background,
     .render_background_context = g_background,
     .exposed_tela = app->tela,
     .directional_light = g_directional_light,
    };

    ray_trace_scene_parallel(app->scene, &params);

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
    f32 theta_delta = -2.0f * PI * (delta.x / WIDTH);
    f32 phi_delta = -2.0f * PI * (delta.y / HEIGHT);

    set_orbit_camera(
        app->camera, orbit.x, orbit.y + theta_delta, orbit.z + phi_delta
    );
    g_mouse_pos = new_pos;
    Tela* exposed = app->tela;
    exposed->iterations = 1;
}

static void on_mouse_scroll(Window* window, i32 delta_y, void* ctx) {
    App* app = (App*)ctx;

    Vec3 orbit = get_camera_orbit(app->camera);
    f32 new_radius = orbit.x + delta_y * 0.1f;

    set_orbit_camera(app->camera, new_radius, orbit.y, orbit.z);
    Tela* exposed = app->tela;
    exposed->iterations = 1;
}

static void on_key_down(Window* window, u32 keycode, void* ctx) {
    App* app = (App*)ctx;
    const f32 magnitude = 500.0f;
    if (keycode == SDLK_w) app->cam_speed = vec3(0, 0, magnitude);
    if (keycode == SDLK_s) app->cam_speed = vec3(0, 0, -magnitude);
    Tela* exposed = app->tela;
    exposed->iterations = 1;
}

static void on_key_up(Window* window, u32 keycode, void* ctx) {
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
 * Material
 * ========================================================================== */

Material diffuse_material_mapper(Face face, void* ctx) {
    return build_diffuse_material();
}

/* =============================================================================
 * Main
 * ========================================================================== */

int main(void) {
    g_background = io_read_image("assets/sky.jpg");
    Vec3 light_dir = vec3(1.0f, 1.0f, 1.0f);
    normalize_vec3(light_dir, &light_dir);
    g_directional_light = &(DirectionalLightParams) {
        .direction = light_dir,
        .sharpness = 200.0f
    };

    // Create canvas and window (canvas is half-size, window is full-size)
    Tela* tela = new_tela(WIDTH, HEIGHT);
    Window* window = new_window(WIDTH * 2, HEIGHT * 2, "Summer Forest");

    // Setup camera looking at scene center, orbiting at radius 3
    Vec3 look_at = vec3(8496.0f, 1431.0f, 2429.0f);
    Camera camera = create_camera(vec3(0, 0, 0), look_at, 1.0f);
    set_orbit_camera(&camera, 3.0f, 0.0f, 0.0f);

    // Load mesh (no normalization - use world coordinates as-is)
    String obj = io_read_file("./assets/summer_forest.obj");
    Mesh mesh = read_obj_mesh(obj, "summer_forest");

    // Map all vertex colors to dark gray (0.25, 0.25, 0.25)
    mesh.colors = new_array(mesh.vertices.length, sizeof(Color));
    for (u32 i = 0; i < mesh.vertices.length; i++) {
        Color c = { 0.25f, 0.25f, 0.25f, 1.0f };
        push_array(&mesh.colors, &c);
    }

    // Add texture
    add_texture_mesh(&mesh, io_read_image("./assets/summer_forest.png"));

    // Assign diffuse material to all triangles (required by ray tracer)
    map_triangles_materials_mesh(&mesh, diffuse_material_mapper, NULL);

    // Build scene
    Scene scene = new_kscene(20);
    Array mesh_elems = triangles_to_scene_elems(get_triangles_mesh(&mesh));
    add_scene_elems_scene(&scene, mesh_elems);
    free_array(&mesh_elems);
    scene.vtable->rebuild_scene(&scene); // force build before rendering
    // Application state
    App app = {
        .tela = tela,
        .window = window,
        .camera = &camera,
        .scene = &scene,
        .cam_speed = vec3(0, 0, 0)
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
