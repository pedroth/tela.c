/**
 * SVG Anime 2
 *
 * Port of tela.js test/node/svg_anime_2.js
 *
 * For each pixel, computes the continuous (non-rounded) winding number w.r.t.
 * all SVG paths, using only the first N = floor(2*n*(L-1)) segments per path
 * (L = path length, n = time/maxT).  Colour blends BLUE → YELLOW as the
 * winding number grows from 0 to 1, producing an "inking" animation.
 *
 * Usage:  ./svg_anime_2 [path/to/file.svg]
 * Output: svg_anime_2.mp4  (requires ffmpeg)
 * Compile: gcc -O3 -fopenmp -o svg_anime_2 test/svg_anime_2.c -lSDL2 -lm
 */

#include "../src/index.c"

// gcc -O3 -fopenmp -o svg_anime_2 test/svg_anime_2.c -lSDL2 -lm

static const u32 WIDTH  = 640;
static const u32 HEIGHT = 480;
static const u32 FPS    = 30;
static const f32 MAX_T  = 10.0f;

/* =============================================================================
 * Winding-number pixel shader
 * ========================================================================== */

typedef struct {
    Array** px_paths;
    u32     n_paths;
    f32     n_norm;   /* = time / MAX_T, in [0, 1] */
} WNCtx;

/*
 * Continuous winding number of pixel (x,y) w.r.t. all paths, summed.
 * Uses only the first N = floor(2*n*(path.length-1)) segments per path.
 * Matches JS windingNumber() with theta += -dTheta (negated accumulation).
 *
 * Called for every pixel via map_tela_parallel — keep it branch-light.
 */
static Color wn_shader(u32 x, u32 y, const void* raw) {
    const WNCtx* ctx = (const WNCtx*)raw;
    Vec2 p = vec2((f32)x, (f32)y);
    f32  t = 0.0f;
    f32  n = ctx->n_norm;

    for (u32 i = 0; i < ctx->n_paths; i++) {
        const Array* path = ctx->px_paths[i];
        u32 np = path->length;
        if (np < 2) continue;

        u32 N = (u32)floorf(2.0f * n * (f32)(np - 1));
        if (N > np - 1) N = np - 1;

        f32 theta = 0.0f;
        for (u32 j = 0; j < N; j++) {
            Vec2 a  = *(Vec2*)get_array_element((Array*)path, j);
            Vec2 b  = *(Vec2*)get_array_element((Array*)path, j + 1);
            Vec2 u  = sub_vec2(p, a);
            Vec2 v  = sub_vec2(p, b);
            f32  ti = atan2f(u.y, u.x);
            f32  tj = atan2f(v.y, v.x);
            f32  dt = tj - ti;
            /* Wrap to (-π, π] */
            dt -= 2.0f * (f32)PI * roundf(dt / (2.0f * (f32)PI));
            theta += -dt;   /* negated — matches JS: theta += -dTheta */
        }
        t += theta / (2.0f * (f32)PI);
    }

    /* BLUE*(1-t) + YELLOW*t  →  red=t, green=t, blue=1-t */
    return (Color){ t, t, 1.0f - t, 1.0f };
}

/* =============================================================================
 * Animation loop
 * ========================================================================== */

typedef struct {
    Tela*  tela;
    WNCtx  wn;
    f32    max_t;
} AnimCtx;

static bool anim_until(f32 time) { return time < MAX_T; }

static void draw_frame(f32 dt, f32 time, void* raw) {
    (void)dt;
    AnimCtx* ctx   = (AnimCtx*)raw;
    ctx->wn.n_norm = time / ctx->max_t;
    map_tela_parallel(ctx->tela, wn_shader, &ctx->wn);
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
        fprintf(stderr, "No paths in '%s'\n", svg_path);
        free_svg(&svg);
        return 1;
    }

    /* Scale normalised [0,1]^2 → pixel coordinates.
     * parse_svg already flips Y, so p.y=0 is canvas-bottom (y-from-bottom
     * convention used by map_tela_parallel / draw_line_tela). */
    Array** px_paths = malloc(n * sizeof(Array*));
    for (u32 i = 0; i < n; i++) {
        Array* src  = *(Array**)get_array_element(&svg.paths, i);
        px_paths[i] = svg_new_subpath();
        for (u32 j = 0; j < src->length; j++) {
            Vec2 p = *(Vec2*)get_array_element(src, j);
            p.x = floorf(p.x * (f32)WIDTH);
            p.y = floorf(p.y * (f32)HEIGHT);
            push_array(px_paths[i], &p);
        }
    }

    Tela* tela = new_tela(WIDTH, HEIGHT);
    fill_tela(tela, (Color){0.0f, 0.0f, 1.0f, 1.0f}); /* initial frame = BLUE */

    AnimCtx ctx = {
        .tela  = tela,
        .wn    = { .px_paths = px_paths, .n_paths = n, .n_norm = 0.0f },
        .max_t = MAX_T,
    };

    Loop* anim = loop(draw_frame, &ctx);
    loop_to_video(anim, "svg_anime_2.mp4", (LoopVideoParams){
        .tela  = tela,
        .fps   = FPS,
        .until = anim_until,
    });

    for (u32 i = 0; i < n; i++) { free_array(px_paths[i]); free(px_paths[i]); }
    free(px_paths);
    free_svg(&svg);
    return 0;
}
