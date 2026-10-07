#include "../src/index.c"

#include <math.h>
#include <stdio.h>

// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Copyright (c) 2026 @Frostbyte
//[LICENSE] https://creativecommons.org/licenses/by-nc-sa/4.0/
// AI Port from https://fragcoord.xyz/s/mi99r927

// gcc -O3 -fopenmp -ffast-math -o app test/amazing_shader_2.c -lSDL2 -lm -march=native && ./app


/* ============================================================================
 * Constants
 * ========================================================================== */

static const u32 WIDTH  = 640/2;
static const u32 HEIGHT = 480/2;

static const u32 AA = 2;


/* ============================================================================
 * Application
 * ========================================================================== */

typedef struct {
  Tela* tela;
  Window* window;
  bool use_parallel;
} App;


/* ============================================================================
 * Helpers
 * ========================================================================== */

static inline f32 saturate(f32 x) {
  return fminf(fmaxf(x, 0.0f), 1.0f);
}


/* ============================================================================
 * 2D rotation
 * ========================================================================== */

/*
 * GLSL:
 *
 * vec2 r(vec2 v,float t)
 * {
 *     float s=sin(t),c=cos(t);
 *     return mat2(c,-s,s,c)*v;
 * }
 */

static inline Vec2 rotate_vec2(Vec2 v, f32 t) {

  const f32 s = sinf(t);
  const f32 c = cosf(t);

  return vec2(
      c * v.x - s * v.y,
      s * v.x + c * v.y
  );
}


/* ============================================================================
 * XOR Dot Noise
 * ========================================================================== */

/*
 * Important:
 *
 * GLSL matrix constructors are column-major.
 *
 * Therefore:
 *
 *     GOLD * p
 *
 * and:
 *
 *     p * GOLD
 *
 * are different operations.
 *
 * The original shader intentionally uses BOTH:
 *
 *     cos(GOLD * p)
 *
 * and:
 *
 *     sin(PHI * p * GOLD)
 *
 * so we implement both explicitly.
 */

static inline f32 dot_noise(Vec3 p) {

  const f32 PHI = 1.618033988f;


  /*
   * GLSL:
   *
   * mat3 GOLD = mat3(
   *     -0.571464913, +0.814921382, +0.096597072,
   *     -0.278044873, -0.303026659, +0.911518454,
   *     +0.772087367, +0.494042493, +0.399753815
   * );
   *
   * Column-major:
   *
   *       | a d g |
   * GOLD =| b e h |
   *       | c f i |
   */

  const f32 a = -0.571464913f;
  const f32 b =  0.814921382f;
  const f32 c =  0.096597072f;

  const f32 d = -0.278044873f;
  const f32 e = -0.303026659f;
  const f32 f =  0.911518454f;

  const f32 g =  0.772087367f;
  const f32 h =  0.494042493f;
  const f32 i =  0.399753815f;


  /*
   * GOLD * p
   */

  const Vec3 gp = vec3(
      a * p.x + d * p.y + g * p.z,
      b * p.x + e * p.y + h * p.z,
      c * p.x + f * p.y + i * p.z
  );


  /*
   * p * GOLD
   *
   * Row-vector multiplication.
   */

  const Vec3 pg = vec3(
      p.x * a + p.y * b + p.z * c,
      p.x * d + p.y * e + p.z * f,
      p.x * g + p.y * h + p.z * i
  );


  /*
   * dot(
   *     cos(GOLD * p),
   *     sin(PHI * p * GOLD)
   * )
   */

  return
      cosf(gp.x) * sinf(PHI * pg.x) +
      cosf(gp.y) * sinf(PHI * pg.y) +
      cosf(gp.z) * sinf(PHI * pg.z);
}


/* ============================================================================
 * ACES Tonemap
 * ========================================================================== */

