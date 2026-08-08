/**
 * stickman.c - Port of test/web/stickman.js from pedroth/tela.js (ai/sprite-testII)
 * 
 * gcc -O3 -march=native -ffast-math -fopenmp -o app test/stickman.c -lSDL2 -lm && ./app 
 *
 * Controls:
 *   WASD          - move
 *   Shift         - sprint
 *   Space         - jump
 *   Mouse drag    - orbit camera
 *   Mouse wheel   - zoom
 *   R             - toggle raster / raytrace
 */
#include "../src/index.c"

#define WIDTH  (640)
#define HEIGHT (480)

/* Sprite atlas frame pixel coordinates */
static const i32 FRAME_COLS[10][2] = {
    {82, 202}, {214, 333}, {350, 467}, {483, 598}, {614, 730},
    {749, 865}, {875, 999}, {1017, 1140}, {1156, 1272}, {1287, 1400},
};
static const i32 ANIM_ROWS[3][2] = {
    {72, 235}, {292, 469}, {524, 720},
};

#define GROUND_TILES  12
#define GROUND_MIN   -18.0f
#define GROUND_MAX    18.0f
#define GROUND_STEP   ((GROUND_MAX - GROUND_MIN) / (f32)GROUND_TILES)
/* stickman triangles are appended after all ground tiles */
#define STICKMAN_BASE (GROUND_TILES * GROUND_TILES * 2)

#define MOVE_SPEED   2.2f
#define SPRINT_SPEED 4.0f
#define JUMP_SPEED   6.2f
#define GRAVITY      16.0f
#define BODY_WIDTH   1.2f
#define BODY_HEIGHT  2.4f

typedef enum { ANIM_IDLE = 0, ANIM_MOVE = 1, ANIM_JUMP = 2 } AnimState;

typedef struct {
    Vec3      position;
    Vec3      velocity;
    bool      facing_right;
    bool      on_ground;
    AnimState anim_state;
    f32       anim_time;
    f32       idle_frame_time;
    f32       move_frame_time;
    f32       jump_frame_time;
    i32       last_frame;
} Stickman;

typedef struct {
    Tela*  sky;
    Scene* scene;
} SkyCtx;

typedef struct {
    Tela*   tela;
    Tela*   exposed_tela;
    Window* window;
    Camera  camera;
    Scene   scene;
    Tela*   atlas;
    Tela*   sky;
    SkyCtx  sky_ctx;
    Loop*   animation;

    Stickman stickman;
    f32      orbit_dist;
    f32      orbit_yaw;
    f32      orbit_pitch;

    bool     mouse_down;
    Vec2     last_mouse;
    bool     raytrace_mode;
    bool     should_reset_exposure;

    /* persistent props for the two stickman billboard triangles */
    RasterTriangleProps stickman_props[2];
    /* persistent material for stickman (diffuse, used in raytrace mode) */
    Material stickman_mat;
} App;

/* Ground props must outlive scene elements that reference them */
static RasterTriangleProps g_ground_props[GROUND_TILES * GROUND_TILES * 2];
static Material            g_ground_mat;

/* -----------------------------------------------------------------------
 * UV / animation helpers
 * --------------------------------------------------------------------- */

static void apply_frame(App* app, i32 row, i32 frame, bool flip_x) {
    const i32 inset = 4;
    f32 aw = (f32)app->atlas->width;
    f32 ah = (f32)app->atlas->height;

    f32 left   = (FRAME_COLS[frame][0] + inset) / aw;
    f32 right  = (FRAME_COLS[frame][1] - inset) / aw;
    f32 bottom = (ah - (f32)(ANIM_ROWS[row][1] - inset)) / ah;
    f32 top    = (ah - (f32)(ANIM_ROWS[row][0] + inset)) / ah;

    f32 u0 = flip_x ? right : left;
    f32 u1 = flip_x ? left  : right;

    /* Triangle A: bottomLeft, bottomRight, topRight */
    app->stickman_props[0].tex_coords[0] = vec2(u0, bottom);
    app->stickman_props[0].tex_coords[1] = vec2(u1, bottom);
    app->stickman_props[0].tex_coords[2] = vec2(u1, top);
    /* Triangle B: topRight, topLeft, bottomLeft */
    app->stickman_props[1].tex_coords[0] = vec2(u1, top);
    app->stickman_props[1].tex_coords[1] = vec2(u0, top);
    app->stickman_props[1].tex_coords[2] = vec2(u0, bottom);
}

