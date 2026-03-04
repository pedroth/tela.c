#include "../src/index.c"

static const u32 WIDTH = 640;
static const u32 HEIGHT = 480;

static const u32 GRID_N = 10;
static const u32 MAX_ITE = 100;
static const f32 RAY_EPSILON = 1e-3f;

typedef struct {
    Tela* tela;
    Window* window;
    Scene scene;
    Camera camera;
} App;

static bool g_mouse_down = false;
static Vec2 g_mouse = { 0 };

static inline Color color_rgb(f32 r, f32 g, f32 b) {
    return (Color) { r, g, b, 1.0f };
}

static SceneElem make_line(Vec3 init, Vec3 end, Color color) {
    Line line = build_line(init, end);
    line.radius = 0.01f;

    LineProps* props = (LineProps*)malloc(sizeof(LineProps));
    if (props) {
        props->colors[0] = color;
        props->colors[1] = color;
    }
    line.props = props;

    return build_scene_elem_line(line);
}

static SceneElem make_sphere(Vec3 p, f32 radius, Color color) {
    Sphere sphere = build_sphere(p, radius);
    RasterSphereProps* props = (RasterSphereProps*)malloc(sizeof(RasterSphereProps));
    if (props) {
        *props = (RasterSphereProps){
            .color = color,
            .tex_coord = vec2(0, 0),
            .texture = NULL,
            .material = NULL,
        };
    }
    sphere.props = props;
    return build_scene_elem_sphere(sphere);
}

static void add_wire_sphere(Scene* scene, Vec3 p, f32 radius, Color color) {
    const u32 samples = 20;
    const f32 tau = 2.0f * PI;

    for (u32 i = 0; i < samples; i++) {
        f32 t0 = (f32)i / (f32)(samples - 1);
        f32 t1 = (f32)(i + 1) / (f32)(samples - 1);

        Vec3 a_long = add_vec3(p, vec3(0.0f, radius * cosf(tau * t0), radius * sinf(tau * t0)));
        Vec3 b_long = add_vec3(p, vec3(0.0f, radius * cosf(tau * t1), radius * sinf(tau * t1)));
        add_scene_elem_scene(scene, make_line(a_long, b_long, color));

        Vec3 a_lat = add_vec3(p, vec3(radius * cosf(tau * t0), radius * sinf(tau * t0), 0.0f));
        Vec3 b_lat = add_vec3(p, vec3(radius * cosf(tau * t1), radius * sinf(tau * t1), 0.0f));
        add_scene_elem_scene(scene, make_line(a_lat, b_lat, color));
    }
}

static void free_scene_props(Scene* scene) {
    Array* elems = get_scene_elems_scene(scene);
    for (u32 i = 0; i < elems->length; i++) {
        SceneElem* elem = (SceneElem*)get_array_element(elems, i);
        if (elem->geometry_type == LINE_GEOMETRY) {
            Line* line = &elem->as.line;
            if (line->props) {
                free(line->props);
                line->props = NULL;
            }
        }
        else if (elem->geometry_type == SPHERE) {
            Sphere* sphere = &elem->as.sphere;
            if (sphere->props) {
                free(sphere->props);
                sphere->props = NULL;
            }
        }
    }
}

static void draw_debug_ray(App* app, Ray ray) {
    Scene debug_scene = new_naive_scene();

    Vec3 p = ray.init;
    f32 t = distance_to_point_scene(&app->scene, p, smooth_min_scene_default);
    f32 max_dist = t;

    add_scene_elem_scene(&debug_scene, make_sphere(p, 0.05f, COLOR_BLUE));
    add_wire_sphere(&debug_scene, p, t, COLOR_GREEN);
    add_scene_elem_scene(&debug_scene, make_line(ray.init, trace_ray(ray, t), COLOR_RED));

    for (u32 i = 0; i < MAX_ITE; i++) {
        add_scene_elem_scene(&debug_scene,
            make_sphere(p, 0.05f, color_rgb(0.0f, 0.0f, clamp((f32)i / 5.0f, 0.0f, 1.0f))));

        p = trace_ray(ray, t);
        f32 d = distance_to_point_scene(&app->scene, p, smooth_min_scene_default);
        add_wire_sphere(&debug_scene, p, d,
            color_rgb(0.0f, clamp(1.0f - (f32)i / 10.0f, 0.0f, 1.0f), 0.0f));

        t += d;

        if (d < RAY_EPSILON) {
            add_scene_elem_scene(&debug_scene, make_sphere(p, 0.05f, COLOR_BLUE));
            break;
        }
        if (d > max_dist) {
            break;
        }
    }

    raster_scene(&debug_scene, (RasterParams) {
        .clear_screen = false,
            .camera = &app->camera,
            .tela = app->tela,
    });

    free_scene_props(&debug_scene);
    free_scene(&debug_scene);
}