static inline Vec3 aces_tonemap(Vec3 c) {

  /*
   * GLSL:
   *
   * mat3 m1=mat3(
   *     0.59719,0.07600,0.02840,
   *     0.35458,0.90834,0.13383,
   *     0.04823,0.01566,0.83777
   * );
   *
   * mat3 m2=mat3(
   *     1.60475,-0.10208,-0.00327,
   *    -0.53108,1.10813,-0.07276,
   *    -0.07367,-0.00605,1.07602
   * );
   */

  const f32 m1a =  0.59719f;
  const f32 m1b =  0.07600f;
  const f32 m1c =  0.02840f;

  const f32 m1d =  0.35458f;
  const f32 m1e =  0.90834f;
  const f32 m1f =  0.13383f;

  const f32 m1g =  0.04823f;
  const f32 m1h =  0.01566f;
  const f32 m1i =  0.83777f;


  const f32 m2a =  1.60475f;
  const f32 m2b = -0.10208f;
  const f32 m2c = -0.00327f;

  const f32 m2d = -0.53108f;
  const f32 m2e =  1.10813f;
  const f32 m2f = -0.07276f;

  const f32 m2g = -0.07367f;
  const f32 m2h = -0.00605f;
  const f32 m2i =  1.07602f;


  /*
   * v = m1 * c
   */

  const Vec3 v = vec3(
      m1a * c.x + m1d * c.y + m1g * c.z,
      m1b * c.x + m1e * c.y + m1h * c.z,
      m1c * c.x + m1f * c.y + m1i * c.z
  );


  /*
   * a = v*(v+0.0245786)-0.000090537
   */

  const Vec3 aa = vec3(
      v.x * (v.x + 0.0245786f) - 0.000090537f,
      v.y * (v.y + 0.0245786f) - 0.000090537f,
      v.z * (v.z + 0.0245786f) - 0.000090537f
  );


  /*
   * b = v*(0.983729*v+0.4329510)+0.238081
   */

  const Vec3 bb = vec3(
      v.x * (0.983729f * v.x + 0.4329510f) + 0.238081f,
      v.y * (0.983729f * v.y + 0.4329510f) + 0.238081f,
      v.z * (0.983729f * v.z + 0.4329510f) + 0.238081f
  );


  /*
   * a / b
   */

  const Vec3 q = vec3(
      aa.x / bb.x,
      aa.y / bb.y,
      aa.z / bb.z
  );


  /*
   * m2 * q
   */

  return vec3(
      m2a * q.x + m2d * q.y + m2g * q.z,
      m2b * q.x + m2e * q.y + m2h * q.z,
      m2c * q.x + m2f * q.y + m2i * q.z
  );
}


/* ============================================================================
 * Fragment Shader
 * ========================================================================== */

static Color fragment_shader(
    u32 x,
    u32 y,
    const void* context
) {

  const f32 time = *(const f32*)context;


  /*
   * ShaderToy:
   *
   * gl_FragCoord.xy
   *
   * tela.c already supplies y with the same bottom-up convention
   * used by the original shader.
   */

  const f32 width  = (f32)WIDTH;
  const f32 height = (f32)HEIGHT;


  /*
   * d=normalize(
   *     vec3(
   *         2.*u-iResolution.xy,
   *         iResolution.y
   *     )
   * );
   */

  Vec3 final_color = vec3(
      0.0f,
      0.0f,
      0.0f
  );


  /* ========================================================================
   * 2x2 anti-aliasing
   * ====================================================================== */

  for (u32 m = 0; m < AA; m++) {

    for (u32 n = 0; n < AA; n++) {


      /*
       * GLSL:
       *
       * vec2 off =
       *     (vec2(float(m), float(n)) + 0.5)
       *     / float(AA) - 0.5;
       */

      const f32 off_x =
          ((f32)m + 0.5f) / (f32)AA - 0.5f;

      const f32 off_y =
          ((f32)n + 0.5f) / (f32)AA - 0.5f;


      const f32 u_x =
          (f32)x + off_x;

      const f32 u_y =
          (f32)y + off_y;


      /* ====================================================================
       * Initial ray position
       * ================================================================== */

      Vec3 p = vec3(
          0.0f,
          0.0f,
          -1.0f - 0.5f * sinf(time * 0.1f)
      );


      /* ====================================================================
       * Ray direction
       * ================================================================== */

      Vec3 d = vec3(
          2.0f * u_x - width,
          2.0f * u_y - height,
          height
      );

      d = normalize_vec3(d);


      /* ====================================================================
       * Light accumulation
       * ================================================================== */

      Vec3 light = vec3(
          0.0f,
          0.0f,
          0.0f
      );


      /* ====================================================================
       * Ray march
       * ================================================================== */

      for (u32 iter = 0; iter < 10; iter++) {

        /*
         * b = p
         */

        Vec3 b = p;


        /*
         * b.xy = r(
         *     sin(b.xy*.25),
         *     t*.5+b.z*2.
         * );
         */

        const Vec2 rotated = rotate_vec2(
            vec2(
                sinf(b.x * 0.25f),
                sinf(b.y * 0.25f)
            ),
            time * 0.5f + b.z * 2.0f
        );

        b.x = rotated.x;
        b.y = rotated.y;


        /*
         * s=.001+
         *   abs(no(b*20.)/20.-no(b))*.7;
         */

        const Vec3 b20 = vec3(
            b.x * 20.0f,
            b.y * 20.0f,
            b.z * 20.0f
        );

        const f32 n20 = dot_noise(b20);
        const f32 n0  = dot_noise(b);


        f32 s =
            0.001f +
            fabsf(n20 / 20.0f - n0) * 0.7f;


        /*
         * s += abs(
         *     p.y*.2+
         *     sin(
         *         p.z*2.+
         *         (abs(p.x)*.5)
         *     )
         * )*.5;
         */

        s +=
            fabsf(
                p.y * 0.2f +
                sinf(
                    p.z * 2.0f +
                    fabsf(p.x) * 0.5f
                )
            ) * 0.5f;


        /*
         * p += d*s;
         */

        p.x += d.x * s;
        p.y += d.y * s;
        p.z += d.z * s;


        /*
         * l +=
         *   (
         *       1.+
         *       1.5*sin(
         *           i+
         *           length(p.xy*.1)+
         *           2.+
         *           vec3(3,1.5,.5)
         *       )
         *   ) / s;
         */

        const f32 radial =
            sqrtf(
                p.x * p.x +
                p.y * p.y
            ) * 0.1f;


        light.x +=
            (
                1.0f +
                1.5f * sinf(
                    (f32)iter +
                    radial +
                    2.0f +
                    3.0f
                )
            ) / s;


        light.y +=
            (
                1.0f +
                1.5f * sinf(
                    (f32)iter +
                    radial +
                    2.0f +
                    1.5f
                )
            ) / s;


        light.z +=
            (
                1.0f +
                1.5f * sinf(
                    (f32)iter +
                    radial +
                    2.0f +
                    0.5f
                )
            ) / s;
      }


      /* ====================================================================
       * ACES
       *
       * color += a(l*l/5e2);
       * ================================================================== */

      const Vec3 mapped = aces_tonemap(
          vec3(
              light.x * light.x / 500.0f,
              light.y * light.y / 500.0f,
              light.z * light.z / 500.0f
          )
      );


      final_color.x += mapped.x;
      final_color.y += mapped.y;
      final_color.z += mapped.z;
    }
  }


  /* ========================================================================
   * Average AA samples
   * ====================================================================== */

  const f32 samples =
      (f32)(AA * AA);

  final_color.x /= samples;
  final_color.y /= samples;
  final_color.z /= samples;


  /* ========================================================================
   * Return Tela color
   * ====================================================================== */

  return (Color){
      .red   = saturate(final_color.x),
      .green = saturate(final_color.y),
      .blue  = saturate(final_color.z),
      .alpha = 1.0f
  };
}