/* -----------------------------------------------------------------------
 * Camera
 * --------------------------------------------------------------------- */

static void update_camera(App* app) {
    Vec3 center = add_vec3(app->stickman.position,
                           vec3(0.0f, 0.0f, BODY_HEIGHT / 2.0f));
    app->camera.look_at = center;
    set_orbit_camera(&app->camera,
                     app->orbit_dist, app->orbit_yaw, app->orbit_pitch);
}

/* Ground-plane forward and right vectors derived from camera yaw */
static void get_ground_axes(f32 yaw, Vec3* fwd, Vec3* rgt) {
    *fwd = vec3(-cosf(yaw), -sinf(yaw), 0.0f);
    *rgt = vec3(-sinf(yaw),  cosf(yaw), 0.0f);
}

/* -----------------------------------------------------------------------
 * Stickman update (physics + animation + billboard)
 * --------------------------------------------------------------------- */

static void update_stickman(App* app, f32 dt) {
    const u8* kb = SDL_GetKeyboardState(NULL);
    Stickman* s  = &app->stickman;
    Vec3 prev_pos = s->position;

    bool sprint = kb[SDL_SCANCODE_LSHIFT] || kb[SDL_SCANCODE_RSHIFT];
    i32  mx     = (i32)kb[SDL_SCANCODE_D] - (i32)kb[SDL_SCANCODE_A];
    i32  my     = (i32)kb[SDL_SCANCODE_W] - (i32)kb[SDL_SCANCODE_S];
    bool moving = (mx != 0) || (my != 0);
    f32  speed  = sprint ? SPRINT_SPEED : MOVE_SPEED;

    if (moving) {
        Vec3 fwd, rgt;
        get_ground_axes(app->orbit_yaw, &fwd, &rgt);
        Vec3 dir = normalize_vec3(add_vec3(scale_vec3(rgt, (f32)mx),
                                           scale_vec3(fwd, (f32)my)));
        s->velocity.x = dir.x * speed;
        s->velocity.y = dir.y * speed;
        if (fabsf(dot_vec3(dir, rgt)) > 1e-6f)
            s->facing_right = (dot_vec3(dir, rgt) >= 0.0f);
    } else {
        s->velocity.x = 0.0f;
        s->velocity.y = 0.0f;
    }

    s->velocity.z -= GRAVITY * dt;
    s->position    = add_vec3(s->position, scale_vec3(s->velocity, dt));

    if (s->position.z <= 0.0f) {
        s->position.z      = 0.0f;
        s->velocity.z      = 0.0f;
        s->on_ground       = true;
        s->jump_frame_time = 0.0f;
    } else {
        s->on_ground = false;
    }

    /* Animation state machine */
    AnimState next = s->on_ground ? (moving ? ANIM_MOVE : ANIM_IDLE) : ANIM_JUMP;
    if (next != s->anim_state) {
        s->anim_state = next;
        s->anim_time  = 0.0f;
    }
    s->anim_time += dt;

    i32  row = 0, frame = 0;
    bool flip = !s->facing_right;
    if (s->anim_state == ANIM_MOVE) {
        row = 1;
        s->move_frame_time += dt * (sprint ? 1.5f : 1.0f);
        frame = (i32)(s->move_frame_time * (sprint ? 14.0f : 10.0f)) % 10;
    } else if (s->anim_state == ANIM_JUMP) {
        row = 2;
        s->jump_frame_time += dt;
        i32 f = (i32)(s->jump_frame_time * 14.0f);
        frame = (f < 0) ? 0 : (f > 9 ? 9 : f);
    } else {
        row = 0;
        s->idle_frame_time += dt;
        frame = (i32)(s->idle_frame_time * 6.0f) % 10;
    }

    apply_frame(app, row, frame, flip);

    if (!equals_vec3(s->position, prev_pos) || frame != s->last_frame)
        app->should_reset_exposure = true;
    s->last_frame = frame;

    Vec3 prev_cam = app->camera.position;
    update_camera(app);
    if (!equals_vec3(app->camera.position, prev_cam))
        app->should_reset_exposure = true;

    /* Update billboard positions in the scene array in-place */
    Vec3 center  = add_vec3(s->position, vec3(0.0f, 0.0f, BODY_HEIGHT / 2.0f));
    Vec3 cam_rgt = scale_vec3(app->camera.basis[0], BODY_WIDTH  / 2.0f);
    Vec3 cam_up  = scale_vec3(app->camera.basis[1], BODY_HEIGHT / 2.0f);
    Vec3 bl = sub_vec3(sub_vec3(center, cam_rgt), cam_up);
    Vec3 br = sub_vec3(add_vec3(center, cam_rgt), cam_up);
    Vec3 tr = add_vec3(add_vec3(center, cam_rgt), cam_up);
    Vec3 tl = add_vec3(sub_vec3(center, cam_rgt), cam_up);

    Array* elems = get_scene_elems_scene(&app->scene);
    SceneElem* ea = (SceneElem*)get_array_element(elems, STICKMAN_BASE);
    SceneElem* eb = (SceneElem*)get_array_element(elems, STICKMAN_BASE + 1);
    ea->as.triangle.positions[0] = bl;
    ea->as.triangle.positions[1] = br;
    ea->as.triangle.positions[2] = tr;
    eb->as.triangle.positions[0] = tr;
    eb->as.triangle.positions[1] = tl;
    eb->as.triangle.positions[2] = bl;
}

