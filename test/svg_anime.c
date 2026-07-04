/**
 * SVG Anime
 *
 * Animates an SVG file:
 *  - Each path's outline is drawn progressively ("writes itself")
 *  - Fill fades black → white once the stroke is past the halfway point
 *
 * Direct port of tela.js test/node/svg_anime.js
 *
 * Usage:  ./svg_anime [path/to/file.svg]
 * Output: svg_anime.mp4  (requires ffmpeg)
 * Compile: gcc -O3 -fopenmp -o svg_anime test/svg_anime.c -lSDL2 -lm
 */

#include "../src/index.c"

// gcc -O3 -fopenmp -o svg_anime test/svg_anime.c -lSDL2 -lm

static const u32 WIDTH  = 640;
static const u32 HEIGHT = 480;
static const u32 FPS    = 30;
static const f32 MAX_T  = 3.0f;

/* =============================================================================
 * Winding Number (matches JS isInsideCurve)
 * ========================================================================== */

/* Winding number of point p w.r.t. a single closed path (pixel coords). */
static i32 path_winding(Vec2 p, const Array* path) {
    f32 theta = 0.0f;
    u32 np = path->length;
    for (u32 j = 0; j + 1 < np; j++) {
        Vec2 a = *(Vec2*)get_array_element((Array*)path, j);
        Vec2 b = *(Vec2*)get_array_element((Array*)path, j + 1);
        Vec2 u = sub_vec2(p, a);
        Vec2 v = sub_vec2(p, b);
        f32  ti = atan2f(u.y, u.x);
        f32  tj = atan2f(v.y, v.x);
        f32  dt = tj - ti;
        /* Wrap to (-π, π] */
        dt -= 2.0f * (f32)PI * roundf(dt / (2.0f * (f32)PI));
        theta += dt;
    }
    return (i32)roundf(theta / (2.0f * (f32)PI));
}

/* Summed winding across all paths; true when count < 0 (matches JS). */
static bool is_inside(Vec2 p, Array** px_paths, const AABB_2D* boxes, u32 n) {
    i32 count = 0;
    for (u32 i = 0; i < n; i++) {
        /* Bounding-box early-out */
        if (p.x < boxes[i].min.x || p.x > boxes[i].max.x ||
            p.y < boxes[i].min.y || p.y > boxes[i].max.y) continue;
        count += path_winding(p, px_paths[i]);
    }
    return count < 0;
}

/* =============================================================================
 * Animation Context
 * ========================================================================== */

typedef struct {
    Tela*    tela;
    Array**  px_paths; /* paths in pixel coordinates */
    AABB_2D* boxes;
    u32      n;
    f32      max_t;
} AnimCtx;

static bool anim_until(f32 time) { return time < MAX_T; }