static void free_scene_sphere_props(Scene* scene) {
    Array* elems = get_scene_elems_scene(scene);
    for (u32 i = 0; i < elems->length; i++) {
        SceneElem* elem = (SceneElem*)get_array_element(elems, i);
        if (elem->geometry_type != SPHERE) {
            continue;
        }
        Sphere* sphere = &elem->as.sphere;
        if (sphere->props) {
            free(sphere->props);
            sphere->props = NULL;
        }
    }
}

static void on_frame(f32 dt, f32 time, void* ctx) {
    App* app = (App*)ctx;

    raster_scene(&app->scene, (RasterParams) {
        .clear_screen = true,
            .camera = &app->camera,
            .tela = app->tela,
    });
    debug_scene(&app->scene, &(SceneDebugProps) {.camera = &app->camera, .tela = app->tela });

    const f32 freq = 0.05f;
    f32 t = fmodf(time, 10.0f);
    Vec3 debug_dir = vec3(0.0f, cosf(freq * t), -sinf(freq * t));
    draw_debug_ray(app, build_ray(vec3(0.0f, -5.0f, 2.0f), debug_dir));

    paint_window(app->window, app->tela);
    set_window_title(app->window,
        format_string("Debug Ray Marching | FPS: %.1f", dt > 0.0f ? (1.0f / dt) : 0.0f));
}

static void on_close(Window* window, void* ctx) {
    (void)window;
    Loop* animation = (Loop*)ctx;
    stop_loop(animation);
}

static void on_mouse_down(Window* window, i32 x, i32 y, u32 button, void* ctx) {
    (void)window;
    (void)button;
    (void)ctx;
    g_mouse_down = true;
    g_mouse = vec2((f32)x, (f32)y);
}

static void on_mouse_up(Window* window, i32 x, i32 y, u32 button, void* ctx) {
    (void)window;
    (void)x;
    (void)y;
    (void)button;
    (void)ctx;
    g_mouse_down = false;
    g_mouse = vec2(0, 0);
}

static void on_mouse_move(Window* window, i32 x, i32 y, void* ctx) {
    (void)window;
    App* app = (App*)ctx;

    Vec2 new_mouse = vec2((f32)x, (f32)y);
    if (!g_mouse_down || equals_vec2(new_mouse, g_mouse)) {
        return;
    }

    Vec2 delta = sub_vec2(new_mouse, g_mouse);
    Vec3 orbit = get_camera_orbit(&app->camera);
    set_orbit_camera(
        &app->camera,
        orbit.x,
        orbit.y - 2.0f * PI * (delta.x / (f32)app->tela->width),
        orbit.z - 2.0f * PI * (delta.y / (f32)app->tela->height)
    );

    g_mouse = new_mouse;
}

static void on_mouse_scroll(Window* window, i32 delta_y, void* ctx) {
    (void)window;
    App* app = (App*)ctx;

    Vec3 orbit = get_camera_orbit(&app->camera);
    set_orbit_camera(&app->camera, orbit.x + (f32)delta_y * 0.1f, orbit.y, orbit.z);
}


static void register_input_handlers(Window* window, App* app) {
    on_mouse_down_window(window, on_mouse_down, app);
    on_mouse_up_window(window, on_mouse_up, app);
    on_mouse_move_window(window, on_mouse_move, app);
    on_mouse_scroll_window(window, on_mouse_scroll, app);
}

int main(void) {
    Tela* tela = new_tela(WIDTH, HEIGHT);
    Window* window = new_window(WIDTH, HEIGHT, "Debug Ray Marching");

    Scene scene = new_kscene(30);
    for (u32 k = 0; k < GRID_N * GRID_N; k++) {
        u32 i = k / GRID_N;
        u32 j = k % GRID_N;

        Vec3 initial = vec3(0.0f, (f32)j / (f32)GRID_N, (f32)i / (f32)GRID_N);
        Vec3 random_offset = random_vec3();
        Vec3 p = add_vec3(initial, random_offset);
        p = vec3(2.0f * p.x - 1.0f, 2.0f * p.y - 1.0f, 2.0f * p.z - 1.0f);

        Sphere sphere = build_sphere(p, 1e-2f);
        RasterSphereProps* props = (RasterSphereProps*)malloc(sizeof(RasterSphereProps));
        if (props) {
            *props = (RasterSphereProps){
                .color = color_rgb(0.5f, 0.5f, 0.5f),
                .tex_coord = vec2(0, 0),
                .texture = NULL,
                .material = NULL,
            };
        }
        sphere.props = props;
        add_scene_elem_scene(&scene, build_scene_elem_sphere(sphere));
    }
    scene.vtable->rebuild_scene(&scene);

    Camera camera = create_camera(vec3(5.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 1.0f);

    App app = {
        .tela = tela,
        .window = window,
        .scene = scene,
        .camera = camera,
    };

    Loop* animation = loop(on_frame, &app);
    on_close_window(window, on_close, animation);
    register_input_handlers(window, &app);

    play_loop(animation);

    free_scene_sphere_props(&app.scene);
    free_scene(&app.scene);

    return 0;
}