/* -----------------------------------------------------------------------
 * Ground
 * --------------------------------------------------------------------- */

static void build_ground(Scene* scene) {
    i32 k = 0;
    for (i32 gx = 0; gx < GROUND_TILES; gx++) {
        for (i32 gy = 0; gy < GROUND_TILES; gy++) {
            f32 x0   = GROUND_MIN + (f32)gx * GROUND_STEP;
            f32 x1   = x0 + GROUND_STEP;
            f32 y0   = GROUND_MIN + (f32)gy * GROUND_STEP;
            f32 y1   = y0 + GROUND_STEP;
            f32 tint = 0.02f * (((gx + gy) % 2 == 0) ? 1.0f : -1.0f);

            Color ca = { 0.20f + tint, 0.28f + tint, 0.20f + tint, 1.0f };
            Color cb = { 0.22f + tint, 0.30f + tint, 0.22f + tint, 1.0f };

            Triangle t1 = build_triangle(
                vec3(x0, y0, 0), vec3(x1, y0, 0), vec3(x1, y1, 0));
            g_ground_props[k] = (RasterTriangleProps){
                .colors     = { ca, cb, ca },
                .tex_coords = { {0, 0}, {0, 0}, {0, 0} },
                .texture    = NULL,
                .material   = &g_ground_mat,
            };
            t1.props = &g_ground_props[k++];
            add_scene_elem_scene(scene, build_scene_elem_triangle(t1));

            Triangle t2 = build_triangle(
                vec3(x1, y1, 0), vec3(x0, y1, 0), vec3(x0, y0, 0));
            g_ground_props[k] = (RasterTriangleProps){
                .colors     = { cb, ca, cb },
                .tex_coords = { {0, 0}, {0, 0}, {0, 0} },
                .texture    = NULL,
                .material   = &g_ground_mat,
            };
            t2.props = &g_ground_props[k++];
            add_scene_elem_scene(scene, build_scene_elem_triangle(t2));
        }
    }
}

/* -----------------------------------------------------------------------
 * Sky background — equirectangular sky.jpg + alpha-aware directional
 * shadow (keeps tracing past transparent sprite pixels)
 * --------------------------------------------------------------------- */

static bool is_in_shadow_alpha(Scene* scene, Vec3 origin, Vec3 light_dir) {
    Ray shadow = build_ray(
        add_vec3(origin, scale_vec3(light_dir, 1e-3f)), light_dir);
    for (i32 steps = 0; steps < 8; steps++) {
        SceneHit hit = intersect_scene(scene, shadow);
        if (!hit.hit) return false;
        Color c = get_color_from_hit(hit, shadow, false);
        if (c.alpha > 0.5f) return true;
        /* transparent hit — advance origin past it and keep going */
        shadow.init = add_vec3(hit.position, scale_vec3(light_dir, 1e-3f));
    }
    return false;
}

static Color render_sky(Ray ray, void* ctx) {
    SkyCtx* sky_ctx = (SkyCtx*)ctx;
    Tela*   sky     = sky_ctx->sky;
    Vec3 dir = normalize_vec3(ray.dir);

    Color sky_color;
    if (sky) {
        f32 theta = atan2f(dir.y, dir.x) / (2.0f * (f32)PI) + 0.5f;
        f32 phi   = acosf(fminf(1.0f, fmaxf(-1.0f, -dir.z))) / (f32)PI;
        sky_color = get_pxl_tela(sky,
            (u32)(theta * (f32)sky->width),
            (u32)(phi   * (f32)sky->height));
    } else {
        sky_color = (Color){ 0.72f, 0.86f, 0.98f, 1.0f };
    }

    Vec3 sun_dir = normalize_vec3(vec3(-0.4f, -0.35f, 0.85f));
    if (is_in_shadow_alpha(sky_ctx->scene, ray.init, sun_dir)) {
        return lerp_color(sky_color, COLOR_BLACK, 0.5f);
    }
    f32 sun_dot       = fmaxf(0.0f, dot_vec3(sun_dir, dir));
    f32 sun_intensity = powf(sun_dot, 48.0f);
    return lerp_color(sky_color, COLOR_WHITE, sun_intensity);
}