/* ============================================================================
 * Frame
 * ========================================================================== */

static void on_frame(
    f32 dt,
    f32 time,
    void* ctx
) {

  App* app = (App*)ctx;


  if (app->use_parallel) {

    map_tela_parallel(
        app->tela,
        fragment_shader,
        &time
    );

  } else {

    map_tela(
        app->tela,
        fragment_shader,
        &time
    );
  }


  set_window_title(
      app->window,
      format_string(
          "ShaderToy Raymarch | Parallel: %s (P) | FPS: %.1f",
          app->use_parallel ? "ON" : "OFF",
          dt > 0.0f ? 1.0f / dt : 0.0f
      )
  );


  paint_window(
      app->window,
      app->tela
  );
}


/* ============================================================================
 * Keyboard
 * ========================================================================== */

static void on_key_down(
    Window* window,
    u32 keycode,
    void* ctx
) {

  (void)window;

  App* app = (App*)ctx;


  if (keycode == SDLK_p) {

    app->use_parallel =
        !app->use_parallel;

  }
}


/* ============================================================================
 * Close
 * ========================================================================== */

static void on_close(
    Window* window,
    void* ctx
) {

  (void)window;

  Loop* animation = (Loop*)ctx;

  stop_loop(animation);
}


/* ============================================================================
 * Main
 * ========================================================================== */

int main(void) {

  /*
   * Create Tela canvas.
   */

  Tela* tela =
      new_tela(
          WIDTH,
          HEIGHT
      );


  /*
   * Create SDL window.
   */

  Window* window =
      new_window(
          WIDTH,
          HEIGHT,
          "ShaderToy Raymarch"
      );


  if (!tela || !window) {

    fprintf(
        stderr,
        "Failed to create Tela or window.\n"
    );

    return 1;
  }


  /*
   * Application state.
   */

  App app = {
      .tela = tela,
      .window = window,
      .use_parallel = true
  };


  /*
   * Animation.
   */

  Loop* animation =
      loop(
          on_frame,
          &app
      );


  /*
   * Events.
   */

  on_close_window(
      window,
      on_close,
      animation
  );


  on_key_down_window(
      window,
      on_key_down,
      &app
  );


  /*
   * Run.
   */

  play_loop(animation);


  /*
   * Cleanup.
   */

  free_loop(animation);
  free_tela(tela);
  free_window(window);

  return 0;
}