static void draw_frame(f32 dt, f32 time, void* raw) {
    (void)dt;
    AnimCtx* ctx = (AnimCtx*)raw;
    f32 n = time / ctx->max_t;   /* normalised time [0, 1] */

    for (u32 i = 0; i < ctx->n; i++) {
        Array*  path = ctx->px_paths[i];
        AABB_2D box  = ctx->boxes[i];
        u32 np       = path->length;

        /* ---- Fill: black → white ---------------------------------------- */
        f32 t_fill = fmaxf(0.0f, 2.0f * n - 0.75f);
        if (t_fill > 0.0f) {
            u32 x0 = (u32)fmaxf(0.0f,               box.min.x);
            u32 y0 = (u32)fmaxf(0.0f,               box.min.y);
            u32 x1 = (u32)fminf((f32)(WIDTH  - 1),  box.max.x);
            u32 y1 = (u32)fminf((f32)(HEIGHT - 1),  box.max.y);

#pragma omp parallel for schedule(dynamic, 4)
            for (i32 py = (i32)y0; py <= (i32)y1; py++) {
                for (u32 px = x0; px <= x1; px++) {
                    Vec2 p = vec2((f32)px, (f32)py);
                    if (is_inside(p, ctx->px_paths, ctx->boxes, ctx->n)) {
                        Color c = {t_fill, t_fill, t_fill, 1.0f};
                        set_pxl_tela(ctx->tela, px, (u32)py, c);
                    }
                }
            }
        }

        /* ---- Outline: progressive stroke --------------------------------- */
        if (np < 2) continue;

        f32 end_pt = 2.0f * n * (f32)(np - 1);
        u32 ep_fl  = (u32)floorf(end_pt);
        if (ep_fl >= np) ep_fl = np - 1;

        Color white = {1.0f, 1.0f, 1.0f, 1.0f};

        /* All fully-completed segments */
        for (u32 j = 0; j + 1 < ep_fl && j + 1 < np; j++) {
            Vec2 p0 = *(Vec2*)get_array_element(path, j);
            Vec2 p1 = *(Vec2*)get_array_element(path, j + 1);
            Line_2D seg = { .positions = {p0, p1} };
            draw_line_tela(ctx->tela, &seg, _svg_solid_line_shader, &white);
        }

        /* Partial last segment */
        if ((f32)ep_fl < end_pt && ep_fl + 1 < np) {
            f32  frac = end_pt - (f32)ep_fl;
            Vec2 pa   = *(Vec2*)get_array_element(path, ep_fl);
            Vec2 pb   = *(Vec2*)get_array_element(path, ep_fl + 1);
            Vec2 pmid = add_vec2(scale_vec2(pa, 1.0f - frac),
                                 scale_vec2(pb, frac));
            Line_2D seg = { .positions = {pa, pmid} };
            draw_line_tela(ctx->tela, &seg, _svg_solid_line_shader, &white);
        }
    }
}

/* =============================================================================
 * Main
 * ========================================================================== */

int main(int argc, char** argv) {
    const char* svg_path = argc > 1 ? argv[1] : "./assets/cross.svg";

    String file = io_read_file(svg_path);
    if (!file.data) {
        fprintf(stderr, "Cannot open '%s'\n", svg_path);
        return 1;
    }

    SVGData svg = parse_svg(file.data, file.length);
    free(file.data);

    u32 n = svg.paths.length;
    if (n == 0) {
        fprintf(stderr, "No paths found in '%s'\n", svg_path);
        free_svg(&svg);
        return 1;
    }

    /* Scale normalised [0,1]^2 paths to pixel coordinates (floor, Y-down) */
    Array** px_paths = malloc(n * sizeof(Array*));
    AABB_2D* boxes   = malloc(n * sizeof(AABB_2D));

    for (u32 i = 0; i < n; i++) {
        Array* src  = *(Array**)get_array_element(&svg.paths, i);
        px_paths[i] = svg_new_subpath();
        for (u32 j = 0; j < src->length; j++) {
            Vec2 p = *(Vec2*)get_array_element(src, j);
            p.x = floorf(p.x * (f32)WIDTH);
            p.y = floorf(p.y * (f32)HEIGHT);
            push_array(px_paths[i], &p);
        }
        boxes[i] = svg_path_aabb(px_paths[i]);
    }

    /* Black canvas — NOT cleared between frames (accumulates, like JS) */
    Tela* tela = new_tela(WIDTH, HEIGHT);
    fill_tela(tela, (Color){0.0f, 0.0f, 0.0f, 1.0f});

    AnimCtx ctx = {
        .tela     = tela,
        .px_paths = px_paths,
        .boxes    = boxes,
        .n        = n,
        .max_t    = MAX_T,
    };

    Loop* anim = loop(draw_frame, &ctx);
    loop_to_video(anim, "svg_anime.mp4", (LoopVideoParams){
        .tela  = tela,
        .fps   = FPS,
        .until = anim_until,
    });

    for (u32 i = 0; i < n; i++) { free_array(px_paths[i]); free(px_paths[i]); }
    free(px_paths);
    free(boxes);
    free_svg(&svg);
    return 0;
}