/* -----------------------------------------------------------------------
 * Render
 * --------------------------------------------------------------------- */

static void render_raster(App* app) {
    raster_scene(&app->scene, (RasterParams){
        .cull_backfaces      = false,
        .bilinear_texture    = false,
        .clip_camera_plane   = true,
        .clear_screen        = true,
        .background_color    = { 0.72f, 0.86f, 0.98f, 1.0f },
        .perspective_correct = true,
        .near_plane_z        = 0.01f,
        .camera              = &app->camera,
        .tela                = app->tela,
    });
}

static void render_raytrace(App* app) {
    /* rebuild BVH every frame — stickman billboard positions change */
    app->scene.vtable->rebuild_scene(&app->scene);

    if (app->should_reset_exposure) {
        app->exposed_tela->iterations = 1;
        app->should_reset_exposure = false;
    }
    RaytraceParams params = {
        .samples_per_pixel         = 1,
        .bounces                   = 3,
        .variance                  = 0.001f,
        .gamma                     = 0.5f,
        .bilinear_texture          = false,
        .is_biased                 = false,
        .render_background         = render_sky,
        .render_background_context = &app->sky_ctx,
        .exposed_tela              = app->exposed_tela,
        .camera                    = &app->camera,
        .directional_light         = NULL,
    };
    ray_trace_scene_parallel(&app->scene, &params);
    /* blit accumulated result into display tela */
    memcpy(app->tela->image, app->exposed_tela->image,
           app->tela->width * app->tela->height * 4 * sizeof(f32));
}

/* -----------------------------------------------------------------------
 * Frame callback
 * --------------------------------------------------------------------- */

static void on_frame(f32 dt, f32 time, void* ctx) {
    App* app = (App*)ctx;
    dt = clamp(dt, 0.0f, 1.0f / 24.0f);

    update_stickman(app, dt);

    if (app->raytrace_mode)
        render_raytrace(app);
    else
        render_raster(app);

    const char* state =
        app->stickman.anim_state == ANIM_IDLE ? "idle" :
        app->stickman.anim_state == ANIM_MOVE ? "move" : "jump";
    const char* mode = app->raytrace_mode ? "raytrace" : "raster";
    char* title = format_string("Stickman | FPS: %.2f | %s | %s",
                                1.0f / dt, state, mode);
    set_window_title(app->window, title);
    free(title);

    paint_window(app->window, app->tela);
}

/* -----------------------------------------------------------------------
 * Input callbacks
 * --------------------------------------------------------------------- */

static void on_close(Window* w, void* ctx) {
    stop_loop((Loop*)ctx);
}

static void on_mouse_down(Window* w, i32 x, i32 y, u32 btn, void* ctx) {
    App* app = (App*)ctx;
    app->mouse_down = true;
    app->last_mouse = vec2((f32)x, (f32)y);
}

static void on_mouse_up(Window* w, i32 x, i32 y, u32 btn, void* ctx) {
    ((App*)ctx)->mouse_down = false;
}

static void on_mouse_move(Window* w, i32 x, i32 y, void* ctx) {
    App* app = (App*)ctx;
    if (!app->mouse_down) return;
    Vec2 mouse = vec2((f32)x, (f32)y);
    if (equals_vec2(mouse, app->last_mouse)) return;
    Vec2 delta = sub_vec2(mouse, app->last_mouse);
    app->orbit_yaw  -= 2.0f * (f32)PI * (delta.x / (f32)WIDTH);
    app->orbit_pitch = clamp(
        app->orbit_pitch - 2.0f * (f32)PI * (delta.y / (f32)HEIGHT),
        -0.15f, (f32)(PI / 2.5));
    app->should_reset_exposure = true;
    app->last_mouse = mouse;
}

