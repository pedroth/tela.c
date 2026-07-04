/**
 * SVG Viewer
 *
 * Loads an SVG file, triangulates its paths (with hole merging), and renders
 * them with a random colour per triangle - matching tela.js svg_2.js.
 *
 * Usage:  ./app [path/to/file.svg]
 * Compile: gcc -O3 -fopenmp -o app test/svg.c -lSDL2 -lm
 */

#include "../src/index.c"

// gcc -O3 -fopenmp -o app test/svg.c -lSDL2 -lm

/* =============================================================================
 * Constants
 * ========================================================================== */

static const u32 WIDTH  = 640;
static const u32 HEIGHT = 480;

/* =============================================================================
 * App
 * ========================================================================== */

typedef struct {
    Tela*     tela;
    Window*   window;
    Camera_2D camera;
    SVGData   svg;
    Array     tris;  /* Array of SVGTri — computed once, drawn every frame */
} App;

static bool g_mouse_down = false;
static Vec2 g_mouse_pos  = {0};

/* =============================================================================
 * Animation Loop
 * ========================================================================== */

static void on_frame(f32 dt, f32 time, void* ctx) {
    (void)time;
    App* app = (App*)ctx;

    fill_tela(app->tela, (Color){0, 0, 0, 1});

    /* Fill: one random colour per triangle, with hole merging */
    draw_svg_tris(app->tela, &app->camera, &app->tris);

    /* Outline: orange -> red gradient per segment (matches svg_2.js) */
    for (u32 i = 0; i < app->svg.paths.length; i++) {
        Array* path = *(Array**)get_array_element(&app->svg.paths, i);
        draw_svg_path_lines_gradient(app->tela, &app->camera, path);
    }

    set_window_title(app->window,
        format_string("SVG Viewer | FPS: %.1f", 1.0f / dt));
    paint_window(app->window, app->tela);
}

static void on_close(Window* w, void* ctx) {
    (void)w; stop_loop((Loop*)ctx);
}

/* =============================================================================
 * Input Handlers
 * ========================================================================== */

static void on_mouse_down(Window* w, i32 x, i32 y, u32 btn, void* ctx) {
    (void)w; (void)btn; (void)ctx;
    g_mouse_down = true;
    g_mouse_pos  = vec2((f32)x, (f32)y);
}

static void on_mouse_up(Window* w, i32 x, i32 y, u32 btn, void* ctx) {
    (void)w; (void)x; (void)y; (void)btn; (void)ctx;
    g_mouse_down = false;
}

static void on_mouse_move(Window* w, i32 x, i32 y, void* ctx) {
    (void)w;
    if (!g_mouse_down) return;
    App*  app     = (App*)ctx;
    Vec2  new_pos = vec2((f32)x, (f32)y);
    if (equals_vec2(new_pos, g_mouse_pos)) return;

    Vec2 delta   = sub_vec2(new_pos, g_mouse_pos);
    Vec2 screen  = vec2((f32)WIDTH, (f32)HEIGHT);
    Vec2 diag    = app->camera.view_box.diagonal;
    Vec2 world_d = mul_vec2(scale_vec2(delta, -1), div_vec2(diag, screen));

    app->camera = build_camera_2d(
        add_vec2(app->camera.view_box.min, world_d),
        add_vec2(app->camera.view_box.max, world_d)
    );
    g_mouse_pos = new_pos;
}

static void on_mouse_scroll(Window* w, i32 delta_y, void* ctx) {
    (void)w;
    App*  app    = (App*)ctx;
    f32   scale  = 1.0f + (f32)delta_y * 0.1f;
    Vec2  center = app->camera.view_box.center;
    Vec2  half   = scale_vec2(app->camera.view_box.diagonal, scale * 0.5f);
    app->camera  = build_camera_2d(sub_vec2(center, half), add_vec2(center, half));
}

/* =============================================================================
 * Main
 * ========================================================================== */

int main(int argc, char** argv) {
    const char* svg_path = argc > 1 ? argv[1] : "assets/euler.svg";

    String file = io_read_file(svg_path);
    if (!file.data) {
        fprintf(stderr, "Failed to open '%s'\n", svg_path);
        return 1;
    }

    SVGData svg = parse_svg(file.data, file.length);
    free(file.data);

    if (svg.paths.length == 0) {
        fprintf(stderr, "No paths found in '%s'\n", svg_path);
        return 1;
    }

    /* Pre-compute triangulation with hole merging + random per-triangle colour */
    Array tris = svg_triangulate_paths(&svg.paths);

    Tela*     tela   = new_tela(WIDTH, HEIGHT);
    Window*   window = new_window(WIDTH, HEIGHT, "SVG Viewer");
    Camera_2D camera = build_camera_2d(vec2(0, 0), vec2(1, 1));

    App app = {
        .tela   = tela,
        .window = window,
        .camera = camera,
        .svg    = svg,
        .tris   = tris,
    };

    Loop* anim = loop(on_frame, &app);
    on_close_window(window,        on_close,        anim);
    on_mouse_down_window(window,   on_mouse_down,   &app);
    on_mouse_up_window(window,     on_mouse_up,     &app);
    on_mouse_move_window(window,   on_mouse_move,   &app);
    on_mouse_scroll_window(window, on_mouse_scroll, &app);
    play_loop(anim);

    free_svg(&svg);
    free_array(&tris);
    return 0;
}