static void on_scroll(Window* w, i32 dy, void* ctx) {
    App* app = (App*)ctx;
    app->orbit_dist = clamp(app->orbit_dist + dy * 0.5f, 3.0f, 10.0f);
    app->should_reset_exposure = true;
}

static void on_key_down(Window* w, u32 key, void* ctx) {
    App* app = (App*)ctx;
    if (key == SDLK_SPACE && app->stickman.on_ground) {
        app->stickman.velocity.z      = JUMP_SPEED;
        app->stickman.on_ground       = false;
        app->stickman.jump_frame_time = 0.0f;
        app->stickman.anim_state      = ANIM_JUMP;
        app->stickman.anim_time       = 0.0f;
    }
    if (key == SDLK_r) {
        app->raytrace_mode         = !app->raytrace_mode;
        app->should_reset_exposure = true;
    }
}

/* -----------------------------------------------------------------------
 * Main
 * --------------------------------------------------------------------- */

int main(void) {
    static App app;
    memset(&app, 0, sizeof(App));

    app.tela         = new_tela(WIDTH, HEIGHT);
    app.exposed_tela = new_tela(WIDTH, HEIGHT);
    app.window       = new_window(WIDTH * 2, HEIGHT * 2, "Stickman");
    app.atlas        = io_read_image("./assets/stickman_sprite.png");
    app.sky          = io_read_image("./assets/sky.jpg");

    app.orbit_dist  = 6.0f;
    app.orbit_yaw   = (f32)(PI * 0.75);
    app.orbit_pitch = 0.45f;
    app.raytrace_mode         = true;
    app.should_reset_exposure = true;

    app.camera = create_camera(
        vec3(0, 0, 0), vec3(0, 0, BODY_HEIGHT / 2.0f), 1.0f);
    update_camera(&app);

    app.stickman.facing_right = true;
    app.stickman.on_ground    = true;
    app.stickman.anim_state   = ANIM_IDLE;

    /* Materials (diffuse for everything) */
    g_ground_mat        = build_diffuse_material();
    app.stickman_mat    = build_diffuse_material();

    /* Build scene */
    app.scene = new_kscene(4);
    build_ground(&app.scene);

    /* Init stickman props */
    app.stickman_props[0] = (RasterTriangleProps){
        .colors   = { COLOR_WHITE, COLOR_WHITE, COLOR_WHITE },
        .texture  = app.atlas,
        .material = &app.stickman_mat,
    };
    app.stickman_props[1] = (RasterTriangleProps){
        .colors   = { COLOR_WHITE, COLOR_WHITE, COLOR_WHITE },
        .texture  = app.atlas,
        .material = &app.stickman_mat,
    };
    apply_frame(&app, 0, 0, false);

    /* Add stickman billboard triangles (positions updated every frame) */
    Vec3 center  = add_vec3(app.stickman.position,
                            vec3(0, 0, BODY_HEIGHT / 2.0f));
    Vec3 cam_rgt = scale_vec3(app.camera.basis[0], BODY_WIDTH  / 2.0f);
    Vec3 cam_up  = scale_vec3(app.camera.basis[1], BODY_HEIGHT / 2.0f);
    Vec3 bl = sub_vec3(sub_vec3(center, cam_rgt), cam_up);
    Vec3 br = sub_vec3(add_vec3(center, cam_rgt), cam_up);
    Vec3 tr = add_vec3(add_vec3(center, cam_rgt), cam_up);
    Vec3 tl = add_vec3(sub_vec3(center, cam_rgt), cam_up);

    Triangle sa = build_triangle(bl, br, tr);
    sa.props = &app.stickman_props[0];
    Triangle sb = build_triangle(tr, tl, bl);
    sb.props = &app.stickman_props[1];
    add_scene_elem_scene(&app.scene, build_scene_elem_triangle(sa));
    add_scene_elem_scene(&app.scene, build_scene_elem_triangle(sb));
    app.scene.vtable->rebuild_scene(&app.scene);

    app.sky_ctx = (SkyCtx){ .sky = app.sky, .scene = &app.scene };

    /* Wire up loop and input */
    Loop* animation = loop(on_frame, &app);
    app.animation = animation;

    on_close_window(app.window, on_close, animation);
    on_mouse_down_window(app.window, on_mouse_down, &app);
    on_mouse_up_window(app.window, on_mouse_up, &app);
    on_mouse_move_window(app.window, on_mouse_move, &app);
    on_mouse_scroll_window(app.window, on_scroll, &app);
    on_key_down_window(app.window, on_key_down, &app);

    play_loop(animation);
    return 0;
}
