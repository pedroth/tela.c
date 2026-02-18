/**
 * tela.c - Unity Build for Tela Library
 *
 * Include this single file to get access to the entire Tela library.
 * This is a pure C unity build with everything inlined.
 *
 * Usage in your application:
 *   #include "src/index.c"
 *
 * Compile with:
 *   cc -o myapp myapp.c -lSDL2 -lm
 *
 * Or optimized compilation with:
 *   cc -O3 -o myapp myapp.c -lSDL2 -lm
 */

#ifndef TELA_C
#define TELA_C

 /* POSIX features for clock_gettime - must be before includes */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

/* ============================================================
 * Standard Library Includes
 * ============================================================ */
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

 /* SDL2 for windowing */
#include <SDL2/SDL.h>

//========================================================================================
/*                                                                                      *
 *                                         TYPES *
 *                                                                                      */
 //========================================================================================

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

typedef float f32;
typedef double f64;

typedef bool bool_t;

//========================================================================================
/*                                                                                      *
 *                                        RANDOM *
 *                                                                                      */
 //========================================================================================

static inline double random_double(void) { return (double)rand() / RAND_MAX; }

//========================================================================================
/*                                                                                      *
 *                                         MATH                                         *
 *                                                                                      */
 //========================================================================================

static inline f32 mod_f32(f32 n, f32 m) {
  return fmodf(fmodf(n, m) + m, m);
}

static inline u32 mod_u32(u32 n, u32 m) {
  return ((n % m) + m) % m;
}

static inline f32 clamp(f32 x, f32 min, f32 max) {
  if (x < min) return min;
  if (x > max) return max;
  return x;
}

static inline f32 lerp_f32(f32 a, f32 b, f32 t) {
  return a + (b - a) * t;
}

static inline f32 q_bezier_f32(f32 p1, f32 p2, f32 p3, f32 t) {
  f32 q1 = lerp_f32(p1, p2, t);
  f32 q2 = lerp_f32(p2, p3, t);
  return lerp_f32(q1, q2, t);
}

static inline f32 c_bezier_f32(f32 p1, f32 p2, f32 p3, f32 p4, f32 t) {
  f32 b1 = lerp_f32(p1, p2, t);
  f32 b2 = lerp_f32(p2, p3, t);
  f32 b3 = lerp_f32(p3, p4, t);

  f32 c1 = lerp_f32(b1, b2, t);
  f32 c2 = lerp_f32(b2, b3, t);
  return lerp_f32(c1, c2, t);
}

//========================================================================================
/*                                                                                      *
 *                                         ARRAY *
 *                                                                                      */
 //========================================================================================

typedef struct {
  void* data;
  u32 length;
  u32 capacity;
  u32 element_size;
} Array;

Array new_array(u32 capacity, u32 element_size) {
  Array array;
  array.data = malloc(capacity * element_size);
  array.length = 0;
  array.capacity = capacity;
  array.element_size = element_size;
  return array;
}

Array push_array(Array* a, const void* element) {
  // Grow array if at capacity
  if (a->length >= a->capacity) {
    u32 new_capacity = (a->capacity == 0) ? 4 : a->capacity * 2;
    void* new_data = realloc(a->data, new_capacity * a->element_size);
    if (!new_data) {
      return *a;
    }
    a->data = new_data;
    a->capacity = new_capacity;
  }

  // Copy element to end of array
  char* destination = (char*)a->data + (a->length * a->element_size);
  memcpy(destination, element, a->element_size);
  a->length++;

  return *a;
}

Array clear_array(Array* a) {
  a->length = 0;
  return *a;
}

Array filter_array(Array* a, bool (*func)(void* element, u32 index)) {
  Array ans = new_array(a->capacity, a->element_size);
  for (u32 i = 0; i < a->length; i++) {
    void* element = (char*)a->data + (i * a->element_size);
    if (func(element, i)) {
      push_array(&ans, element);
    }
  }
  return ans;
}

void* get_array_element(Array* a, u32 index) {
  if (index >= a->length || index < 0) {
    return NULL;
  }
  char* base = (char*)a->data;
  u32 offset = index * a->element_size;
  return base + offset;
}

void* pop_array(Array* a) {
  if (a->length == 0) {
    return NULL;
  }

  a->length--;
  if (a->length > 0 && a->length <= a->capacity / 4) {
    u32 new_cap = a->capacity / 2;
    void* new_data = realloc(a->data, new_cap * a->element_size);
    if (new_data) {
      a->data = new_data;
      a->capacity = new_cap;
    }
  }
  return (char*)a->data + (a->length * a->element_size);
}

void free_array(Array* array) {
  free(array->data);
  array->data = NULL;
  array->length = 0;
  array->capacity = 0;
}

//========================================================================================
/*                                                                                      *
 *                                         VEC2 *
 *                                                                                      */
 //========================================================================================

#define EPSILON 0.0001f

typedef struct {
  f32 x;
  f32 y;
} Vec2;

static inline Vec2 vec2(f32 x, f32 y) {
  Vec2 v;
  v.x = x;
  v.y = y;
  return v;
}

static inline Vec2 add_vec2(const Vec2 a, const Vec2 b) {
  Vec2 result;
  result.x = a.x + b.x;
  result.y = a.y + b.y;
  return result;
}

static inline Vec2 sub_vec2(const Vec2 a, const Vec2 b) {
  Vec2 result;
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  return result;
}

static inline Vec2 mul_vec2(const Vec2 a, const Vec2 b) {
  Vec2 result;
  result.x = a.x * b.x;
  result.y = a.y * b.y;
  return result;
}

static inline Vec2 div_vec2(const Vec2 a, const Vec2 b) {
  Vec2 result;
  result.x = a.x / b.x;
  result.y = a.y / b.y;
  return result;
}

static inline Vec2 scale_vec2(const Vec2 v, f32 scalar) {
  Vec2 result;
  result.x = v.x * scalar;
  result.y = v.y * scalar;
  return result;
}

static inline f32 dot_vec2(const Vec2 a, const Vec2 b) {
  return a.x * b.x + a.y * b.y;
}

static inline f32 length_vec2(const Vec2 v) {
  return sqrtf(v.x * v.x + v.y * v.y);
}

static inline bool normalize_vec2(const Vec2 v, Vec2* out) {
  f32 len = length_vec2(v);
  if (len < EPSILON) {
    return false;
  }
  *out = scale_vec2(v, 1.0f / len);
  return true;
}

static inline bool equals_vec2(const Vec2 a, const Vec2 b) {
  return fabsf(a.x - b.x) < EPSILON && fabsf(a.y - b.y) < EPSILON;
}

static inline Vec2 map_vec2(Vec2 v, f32(*func)(f32)) {
  Vec2 result;
  result.x = func(v.x);
  result.y = func(v.y);
  return result;
}

static inline Vec2 op_vec2(Vec2 a, Vec2 b, f32(*func)(f32, f32)) {
  Vec2 result;
  result.x = func(a.x, b.x);
  result.y = func(a.y, b.y);
  return result;
}

static inline f32 fold_vec2(Vec2 v, f32(*func)(f32, f32), f32 initial) {
  return func(func(initial, v.x), v.y);
}

static inline bool isnan_vec2(Vec2 v) { return isnan(v.x) || isnan(v.y); }

static inline f32 wedge_vec2(Vec2 a, Vec2 b) {
  return a.x * b.y - a.y * b.x;
}
//========================================================================================
/*                                                                                      *
 *                                         VEC3 *
 *                                                                                      */
 //========================================================================================

typedef struct {
  f32 x;
  f32 y;
  f32 z;
} Vec3;

static inline Vec3 vec3(f32 x, f32 y, f32 z) {
  Vec3 v;
  v.x = x;
  v.y = y;
  v.z = z;
  return v;
}

static inline Vec3 add_vec3(const Vec3 a, const Vec3 b) {
  Vec3 result;
  result.x = a.x + b.x;
  result.y = a.y + b.y;
  result.z = a.z + b.z;
  return result;
}

static inline Vec3 sub_vec3(const Vec3 a, const Vec3 b) {
  Vec3 result;
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  result.z = a.z - b.z;
  return result;
}

static inline Vec3 scale_vec3(const Vec3 v, f32 scalar) {
  Vec3 result;
  result.x = v.x * scalar;
  result.y = v.y * scalar;
  result.z = v.z * scalar;
  return result;
}

static inline f32 dot_vec3(const Vec3 a, const Vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline f32 length_vec3(const Vec3 v) {
  return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline Vec3 cross_vec3(const Vec3 a, const Vec3 b) {
  Vec3 result;
  result.x = a.y * b.z - a.z * b.y;
  result.y = a.z * b.x - a.x * b.z;
  result.z = a.x * b.y - a.y * b.x;
  return result;
}

static inline bool normalize_vec3(const Vec3 v, Vec3* out) {
  f32 len = length_vec3(v);
  if (len < EPSILON) {
    return false;
  }
  *out = scale_vec3(v, 1.0f / len);
  return true;
}

static inline bool equals_vec3(const Vec3 a, const Vec3 b) {
  return fabsf(a.x - b.x) < EPSILON && fabsf(a.y - b.y) < EPSILON &&
    fabsf(a.z - b.z) < EPSILON;
}

static inline Vec3 map_vec3(Vec3 v, f32(*func)(f32)) {
  Vec3 result;
  result.x = func(v.x);
  result.y = func(v.y);
  result.z = func(v.z);
  return result;
}

static inline Vec3 op_vec3(Vec3 a, Vec3 b, f32(*func)(f32, f32)) {
  Vec3 result;
  result.x = func(a.x, b.x);
  result.y = func(a.y, b.y);
  result.z = func(a.z, b.z);
  return result;
}

//========================================================================================
/*                                                                                      *
 *                                         COLOR *
 *                                                                                      */
 //========================================================================================

typedef struct {
  f32 red;
  f32 green;
  f32 blue;
  f32 alpha;
} Color;

static inline bool equals_color(const Color a, const Color b) {
  return (a.red == b.red) && (a.green == b.green) && (a.blue == b.blue) &&
    (a.alpha == b.alpha);
}

static inline Color random_color() {
  Color c;
  c.red = (f32)rand() / (f32)RAND_MAX;
  c.green = (f32)rand() / (f32)RAND_MAX;
  c.blue = (f32)rand() / (f32)RAND_MAX;
  c.alpha = 1.0f;
  return c;
}

static inline Color lerp_color(Color a, Color b, f32 t) {
  Color result;
  result.red = lerp_f32(a.red, b.red, t);
  result.green = lerp_f32(a.green, b.green, t);
  result.blue = lerp_f32(a.blue, b.blue, t);
  result.alpha = lerp_f32(a.alpha, b.alpha, t);
  return result;
}

//========================================================================================
/*                                                                                      *
 *                                        AABB_2D *
 *                                                                                      */
 //========================================================================================

typedef struct {
  bool is_empty;
  Vec2 min;
  Vec2 max;
  Vec2 center;
  Vec2 diagonal;
} AABB_2D;

#define EMPTY_AABB_2D                                                          \
  (AABB_2D) {                                                                  \
    .is_empty = true, .min = {0, 0}, .max = {0, 0}, .center = {0, 0},          \
    .diagonal = {0, 0}                                                         \
  }

static inline AABB_2D build_aabb_2d(Vec2 min, Vec2 max) {
  AABB_2D box;
  box.min = vec2(fminf(min.x, max.x), fminf(min.y, max.y));
  box.max = vec2(fmaxf(min.x, max.x), fmaxf(min.y, max.y));
  box.center = scale_vec2(add_vec2(box.min, box.max), 0.5f);
  box.diagonal = vec2(box.max.x - box.min.x, box.max.y - box.min.y);
  box.is_empty = box.diagonal.x < 0.0f || box.diagonal.y < 0.0f;
  return box;
}

AABB_2D build_aabb_from_vec2(Vec2 point) {
  return build_aabb_2d(point, point);
}

static inline AABB_2D union_aabb_2d(const AABB_2D* a, const AABB_2D* b) {
  if (a->is_empty)
    return (AABB_2D) {
    .is_empty = b->is_empty,
      .min = b->min,
      .max = b->max,
      .center = b->center,
      .diagonal = b->diagonal
  };
  if (b->is_empty)
    return (AABB_2D) {
    .is_empty = a->is_empty,
      .min = a->min,
      .max = a->max,
      .center = a->center,
      .diagonal = a->diagonal
  };

  Vec2 new_min = op_vec2(a->min, b->min, fminf);
  Vec2 new_max = op_vec2(a->max, b->max, fmaxf);
  return build_aabb_2d(new_min, new_max);
}

static inline AABB_2D sub_aabb_2d(const AABB_2D* box, const AABB_2D* other) {
  if (box->is_empty || other->is_empty)
    return EMPTY_AABB_2D;
  Vec2 new_min = op_vec2(box->min, other->min, fmaxf);
  Vec2 new_max = op_vec2(box->max, other->max, fminf);
  const Vec2 new_diag = sub_vec2(new_max, new_min);
  const bool is_all_positive = new_diag.x >= 0 && new_diag.y >= 0;
  return is_all_positive ? build_aabb_2d(new_min, new_max) : EMPTY_AABB_2D;
}

static inline bool collides_aabb_2d(const AABB_2D* box, const AABB_2D* other) {
  return !sub_aabb_2d(box, other).is_empty;
}

static inline f32 max_comp_vec2(const Vec2 v) { return fmaxf(v.x, v.y); }

static inline f32 distance_aabb_2d(const AABB_2D* box, const Vec2 point) {
  const Vec2 p = sub_vec2(point, box->center);
  const Vec2 r = sub_vec2(box->max, box->center);
  const Vec2 q = sub_vec2(map_vec2(p, fabsf), r);
  return length_vec2(op_vec2(q, vec2(0, 0), fmaxf)) +
    fminf(0, max_comp_vec2(q));
}

//========================================================================================
/*                                                                                      *
 *                                         AABB *
 *                                                                                      */
 //========================================================================================

typedef struct {
  bool is_empty;
  Vec3 min;
  Vec3 max;
  Vec3 center;
  Vec3 diagonal;
} AABB;

#define EMPTY_AABB                                                             \
  (AABB) {                                                                     \
    .is_empty = true, .min = {0, 0, 0}, .max = {0, 0, 0}, .center = {0, 0, 0}, \
    .diagonal = {0, 0, 0}                                                      \
  }

static inline AABB build_aabb(Vec3 min, Vec3 max) {
  AABB box;
  box.min = vec3(fminf(min.x, max.x), fminf(min.y, max.y), fminf(min.z, max.z));
  box.max = vec3(fmaxf(min.x, max.x), fmaxf(min.y, max.y), fmaxf(min.z, max.z));
  box.center = scale_vec3(add_vec3(box.min, box.max), 0.5f);
  box.diagonal =
    vec3(box.max.x - box.min.x, box.max.y - box.min.y, box.max.z - box.min.z);
  box.is_empty =
    box.diagonal.x < 0.0f || box.diagonal.y < 0.0f || box.diagonal.z < 0.0f;
  return box;
}

static inline AABB union_aabb(const AABB* a, const AABB* b) {
  if (a->is_empty)
    return (AABB) {
    .is_empty = b->is_empty,
      .min = b->min,
      .max = b->max,
      .center = b->center,
      .diagonal = b->diagonal
  };
  if (b->is_empty)
    return (AABB) {
    .is_empty = a->is_empty,
      .min = a->min,
      .max = a->max,
      .center = a->center,
      .diagonal = a->diagonal
  };

  Vec3 new_min = vec3(fminf(a->min.x, b->min.x), fminf(a->min.y, b->min.y),
    fminf(a->min.z, b->min.z));
  Vec3 new_max = vec3(fmaxf(a->max.x, b->max.x), fmaxf(a->max.y, b->max.y),
    fmaxf(a->max.z, b->max.z));
  return build_aabb(new_min, new_max);
}

static inline AABB sub_aabb(const AABB* box, const AABB* other) {
  if (box->is_empty || other->is_empty)
    return EMPTY_AABB;
  Vec3 new_min =
    vec3(fmaxf(box->min.x, other->min.x), fmaxf(box->min.y, other->min.y),
      fmaxf(box->min.z, other->min.z));
  Vec3 new_max =
    vec3(fminf(box->max.x, other->max.x), fminf(box->max.y, other->max.y),
      fminf(box->max.z, other->max.z));
  const Vec3 new_diag = sub_vec3(new_max, new_min);
  const bool is_all_positive =
    new_diag.x >= 0 && new_diag.y >= 0 && new_diag.z >= 0;
  return is_all_positive ? build_aabb(new_min, new_max) : EMPTY_AABB;
}

static inline bool collides_aabb(const AABB* box, const AABB* other) {
  return !sub_aabb(box, other).is_empty;
}

static inline f32 max_comp_vec3(const Vec3 v) {
  return fmaxf(fmaxf(v.x, v.y), v.z);
}

static inline f32 distance_aabb(const AABB* box, const Vec3 point) {
  const Vec3 p = sub_vec3(point, box->center);
  const Vec3 r = sub_vec3(box->max, box->center);
  const Vec3 q = sub_vec3(map_vec3(p, fabsf), r);
  return length_vec3(op_vec3(q, vec3(0, 0, 0), fmaxf)) +
    fminf(0, max_comp_vec3(q));
}

//========================================================================================
/*                                                                                      *
 *                                        LINE_2D *
 *                                                                                      */
 //========================================================================================

typedef struct {
  Vec2 positions[2];
  void* props;
} Line_2D;

//========================================================================================
/*                                                                                      *
 *                                         LINE *
 *                                                                                      */
 //========================================================================================

typedef struct {
  Vec3 positions[2];
  void* props;
} Line;

//========================================================================================
/*                                                                                      *
 *                                      Triangle_2D                                     *
 *                                                                                      */
 //========================================================================================


typedef struct {
  Vec2 positions[3];
  void* props;
} Triangle_2D;


//========================================================================================
/*                                                                                      *
 *                                       TRIANGLE                                       *
 *                                                                                      */
 //========================================================================================

typedef struct {
  Vec3 positions[3];
  void* props;
} Triangle;

AABB get_bounding_box_triangle(Triangle* triangle) {
  AABB box;
  AABB b1 = build_aabb(triangle->positions[0], triangle->positions[0]);
  AABB b2 = build_aabb(triangle->positions[1], triangle->positions[1]);
  AABB b3 = build_aabb(triangle->positions[2], triangle->positions[2]);
  box = union_aabb(&b1, &b2);
  box = union_aabb(&box, &b3);
  return box;
}

//========================================================================================
/*                                                                                      *
 *                                      NAIVE_SCENE                                     *
 *                                                                                      */
 //========================================================================================

typedef struct {
  Array triangles;
} NaiveScene;

NaiveScene add_triangle_nscene(NaiveScene* scene, Triangle triangle) {
  if (scene->triangles.element_size == 0) {
    scene->triangles = new_array(4, sizeof(Triangle));
  }
  push_array(&scene->triangles, &triangle);
  return *scene;
}

NaiveScene add_triangles_nscene(NaiveScene* scene, Array triangles) {
  for (u32 i = 0; i < triangles.length; i++) {
    Triangle* tri = (Triangle*)get_array_element(&triangles, i);
    add_triangle_nscene(scene, *tri);
  }
  return *scene;
}


NaiveScene clear_triangles_nscene(NaiveScene* scene) {
  clear_array(&scene->triangles);
  return *scene;
}

//========================================================================================
/*                                                                                      *
 *                                         TELA *
 *                                                                                      */
 //========================================================================================

#define COLOR_CHANNELS 4

typedef struct {
  u32 width;
  u32 height;
  u32 channels;
  f32* image;
  AABB_2D box;
} Tela;

static inline Tela* new_tela(u32 width, u32 height) {
  Tela* tela = (Tela*)malloc(sizeof(Tela));
  tela->width = width;
  tela->height = height;
  tela->channels = COLOR_CHANNELS;
  tela->box = build_aabb_2d(vec2(0.0f, 0.0f), vec2((f32)width, (f32)height));
  tela->image = (f32*)calloc(width * height * COLOR_CHANNELS, sizeof(f32));
  return tela;
}

static inline void free_tela(Tela* tela) {
  free(tela->image);
  tela->image = NULL;
  free(tela);
}

static inline Tela* map_tela(Tela* tela,
  Color(*lambda)(u32, u32, void const*),
  void const* context) {
  const u32 w = tela->width;
  const u32 h = tela->height;
  const u32 c = tela->channels;
  const u32 size = w * h * c;
  for (u32 k = 0; k < size; k += c) {
    u32 i = k / (c * w);
    u32 j = (k / c) % w;
    const u32 x = j;
    const u32 y = h - 1 - i;
    Color color = lambda(x, y, context);
    if (color.alpha == 0.0f) {
      continue;
    }
    tela->image[k + 0] = color.red;
    tela->image[k + 1] = color.green;
    tela->image[k + 2] = color.blue;
    tela->image[k + 3] = color.alpha;
  }
  return tela;
}

static inline Tela* fill_tela(Tela* tela, Color color) {
  const u32 w = tela->width;
  const u32 h = tela->height;
  const u32 c = tela->channels;
  const u32 size = w * h * c;
  for (u32 k = 0; k < size; k += c) {
    tela->image[k + 0] = color.red;
    tela->image[k + 1] = color.green;
    tela->image[k + 2] = color.blue;
    tela->image[k + 3] = color.alpha;
  }
  return tela;
}

static inline Vec2 to_grid_tela(const Tela* tela, u32 x, u32 y) {
  const u32 h = tela->height;
  const u32 j = x;
  const u32 i = h - 1 - y;
  return vec2((f32)i, (f32)j);
}

static inline Color get_pxl_tela(const Tela* tela, u32 x, u32 y) {
  const u32 w = tela->width;
  const u32 h = tela->height;
  Vec2 grid = to_grid_tela(tela, x, y);
  u32 i = (u32)grid.x;
  u32 j = (u32)grid.y;
  i = mod_u32(i, h);
  j = mod_u32(j, w);
  u32 index = COLOR_CHANNELS * (w * i + j);
  return (Color) { tela->image[index], tela->image[index + 1], tela->image[index + 2], tela->image[index + 3] };
}

/**
 * return solution to : [ v_0 , 0] x = f_0
 *                      [ v_1,  a] y = f_1
 */
void solve_low_tri_matrix(Vec2 v, f32 a, Vec2 f, Vec2* out) {
  const f32 v1 = v.x;
  const f32 v2 = v.y;
  const f32 av1 = a * v1;
  if (av1 == 0 || v1 == 0)
    return;
  const f32 f1 = f.x;
  const f32 f2 = f.y;
  *out = (Vec2){ f1 / v1, (f2 * v1 - v2 * f1) / av1 };
}

/**
 * return solution to : [ v_0 , a] x = f_0
 *					            [ v_1,  0] y = f_1
 */
void solve_up_tri_matrix(Vec2 v, f32 a, Vec2 f, Vec2* out) {
  const f32 v1 = v.x;
  const f32 v2 = v.y;
  const f32 av2 = a * v2;
  if (av2 == 0 || v2 == 0)
    return;
  const f32 f1 = f.x;
  const f32 f2 = f.y;
  *out = (Vec2){ f2 / v2, (f1 * v2 - v1 * f2) / av2 };
}

typedef struct {
  Vec2 points[4];
  u32 length;
} LineBoxIntersection;

LineBoxIntersection line_box_intersection(Vec2 start, Vec2 end, AABB_2D box) {
  LineBoxIntersection result = { 0 };
  f32 width = box.diagonal.x;
  f32 height = box.diagonal.y;
  Vec2 v = sub_vec2(end, start);
  // point and direction of boundary
  Vec2 boundary[4][2] = {
      {(Vec2) { 0.0f, 0.0f }, (Vec2) { width, 0.0f }},
      {(Vec2) { width, 0.0f }, (Vec2) { 0.0f, height }},
      {(Vec2) { width, height }, (Vec2) { -width, 0.0f }},
      {(Vec2) { 0.0f, height }, (Vec2) { 0.0f, -height }},
  };
  LineBoxIntersection intersection_solutions = { 0 };
  for (u32 i = 0; i < 4; i++) {
    Vec2 s = boundary[i][0];
    Vec2 d = boundary[i][1];
    if (d.x == 0) {
      Vec2 solution = { NAN, NAN };
      solve_low_tri_matrix(v, -d.y, sub_vec2(s, start), &solution);
      if (!isnan_vec2(solution)) {
        intersection_solutions.points[intersection_solutions.length++] =
          solution;
      }
    }
    else {
      Vec2 solution = { NAN, NAN };
      solve_up_tri_matrix(v, -d.x, sub_vec2(s, start), &solution);
      if (!isnan_vec2(solution)) {
        intersection_solutions.points[intersection_solutions.length++] =
          solution;
      }
    }
  }
  LineBoxIntersection valid_intersections = { 0 };
  for (u32 i = 0; i < intersection_solutions.length; i++) {
    const f32 x = intersection_solutions.points[i].x;
    const f32 y = intersection_solutions.points[i].y;
    if (0 <= x && x <= 1 && 0 <= y && y <= 1) {
      valid_intersections.points[valid_intersections.length++] = (Vec2){ x, y };
    }
  }
  if (valid_intersections.length == 0) {
    return result;
  }
  if (valid_intersections.length >= 2) {
    Vec2 p1 = add_vec2(start, scale_vec2(v, valid_intersections.points[0].x));
    Vec2 p2 = add_vec2(start, scale_vec2(v, valid_intersections.points[1].x));
    result.points[0] = p1;
    result.points[1] = p2;
    result.length = 2;
    return result;
  }
  // it can be shown that at this point there is only one valid intersection
  result.points[0] =
    add_vec2(start, scale_vec2(v, valid_intersections.points[0].x));
  result.length = 1;
  return result;
}

bool clip_line(Vec2 p0, Vec2 p1, AABB_2D box, Line_2D* clipped_line) {
  AABB_2D p0_box = { .is_empty = false,
                    .min = p0,
                    .max = p0,
                    .center = p0,
                    .diagonal = vec2(0, 0) };
  AABB_2D p1_box = { .is_empty = false,
                    .min = p1,
                    .max = p1,
                    .center = p1,
                    .diagonal = vec2(0, 0) };
  bool p0_inside = collides_aabb_2d(&box, &p0_box);
  bool p1_inside = collides_aabb_2d(&box, &p1_box);

  // both points are inside
  if (p0_inside && p1_inside) {
    clipped_line->positions[0] = p0;
    clipped_line->positions[1] = p1;
    return true;
  }

  // one of them is inside
  if (p0_inside && !p1_inside) {
    LineBoxIntersection intersection = line_box_intersection(p0, p1, box);
    if (intersection.length == 0)
      return false;
    clipped_line->positions[0] = p0;
    clipped_line->positions[1] = intersection.points[0];
    return true;
  }

  if (!p0_inside && p1_inside) {
    LineBoxIntersection intersection = line_box_intersection(p0, p1, box);
    if (intersection.length == 0)
      return false;
    clipped_line->positions[0] = intersection.points[0];
    clipped_line->positions[1] = p1;
    return true;
  }

  // both points are outside, need to intersect the boundary
  LineBoxIntersection intersection = line_box_intersection(p0, p1, box);
  if (intersection.length < 2)
    return false;
  clipped_line->positions[0] = intersection.points[0];
  clipped_line->positions[1] = intersection.points[1];
  return true;
}

static inline Tela* draw_line_tela(Tela* tela, const Line_2D* line,
  Color(*func)(u32, u32, Line_2D*, void*),
  void* context) {

  u32 w = tela->width;
  u32 h = tela->height;
  Vec2 p1 = add_vec2(line->positions[0], (Vec2) { 0.5f, 0.5f }); // center of pixel
  Vec2 p2 = add_vec2(line->positions[1], (Vec2) { 0.5f, 0.5f });   // center of pixel
  Line_2D clipped_line;
  if (!clip_line(p1, p2, tela->box, &clipped_line))
    return tela;
  // Copy context from original line to clipped line
  clipped_line.props = line->props;
  Vec2 pi = clipped_line.positions[0];
  Vec2 pf = clipped_line.positions[1];
  Vec2 v = sub_vec2(pf, pi);

  f32 nf = fabsf(v.x) + fabsf(v.y) + 5.0f;
  u32 n = (u32)nf;

  for (u32 k = 0; k < n; k++) {
    f32 s = k / (nf - 1.0f);
    Vec2 lineP = add_vec2(pi, scale_vec2(v, s));
    u32 x = floorf(lineP.x + 0.5f);
    u32 y = floorf(lineP.y + 0.5f);
    if (x < 0 || x >= w || y < 0 || y >= h)
      continue;
    u32 j = x;
    u32 i = h - 1 - y;
    u32 index = COLOR_CHANNELS * (i * w + j);
    Color color = func(x, y, &clipped_line, context);
    if (color.alpha == 0.0f)
      continue;
    tela->image[index] = color.red;
    tela->image[index + 1] = color.green;
    tela->image[index + 2] = color.blue;
    tela->image[index + 3] = color.alpha;
  }
  return tela;
}

typedef struct {
  Vec2 normals[3];
  Vec2 vertices[3];
  u32 vertex_count;
  f32 orientation;
} ConvexPrecomputed;

static inline bool is_inside_convex(const ConvexPrecomputed* precomputed, Vec2 point) {
  const u32 m = precomputed->vertex_count;
  const Vec2* vertices = precomputed->vertices;
  const Vec2* normals = precomputed->normals;
  const f32 orientation = precomputed->orientation;
  for (u32 i = 0; i < m; i++) {
    const Vec2 r = sub_vec2(point, vertices[i]);
    const f32 myDot = dot_vec2(r, normals[i]) * orientation;
    if (myDot < 0) return false;
  }
  return true;
}


static inline Tela* draw_triangle_tela(
  Tela* tela,
  const Triangle_2D* triangle,
  Color(*shader)(u32, u32, const Triangle_2D*, void*),
  void* context
) {
  const u32 width = tela->width;
  const u32 height = tela->height;
  const AABB_2D canvasBox = tela->box;
  AABB_2D boundingBox = EMPTY_AABB_2D;
  for (u32 i = 0; i < 3; i++) {
    AABB_2D pointBox = build_aabb_from_vec2(triangle->positions[i]);
    boundingBox = union_aabb_2d(&boundingBox, &pointBox);
  }
  const AABB_2D finalBox = sub_aabb_2d(&canvasBox, &boundingBox);
  if (finalBox.is_empty) return tela;
  const Vec2 xmin = finalBox.min;
  const Vec2 xmax = finalBox.max;

  // Precompute edge normals and orientation on the stack (no malloc)
  const Vec2* positions = triangle->positions;
  const Vec2 e0 = sub_vec2(positions[1], positions[0]);
  const Vec2 e1 = sub_vec2(positions[2], positions[1]);
  const Vec2 e2 = sub_vec2(positions[0], positions[2]);
  const ConvexPrecomputed precomputed = {
    .normals = {
      (Vec2){ -e0.y, e0.x },
      (Vec2){ -e1.y, e1.x },
      (Vec2){ -e2.y, e2.x },
    },
    .vertices = { positions[0], positions[1], positions[2] },
    .vertex_count = 3,
    .orientation = wedge_vec2(e0, e1) >= 0 ? 1.0f : -1.0f,
  };
  for (u32 x = xmin.x; x < xmax.x; x++) {
    for (u32 y = xmin.y; y < xmax.y; y++) {
      if (is_inside_convex(&precomputed, (Vec2) { (f32)x, (f32)y })) {
        const u32 j = x;
        const u32 i = height - 1 - y;
        const Color color = shader(x, y, triangle, context);
        if (color.alpha == 0.0f) continue;
        const u32 index = COLOR_CHANNELS * (i * width + j);
        tela->image[index] = color.red;
        tela->image[index + 1] = color.green;
        tela->image[index + 2] = color.blue;
        tela->image[index + 3] = color.alpha;
      }
    }
  }
  return tela;
}


//========================================================================================
/*                                                                                      *
 *                                          IO *
 *                                                                                      */
 //========================================================================================

static inline void tela_to_p3(Tela* tela, const char* filename) {
  if (!tela || !tela->image) {
    return;
  }
  const u32 width = (u32)tela->width;
  const u32 height = (u32)tela->height;
  const u32 channels = tela->channels;
  f32* pixel_data = tela->image;

  FILE* file = fopen(filename, "w");
  if (!file) {
    return;
  }

  fprintf(file, "P3\n");
  fprintf(file, "%u %u\n", width, height);
  fprintf(file, "255\n");

  for (u32 k = 0; k < width * height * channels; k += channels) {
    u8 r = (u8)(fminf(fmaxf(pixel_data[k + 0], 0.0f), 1.0f) * 255.0f);
    u8 g = (u8)(fminf(fmaxf(pixel_data[k + 1], 0.0f), 1.0f) * 255.0f);
    u8 b = (u8)(fminf(fmaxf(pixel_data[k + 2], 0.0f), 1.0f) * 255.0f);
    fprintf(file, "%u %u %u\n", r, g, b);
  }

  fclose(file);
}

static inline void tela_to_image(Tela* tela, const char* filename) {
  const char* temp_ppm = "temp_output.ppm";
  tela_to_p3(tela, temp_ppm);
  char command[256];
  snprintf(command, sizeof(command), "ffmpeg -y -i %s %s", temp_ppm, filename);
  printf("Executing command: %s\n", command);
  int ret = system(command);
  if (ret == 0) {
    remove(temp_ppm);
  }
}

typedef struct {
  char* data;
  u32 length;
} String;

static inline String io_read_file(const char* filename) {
  FILE* file = fopen(filename, "rb");
  if (!file) {
    return (String) { NULL, 0 };
  }

  fseek(file, 0, SEEK_END);
  long length = ftell(file);
  fseek(file, 0, SEEK_SET);

  char* buffer = (char*)malloc(length + 1);
  if (!buffer) {
    fclose(file);
    return (String) { NULL, 0 };
  }

  size_t bytes_read = fread(buffer, 1, length, file);
  buffer[bytes_read] = '\0';

  fclose(file);
  return (String) { buffer, (u32)length };
}

// parse P6 type of PPM image file
Tela* io_parse_ppm(String ppm_data) {
  if (!ppm_data.data || ppm_data.length == 0) return NULL;

  const u8* data = (const u8*)ppm_data.data;
  u32 length = ppm_data.length;
  u32 index = 0;

  // Skip 3 header lines (format, dimensions, max color)
  u32 header_lines = 3;
  while (header_lines > 0 && index < length) {
    if (data[index] == '\n') header_lines--;
    index++;
  }

  // Parse width, height, maxColor from the header
  u32 width = 0, height = 0, max_color = 0;
  sscanf((const char*)data, "%*s %u %u %u", &width, &height, &max_color);
  if (width == 0 || height == 0 || max_color == 0) return NULL;

  Tela* tela = new_tela(width, height);
  if (!tela) return NULL;

  f32 inv_max = 1.0f / (f32)max_color;

  // Read pixel data (raw bytes after header)
  for (u32 k = 0; k < width * height && index + 2 < length; k++) {
    u32 img_idx = k * COLOR_CHANNELS;
    tela->image[img_idx + 0] = (f32)data[index] * inv_max;
    tela->image[img_idx + 1] = (f32)data[index + 1] * inv_max;
    tela->image[img_idx + 2] = (f32)data[index + 2] * inv_max;
    tela->image[img_idx + 3] = 1.0f;
    index += 3;
  }

  return tela;
}

// parse P7 PAM (Portable Arbitrary Map) image file
Tela* io_parse_pam(String pam_data) {
  if (!pam_data.data || pam_data.length == 0) return NULL;

  const char* data = pam_data.data;
  u32 length = pam_data.length;
  u32 index = 0;

  // Verify P7 magic number
  if (length < 3 || data[0] != 'P' || data[1] != '7') return NULL;

  u32 width = 0, height = 0, depth = 0, max_val = 0;

  // Parse PAM header lines until ENDHDR
  while (index < length) {
    // Find line start and end
    u32 line_start = index;
    while (index < length && data[index] != '\n') index++;
    if (index < length) index++; // skip '\n'

    if (strncmp(data + line_start, "WIDTH ", 6) == 0) {
      sscanf(data + line_start + 6, "%u", &width);
    } else if (strncmp(data + line_start, "HEIGHT ", 7) == 0) {
      sscanf(data + line_start + 7, "%u", &height);
    } else if (strncmp(data + line_start, "DEPTH ", 6) == 0) {
      sscanf(data + line_start + 6, "%u", &depth);
    } else if (strncmp(data + line_start, "MAXVAL ", 7) == 0) {
      sscanf(data + line_start + 7, "%u", &max_val);
    } else if (strncmp(data + line_start, "ENDHDR", 6) == 0) {
      break;
    }
  }

  if (width == 0 || height == 0 || max_val == 0 || depth == 0) return NULL;

  Tela* tela = new_tela(width, height);
  if (!tela) return NULL;

  f32 inv_max = 1.0f / (f32)max_val;
  const u8* pixel_data = (const u8*)(data + index);
  u32 remaining = length - index;

  // Read pixel data (raw bytes after header)
  // Buffer stores pixels in file order (top-to-bottom, row 0 = top of image).
  // The Y-flip happens in get_pxl_tela/to_grid_tela when reading, matching JS.
  for (u32 k = 0; k < width * height && (k * depth + depth - 1) < remaining; k++) {
    u32 img_idx = k * COLOR_CHANNELS;
    u32 src_idx = k * depth;
    tela->image[img_idx + 0] = (f32)pixel_data[src_idx + 0] * inv_max;
    tela->image[img_idx + 1] = (depth > 1) ? (f32)pixel_data[src_idx + 1] * inv_max : (f32)pixel_data[src_idx + 0] * inv_max;
    tela->image[img_idx + 2] = (depth > 2) ? (f32)pixel_data[src_idx + 2] * inv_max : (f32)pixel_data[src_idx + 0] * inv_max;
    tela->image[img_idx + 3] = (depth > 3) ? (f32)pixel_data[src_idx + 3] * inv_max : 1.0f;
  }

  return tela;
}

Tela* io_read_image(const char* filename) {
  // Check if file has .ppm extension — read directly as PPM
  const char* dot = strrchr(filename, '.');
  if (dot && (strcmp(dot, ".ppm") == 0 || strcmp(dot, ".PPM") == 0)) {
    String file_data = io_read_file(filename);
    if (!file_data.data) return NULL;
    Tela* tela = io_parse_ppm(file_data);
    free(file_data.data);
    return tela;
  }

  // Convert other formats to PAM using ffmpeg (with alpha), then parse
  char temp_pam[512];
  snprintf(temp_pam, sizeof(temp_pam), "temp_read_%d.pam", (int)(rand() % 1000000));

  char command[1024];
  snprintf(command, sizeof(command), "ffmpeg -y -i %s -pix_fmt rgba -update 1 %s", filename, temp_pam);
  int ret = system(command);
  if (ret != 0) return NULL;

  String file_data = io_read_file(temp_pam);
  remove(temp_pam);
  if (!file_data.data) return NULL;

  Tela* tela = io_parse_pam(file_data);
  free(file_data.data);
  return tela;
}

//========================================================================================
/*                                                                                      *
 *                                         TIME *
 *                                                                                      */
 //========================================================================================

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

static inline u32 get_time_ms(void) {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    return 0;

  u64 ms = (u64)ts.tv_sec * 1000ULL + (u64)ts.tv_nsec / 1000000ULL;
  return (u32)ms;
}

//========================================================================================
/*                                                                                      *
 *                                     STRING UTILS *
 *                                                                                      */
 //========================================================================================

static inline String create_string(const char* data) {
  u32 length = (u32)strlen(data);
  return (String) { .data = (char*)data, .length = length };
}

static inline char* to_cstring(String str) {
  return str.data;
}

static inline char* format_string(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int size = vsnprintf(NULL, 0, fmt, args);
  va_end(args);

  char* buffer = (char*)malloc(size + 1);
  if (!buffer) {
    return NULL;
  }

  va_start(args, fmt);
  vsnprintf(buffer, size + 1, fmt, args);
  va_end(args);

  return buffer;
}

//========================================================================================
/*                                                                                      *
 *                                         LOOP *
 *                                                                                      */
 //========================================================================================

typedef struct {
  void (*update)(f32, f32, void* context);
  void* context;
  bool running;
} Loop;

static inline void stop_loop(Loop* loop) { loop->running = false; }

static inline Loop* loop(void (*update)(f32, f32, void* ctx), void* context) {
  Loop* new_loop = (Loop*)malloc(sizeof(Loop));
  new_loop->update = update;
  new_loop->context = context;
  new_loop->running = true;
  return new_loop;
}

static inline void play_loop(Loop* loop) {
  u32 old_time = get_time_ms();
  f32 time = 0.0f;
  while (loop->running) {
    f32 dt = (get_time_ms() - old_time) / 1000.0f;
    old_time = get_time_ms();
    time += dt;
    loop->update(dt, time, loop->context);
  }
}

static inline void free_loop(Loop* loop) { free(loop); }

//========================================================================================
/*                                                                                      *
 *                                        WINDOW *
 *                                                                                      */
 //========================================================================================

typedef struct Window Window;
struct Window {
  i32 width;
  i32 height;
  char* title;
  SDL_Window* sdl_window;
  SDL_Renderer* renderer;
  SDL_Texture* texture;
  u32* pixels;
  void (*on_close_callback)(Window* window, void* context);
  void* on_close_context;

  void (*on_mouse_down_callback)(Window* window, i32 x, i32 y, u32 button,
    void* context);
  void* on_mouse_down_context;

  void (*on_mouse_up_callback)(Window* window, i32 x, i32 y, u32 button,
    void* context);
  void* on_mouse_up_context;

  void (*on_mouse_move_callback)(Window* window, i32 x, i32 y, void* context);
  void* on_mouse_move_context;

  void (*on_mouse_scroll_callback)(Window* window, i32 scroll_y, void* context);
  void* on_mouse_scroll_context;
};

static inline void transform_mouse_coordinates(Window* window, i32 x, i32 y,
  i32 out[2]) {
  i32 transformed_x = x;
  i32 transformed_y = window->height - 1 - y;
  out[0] = transformed_x;
  out[1] = transformed_y;
}

static inline void process_window_events(Window* window) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT ||
      (event.type == SDL_WINDOWEVENT &&
        event.window.event == SDL_WINDOWEVENT_CLOSE)) {
      if (window->on_close_callback) {
        window->on_close_callback(window, window->on_close_context);
      }
    }
    else if (event.type == SDL_MOUSEBUTTONDOWN) {
      if (window->on_mouse_down_callback) {
        i32 transformed_coords[2];
        transform_mouse_coordinates(
          window,
          event.button.x,
          event.button.y,
          transformed_coords
        );
        window->on_mouse_down_callback(
          window,
          transformed_coords[0],
          transformed_coords[1],
          event.button.button,
          window->on_mouse_down_context
        );
      }
    }
    else if (event.type == SDL_MOUSEBUTTONUP) {
      if (window->on_mouse_up_callback) {
        i32 transformed_coords[2];
        transform_mouse_coordinates(
          window,
          event.button.x,
          event.button.y,
          transformed_coords
        );
        window->on_mouse_up_callback(
          window,
          transformed_coords[0],
          transformed_coords[1],
          event.button.button,
          window->on_mouse_up_context
        );
      }
    }
    else if (event.type == SDL_MOUSEMOTION) {
      if (window->on_mouse_move_callback) {
        i32 transformed_coords[2];
        transform_mouse_coordinates(
          window,
          event.motion.x,
          event.motion.y,
          transformed_coords
        );
        window->on_mouse_move_callback(
          window,
          transformed_coords[0],
          transformed_coords[1],
          window->on_mouse_move_context
        );
      }
    }
    else if (event.type == SDL_MOUSEWHEEL) {
      if (window->on_mouse_scroll_callback) {
        window->on_mouse_scroll_callback(
          window,
          event.wheel.y,
          window->on_mouse_scroll_context
        );
      }
    }
  }
}

static inline Window* new_window(i32 width, i32 height, const char* title) {
  Window* window = (Window*)malloc(sizeof(Window));
  window->width = width;
  window->height = height;
  window->title = (char*)malloc(strlen(title) + 1);
  window->pixels = (u32*)malloc(width * height * sizeof(u32));
  strcpy(window->title, title);

  // Initialize callbacks to NULL
  window->on_close_callback = NULL;
  window->on_close_context = NULL;
  window->on_mouse_down_callback = NULL;
  window->on_mouse_down_context = NULL;
  window->on_mouse_up_callback = NULL;
  window->on_mouse_up_context = NULL;
  window->on_mouse_move_callback = NULL;
  window->on_mouse_move_context = NULL;
  window->on_mouse_scroll_callback = NULL;
  window->on_mouse_scroll_context = NULL;

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
    return NULL;
  }

  SDL_Window* sdl_window =
    SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      width, height, SDL_WINDOW_SHOWN);
  SDL_Renderer* renderer =
    SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED);
  SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
    SDL_TEXTUREACCESS_STREAMING,
    window->width, window->height);
  if (!sdl_window || !renderer || !texture) {
    fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(sdl_window);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(texture);
    window->sdl_window = NULL;
    return NULL;
  }
  window->sdl_window = sdl_window;
  window->renderer = renderer;
  window->texture = texture;
  return window;
}

static inline Window* paint_window(Window* window, Tela* tela) {
  if (!window || !window->sdl_window || !tela || !tela->image) {
    return window;
  }

  process_window_events(window);

  const u32 pixel_count = (u32)(window->width * window->height);
  for (u32 k = 0; k < pixel_count; k++) {
    u32 i = k / window->width;
    u32 j = k % window->width;
    f32 nx = (f32)j / (f32)window->width;
    f32 ny = (f32)i / (f32)window->height;
    u32 tx = (u32)(nx * (f32)tela->width);
    u32 ty = (u32)(ny * (f32)tela->height);
    const u32 channels = tela->channels;
    u32 tela_pixel = ty * tela->width + tx;
    u32 tela_index = tela_pixel * channels;

    u32 r = 0, g = 0, b = 0, a = 255;
    r = (u8)(tela->image[tela_index + 0] * 255.0f);
    g = (u8)(tela->image[tela_index + 1] * 255.0f);
    b = (u8)(tela->image[tela_index + 2] * 255.0f);
    a = (u8)(tela->image[tela_index + 3] * 255.0f);

    u32 window_index = i * window->width + j;
    window->pixels[window_index] = (r << 24) | (g << 16) | (b << 8) | a;
  }

  SDL_UpdateTexture(window->texture, NULL, window->pixels,
    window->width * sizeof(u32));
  SDL_RenderClear(window->renderer);
  SDL_RenderCopy(window->renderer, window->texture, NULL, NULL);
  SDL_RenderPresent(window->renderer);
  return window;
}

static inline Window* set_window_title(Window* window, const char* title) {
  if (!window || !window->sdl_window) {
    return window;
  }
  free(window->title);
  window->title = (char*)malloc(strlen(title) + 1);
  strcpy(window->title, title);
  SDL_SetWindowTitle(window->sdl_window, title);
  return window;
}

static inline Window* on_close_window(Window* window,
  void (*callback)(Window*, void*),
  void* context) {
  if (!window) {
    return window;
  }
  window->on_close_callback = callback;
  window->on_close_context = context;
  return window;
}

static inline Window* on_mouse_down_window(Window* window,
  void (*callback)(Window*, i32, i32,
    u32, void*),
  void* context) {
  if (!window) {
    return window;
  }
  window->on_mouse_down_callback = callback;
  window->on_mouse_down_context = context;
  return window;
}

static inline Window* on_mouse_up_window(Window* window,
  void (*callback)(Window*, i32, i32,
    u32, void*),
  void* context) {
  if (!window) {
    return window;
  }
  window->on_mouse_up_callback = callback;
  window->on_mouse_up_context = context;
  return window;
}

static inline Window* on_mouse_move_window(Window* window,
  void (*callback)(Window*, i32, i32,
    void*),
  void* context) {
  if (!window) {
    return window;
  }
  window->on_mouse_move_callback = callback;
  window->on_mouse_move_context = context;
  return window;
}

static inline Window*
on_mouse_scroll_window(Window* window, void (*callback)(Window*, i32, void*),
  void* context) {
  if (!window) {
    return window;
  }
  window->on_mouse_scroll_callback = callback;
  window->on_mouse_scroll_context = context;
  return window;
}

static inline void free_window(Window* window) {
  if (!window) {
    return;
  }
  if (window->texture) {
    SDL_DestroyTexture(window->texture);
  }
  if (window->renderer) {
    SDL_DestroyRenderer(window->renderer);
  }
  if (window->sdl_window) {
    SDL_DestroyWindow(window->sdl_window);
  }
  free(window->pixels);
  free(window->title);
  free(window);
  SDL_Quit();
}

//========================================================================================
/*                                                                                      *
 *                                       CAMERA_2D *
 *                                                                                      */
 //========================================================================================

typedef struct {
  AABB_2D view_box;
} Camera_2D;

//========================================================================================
/*                                                                                      *
 *                                          RAY *
 *                                                                                      */
 //========================================================================================
typedef struct {
  Vec3 init;
  Vec3 dir;
} Ray;

Ray ray(Vec3 init, Vec3 dir) { return (Ray) { init, dir }; }

Vec3 trace_ray(Ray ray, f32 t) {
  return add_vec3(ray.init, scale_vec3(ray.dir, t));
}

//========================================================================================
/*                                                                                      *
 *                                        CAMERA *
 *                                                                                      */
 //========================================================================================

typedef struct {
  Vec3 position;
  Vec3 look_at;
  f32 distance_to_plane;
  Vec3 orbit_coords;  // radius, theta, phi
  Vec2 orient_coords; // theta, phi
  Vec3 basis[3];      // matrix transforming camera space to world space
} Camera;

Camera set_orient_camera(Camera* camera, f32 theta, f32 phi) {
  camera->orient_coords = vec2(theta, phi);

  f32 cosT = cosf(theta);
  f32 sinT = sinf(theta);
  f32 cosP = cosf(phi);
  f32 sinP = sinf(phi);

  // right hand coordinate system
  // z-axis
  camera->basis[2] = vec3(-cosP * cosT, -cosP * sinT, -sinP);
  // y-axis
  camera->basis[1] = vec3(-sinP * cosT, -sinP * sinT, cosP);
  // x-axis
  camera->basis[0] = vec3(-sinT, cosT, 0.0f);
  return *camera;
}

Camera create_camera(Vec3 position, Vec3 look_at, f32 distance_to_plane) {
  Camera camera = { 0 };
  camera.position = position;
  camera.look_at = look_at;
  camera.distance_to_plane = distance_to_plane;

  camera.orbit_coords = vec3(length_vec3(sub_vec3(position, look_at)), 0, 0);

  // Initialize basis via orient
  set_orient_camera(&camera, 0, 0);

  return camera;
}

Camera set_orbit_camera(Camera* camera, f32 radius, f32 theta, f32 phi) {
  camera->orbit_coords = vec3(radius, theta, phi);
  set_orient_camera(camera, theta, phi);

  f32 cosT = cosf(theta);
  f32 sinT = sinf(theta);
  f32 cosP = cosf(phi);
  f32 sinP = sinf(phi);

  Vec3 sphere_coords =
    vec3(radius * cosP * cosT, radius * cosP * sinT, radius * sinP);

  camera->position = add_vec3(sphere_coords, camera->look_at);
  return *camera;
}

Vec3 get_camera_orbit(const Camera* camera) { return camera->orbit_coords; }

typedef struct {
  Camera* camera;
  Tela* tela;
  void* lambda_context;
  Color(*lambdaWithRays)(Ray, void*);
} LambdaRayContext;

Color lambda_tela_from_ray(u32 x, u32 y, void const* context) {
  const LambdaRayContext* lambda_context = (const LambdaRayContext*)context;
  Camera* camera = lambda_context->camera;
  Tela* tela = lambda_context->tela;
  f32 w = (f32)tela->width;
  f32 invW = 1.0f / w;
  f32 h = (f32)tela->height;
  f32 invH = 1.0f / h;
  Vec3 dirInLocal = { (x * invW - 0.5), (y * invH - 0.5),
                     camera->distance_to_plane };
  Vec3 dir = vec3(
    camera->basis[0].x * dirInLocal.x + camera->basis[1].x * dirInLocal.y +
    camera->basis[2].x * dirInLocal.z,
    camera->basis[0].y * dirInLocal.x + camera->basis[1].y * dirInLocal.y +
    camera->basis[2].y * dirInLocal.z,
    camera->basis[0].z * dirInLocal.x + camera->basis[1].z * dirInLocal.y +
    camera->basis[2].z * dirInLocal.z);
  Vec3 dir_norm;
  normalize_vec3(dir, &dir_norm);
  Color c = lambda_context->lambdaWithRays(ray(camera->position, dir_norm),
    lambda_context->lambda_context);
  return c;
}

Tela* ray_map_camera(Camera* camera, Tela* tela,
  Color(*ray_scene)(Ray, void*), void* context) {

  LambdaRayContext lambda_context = {
      .camera = camera,
      .tela = tela,
      .lambda_context = context,
      .lambdaWithRays = ray_scene,
  };
  return map_tela(tela, lambda_tela_from_ray, &lambda_context);
}

Vec3 to_local_coords_camera(const Camera* camera, Vec3 world_coords) {
  Vec3 p = sub_vec3(world_coords, camera->position);
  return vec3(
    dot_vec3(camera->basis[0], p),
    dot_vec3(camera->basis[1], p),
    dot_vec3(camera->basis[2], p)
  );
}

//========================================================================================
/*                                                                                      *
 *                                        RASTER                                        *
 *                                                                                      */
 //========================================================================================
typedef struct {
  bool cull_backfaces;
  bool bilinear_texture;
  bool clip_camera_plane;
  bool  clear_screen;
  Color  background_color;
  bool  perspective_correct;
  Camera* camera;
  Tela* tela;
} RasterParams;

typedef struct {
  Color colors[3];
  Vec2 tex_coords[3];
  Tela* texture;
} RasterTriangleProps;

typedef struct {
  Triangle* triangle;
  Camera* camera;
  Tela* tela;
  RasterParams* params;
  f32* zBuffer; // size will be tela->width * tela->height
} RasterTriangleInput;

const f32 dithering_matrix_4x4[16] = {
  0.0f / 16.0f, 8.0f / 16.0f, 2.0f / 16.0f, 10.0f / 16.0f, 12.0f / 16.0f,
  4.0f / 16.0f, 14.0f / 16.0f, 6.0f / 16.0f, 3.0f / 16.0f, 11.0f / 16.0f,
  1.0f / 16.0f, 9.0f / 16.0f, 15.0f / 16.0f, 7.0f / 16.0f, 13.0f / 16.0f,
};

Color get_bilinear_tex_color(Tela* texture, Vec2 uv) {
  Vec2 size = vec2((f32)texture->width, (f32)texture->height);
  Vec2 texInt = mul_vec2(uv, size);

  Vec2 texInt0 = map_vec2(texInt, floorf);
  Vec2 texInt1 = add_vec2(texInt0, vec2(1, 0));
  Vec2 texInt2 = add_vec2(texInt0, vec2(0, 1));
  Vec2 texInt3 = add_vec2(texInt0, vec2(1, 1));

  Color color0 = get_pxl_tela(texture, texInt0.x, texInt0.y);
  Color color1 = get_pxl_tela(texture, texInt1.x, texInt1.y);
  Color color2 = get_pxl_tela(texture, texInt2.x, texInt2.y);
  Color color3 = get_pxl_tela(texture, texInt3.x, texInt3.y);

  Vec2 x = sub_vec2(texInt, texInt0);
  Color bottomX = lerp_color(color0, color1, x.x);
  Color topX = lerp_color(color2, color3, x.x);
  return lerp_color(bottomX, topX, x.y);
}

Color get_tex_color(const Tela* texture, Vec2 uv) {
  return get_pxl_tela(texture, uv.x * texture->width, uv.y * texture->height);
}

typedef struct {
  Vec2 int_points[3];
  f32 inv_det;
  Vec3 points_in_cam_coords[3];
  Color c1, c2, c3;
  Vec2 tex_coords[3];
  Tela* texture;
  bool have_textures;
  f32* zBuffer; // size will be tela->width * tela->height
  u32 width;
  u32 height;
  Vec2 u;
  Vec2 v;
  RasterParams* params;
  Tela* tela;
} RasterTriangleShaderContext;

Color raster_triangle_shader(u32 x, u32 y, const Triangle_2D* triangle, void* ctx) {
  RasterTriangleShaderContext* context = (RasterTriangleShaderContext*)ctx;
  Vec2* int_points = context->int_points;
  f32 inv_det = context->inv_det;
  Vec3* points_in_cam_coords = context->points_in_cam_coords;
  Vec2 u = context->u;
  Vec2 v = context->v;
  Color c1 = context->c1;
  Color c2 = context->c2;
  Color c3 = context->c3;
  Vec2* tex_coords = context->tex_coords;
  Tela* texture = context->texture;
  bool have_textures = context->have_textures;
  f32* zBuffer = context->zBuffer;
  u32 w = context->width;
  u32 h = context->height;
  RasterParams* params = context->params;
  Tela* tela = context->tela;

  f32 W = 1;
  f32 wReciprocal = 1;

  Vec2 p = sub_vec2(vec2(x, y), int_points[0]);
  f32 alpha = -(v.x * p.y - v.y * p.x) * inv_det;
  f32 beta = (u.x * p.y - u.y * p.x) * inv_det;
  f32 gamma = 1 - alpha - beta;
  const f32 zs[3] = { points_in_cam_coords[0].z, points_in_cam_coords[1].z, points_in_cam_coords[2].z };
  if (params->perspective_correct) {
    // wReciprocal is the weight for perspective correction of z coordinate
    W = (1 / zs[0]) * gamma + (1 / zs[1]) * alpha + (1 / zs[2]) * beta;
    wReciprocal = 1 / W;
    alpha = (alpha / zs[1]) * wReciprocal;
    beta = (beta / zs[2]) * wReciprocal;
    gamma = (gamma / zs[0]) * wReciprocal;
  }
  else {
    wReciprocal = zs[0] * gamma + zs[1] * alpha + zs[2] * beta;
  }
  // compute color
  Color c = {
    c1.red * gamma + c2.red * alpha + c3.red * beta,
    c1.green * gamma + c2.green * alpha + c3.green * beta,
    c1.blue * gamma + c2.blue * alpha + c3.blue * beta,
    c1.alpha * gamma + c2.alpha * alpha + c3.alpha * beta,
  };
  if (have_textures) {
    Vec2 tex_uv = add_vec2(scale_vec2(tex_coords[0], gamma),
      add_vec2(scale_vec2(tex_coords[1], alpha),
        scale_vec2(tex_coords[2], beta)));
    Color tex_color = params->bilinear_texture ?
      get_bilinear_tex_color(texture, tex_uv) :
      get_tex_color(texture, tex_uv);
    c = tex_color;
  }
  const Vec2 ij = to_grid_tela(tela, x, y);
  const u32 zBufferIndex = (u32)floorf(w * ij.x + ij.y);
  if (wReciprocal < zBuffer[zBufferIndex]) {
    const f32 matrix_value = dithering_matrix_4x4[((u32)ij.x % 4) * 4 + ((u32)ij.y % 4)];
    const Color color = matrix_value < c.alpha ? c : (Color) { 0, 0, 0, 0 };
    if (color.alpha > 0) zBuffer[zBufferIndex] = wReciprocal; // if color.alpha is 0, don't update zBuffer
    return color;
  }
  return (Color) { 0, 0, 0, 0 };
}

void raster_triangle(RasterTriangleInput* input) {
  Triangle* triangle = input->triangle;
  Camera* camera = input->camera;
  Tela* tela = input->tela;
  RasterParams* params = input->params;
  f32* zBuffer = input->zBuffer;

  u32 w = tela->width;
  u32 h = tela->height;
  f32 distanceToPlane = camera->distance_to_plane;
  Vec3* positions = triangle->positions;

  RasterTriangleProps* props = (RasterTriangleProps*)triangle->props;
  Color* colors = props->colors;
  Vec2* tex_coords = props->tex_coords;
  Tela* texture = props->texture;
  // camera coords
  Vec3 points_in_cam_coords[3];
  for (u32 i = 0; i < 3; i++) {
    Vec3 p = positions[i];
    points_in_cam_coords[i] = to_local_coords_camera(camera, p);
  }
  // back face culling
  if (params->cull_backfaces) {
    Vec3 du = sub_vec3(points_in_cam_coords[1], points_in_cam_coords[0]);
    Vec3 dv = sub_vec3(points_in_cam_coords[2], points_in_cam_coords[0]);
    Vec3 n = cross_vec3(du, dv);
    normalize_vec3(n, &n);
    if (dot_vec3(n, points_in_cam_coords[0]) <= 0) return;
  }
  //frustum culling
  u32 inFrustum[3];
  u32 inFrustumCount = 0;
  u32 outFrustum[3];
  u32 outFrustumCount = 0;
  for (u32 i = 0; i < 3; i++) {
    if (points_in_cam_coords[i].z < distanceToPlane) {
      outFrustum[outFrustumCount++] = i;
    }
    else {
      inFrustum[inFrustumCount++] = i;
    }
  }
  if (params->clip_camera_plane && outFrustumCount >= 1) return;
  //project
  Vec3 projectedPoints[3];
  for (u32 i = 0; i < 3; i++) {
    projectedPoints[i] = scale_vec3(
      points_in_cam_coords[i],
      distanceToPlane / points_in_cam_coords[i].z
    );
  }
  // integer coordinates
  Vec2 intPoints[3];
  for (u32 i = 0; i < 3; i++) {
    intPoints[i] = vec2(
      (int)(w / 2 + projectedPoints[i].x * w),
      (int)(h / 2 + projectedPoints[i].y * h)
    );
  }
  // shader
  Vec2 u = sub_vec2(intPoints[1], intPoints[0]);
  Vec2 v = sub_vec2(intPoints[2], intPoints[0]);
  f32 det = wedge_vec2(u, v); // wedge product
  if (det == 0) return;
  f32 invDet = 1 / det;
  Color c1 = colors[0];
  Color c2 = colors[1];
  Color c3 = colors[2];
  bool haveTextures = texture != NULL && tex_coords != NULL;

  RasterTriangleProps* new_triangle_props = &(RasterTriangleProps) {
    .colors = { c1, c2, c3 },
      .tex_coords = { tex_coords[0], tex_coords[1], tex_coords[2] },
      .texture = (Tela*)texture
  };
  Triangle_2D* triangle_2d = &(Triangle_2D) {
    .positions = { intPoints[0], intPoints[1], intPoints[2] },
      .props = new_triangle_props
  };

  RasterTriangleShaderContext raster_triangle_shader_context = (RasterTriangleShaderContext){
    .int_points = { intPoints[0], intPoints[1], intPoints[2] },
      .inv_det = invDet,
      .points_in_cam_coords = { points_in_cam_coords[0], points_in_cam_coords[1], points_in_cam_coords[2] },
      .u = u,
      .v = v,
      .c1 = c1,
      .c2 = c2,
      .c3 = c3,
      .tex_coords = { tex_coords[0], tex_coords[1], tex_coords[2] },
      .texture = (Tela*)texture,
      .have_textures = haveTextures,
      .zBuffer = zBuffer,
      .width = w,
      .height = h,
      .params = params,
      .tela = tela
  };
  draw_triangle_tela(
    tela,
    triangle_2d,
    raster_triangle_shader,
    &raster_triangle_shader_context
  );
}

Tela* raster_scene(NaiveScene* scene, RasterParams params) {
  Tela* tela = params.tela;
  Camera* camera = params.camera;
  if (params.clear_screen) {
    fill_tela(tela, params.background_color);
  }
  u32 w = tela->width;
  u32 h = tela->height;
  u32 buffer_length = w * h;
  f32 z_buffer[buffer_length];
  for (u32 i = 0; i < buffer_length; i++) { z_buffer[i] = INFINITY; }
  for (u32 i = 0; i < scene->triangles.length; i++) {
    Triangle* triangle = (Triangle*)get_array_element(&scene->triangles, i);
    raster_triangle(&(RasterTriangleInput) {
      .triangle = triangle,
        .camera = camera,
        .tela = tela,
        .params = &params,
        .zBuffer = z_buffer
    });
  }
  return tela;
}


//========================================================================================
/*                                                                                      *
 *                                         MESH                                         *
 *                                                                                      */
 //========================================================================================
typedef struct {
  u32 vertex_indices[3];
  u32 tex_coord_indices[3];
  u32 normal_indices[3];
} Face;

typedef enum {
  MATERIAL_NONE,
  MATERIAL_DIFFUSE,
  MATERIAL_SPECULAR,
  MATERIAL_EMISSIVE,
} MaterialType;
typedef struct {
  MaterialType type;
  void* data;
} Material;

typedef struct {
  Array vertices;  // Vec3
  Array tex_coords; // Vec2
  Array normals; // Vec3
  Array colors; // Color
  Array faces; // Face
  Array materials; // Material
  Tela* texture;
  String name;
} Mesh;

Mesh read_obj_mesh(String obj_file, char* mesh_name) {
  Mesh mesh = { 0 };
  mesh.name = create_string(mesh_name);
  mesh.vertices = new_array(64, sizeof(Vec3));
  mesh.tex_coords = new_array(64, sizeof(Vec2));
  mesh.normals = new_array(64, sizeof(Vec3));
  mesh.faces = new_array(64, sizeof(Face));

  char* data = obj_file.data;
  u32 len = obj_file.length;
  u32 pos = 0;

  while (pos < len) {
    // Find line start and end
    u32 line_start = pos;
    while (pos < len && data[pos] != '\n' && data[pos] != '\r') pos++;
    u32 line_end = pos;
    // Skip newline characters
    while (pos < len && (data[pos] == '\n' || data[pos] == '\r')) pos++;

    // Skip empty lines
    if (line_end == line_start) continue;

    // Skip leading whitespace
    u32 i = line_start;
    while (i < line_end && (data[i] == ' ' || data[i] == '\t')) i++;
    if (i >= line_end) continue;

    // Parse line type
    if (data[i] == '#') {
      // Comment, skip
      continue;
    }

    if (data[i] == 'v' && i + 1 < line_end && data[i + 1] == 'n' &&
      (i + 2 >= line_end || data[i + 2] == ' ' || data[i + 2] == '\t')) {
      // vn - vertex normal (3 floats)
      i += 2;
      f32 nx = 0, ny = 0, nz = 0;
      sscanf(data + i, "%f %f %f", &nx, &ny, &nz);
      Vec3 n = vec3(nx, ny, nz);
      push_array(&mesh.normals, &n);
      continue;
    }

    if (data[i] == 'v' && i + 1 < line_end && data[i + 1] == 't' &&
      (i + 2 >= line_end || data[i + 2] == ' ' || data[i + 2] == '\t')) {
      // vt - texture coordinate (2 floats)
      i += 2;
      f32 u = 0, v = 0;
      sscanf(data + i, "%f %f", &u, &v);
      Vec2 tc = vec2(u, v);
      push_array(&mesh.tex_coords, &tc);
      continue;
    }

    if (data[i] == 'v' && (i + 1 >= line_end || data[i + 1] == ' ' || data[i + 1] == '\t')) {
      // v - vertex position (3 floats)
      i += 1;
      f32 vx = 0, vy = 0, vz = 0;
      sscanf(data + i, "%f %f %f", &vx, &vy, &vz);
      Vec3 v = vec3(vx, vy, vz);
      push_array(&mesh.vertices, &v);
      continue;
    }

    if (data[i] == 'f' && (i + 1 >= line_end || data[i + 1] == ' ' || data[i + 1] == '\t')) {
      // f - face: collect vertex tokens
      i += 1;

      // Parse face tokens (e.g. "1/2/3", "1//3", "1/2", "1")
      // Max polygon vertices we support for triangulation: 16
      u32 vert_idx[16], tex_idx[16], norm_idx[16];
      u32 face_vert_count = 0;

      while (i < line_end && face_vert_count < 16) {
        // Skip whitespace
        while (i < line_end && (data[i] == ' ' || data[i] == '\t')) i++;
        if (i >= line_end) break;

        // Parse vertex_index[/tex_index[/normal_index]]
        u32 vi = 0, ti = 0, ni = 0;

        // Parse vertex index
        while (i < line_end && data[i] >= '0' && data[i] <= '9') {
          vi = vi * 10 + (data[i] - '0');
          i++;
        }
        if (vi == 0) break; // invalid

        if (i < line_end && data[i] == '/') {
          i++; // skip '/'
          if (i < line_end && data[i] != '/' && data[i] >= '0' && data[i] <= '9') {
            while (i < line_end && data[i] >= '0' && data[i] <= '9') {
              ti = ti * 10 + (data[i] - '0');
              i++;
            }
          }
          if (i < line_end && data[i] == '/') {
            i++; // skip '/'
            if (i < line_end&& data[i] >= '0' && data[i] <= '9') {
              while (i < line_end&& data[i] >= '0' && data[i] <= '9') {
                ni = ni * 10 + (data[i] - '0');
                i++;
              }
            }
          }
        }

        vert_idx[face_vert_count] = vi - 1;  // OBJ is 1-indexed
        tex_idx[face_vert_count] = ti > 0 ? ti - 1 : 0;
        norm_idx[face_vert_count] = ni > 0 ? ni - 1 : 0;
        face_vert_count++;
      }

      // Triangulate: fan triangulation from vertex 0
      // triangle: (0,1,2), quad: (0,1,2),(0,2,3), n-gon: (0,k,k+1)
      for (u32 k = 1; k + 1 < face_vert_count; k++) {
        Face face;
        face.vertex_indices[0] = vert_idx[0];
        face.vertex_indices[1] = vert_idx[k];
        face.vertex_indices[2] = vert_idx[k + 1];
        face.tex_coord_indices[0] = tex_idx[0];
        face.tex_coord_indices[1] = tex_idx[k];
        face.tex_coord_indices[2] = tex_idx[k + 1];
        face.normal_indices[0] = norm_idx[0];
        face.normal_indices[1] = norm_idx[k];
        face.normal_indices[2] = norm_idx[k + 1];
        push_array(&mesh.faces, &face);
      }
      continue;
    }
  }
  return mesh;
}

AABB get_bounding_box_mesh(Mesh* mesh) {
  AABB box = { 0 };
  Array vertices = mesh->vertices;
  for (u32 i = 0; i < vertices.length; i++) {
    Vec3 vertex = *(Vec3*)get_array_element(&vertices, i);
    AABB vertex_box = build_aabb(vertex, vertex);
    box = union_aabb(&box, &vertex_box);
  }
  return box;
}

Mesh map_vertices_mesh(Mesh* mesh, Vec3(*mapper)(Vec3, void*), void* context) {
  for (u32 i = 0; i < mesh->vertices.length; i++) {
    Vec3* v = (Vec3*)get_array_element(&mesh->vertices, i);
    *v = mapper(*v, context);
  }
  return *mesh;
}

Mesh add_texture_mesh(Mesh* mesh, Tela* texture) {
  mesh->texture = texture;
  return *mesh;
}

Array get_triangles_mesh(Mesh* mesh) {
  Array triangles = new_array(mesh->faces.length, sizeof(Triangle));
  Color default_color = { 1.0f, 1.0f, 1.0f, 1.0f };

  for (u32 i = 0; i < mesh->faces.length; i++) {
    Face* face = (Face*)get_array_element(&mesh->faces, i);

    Triangle tri;
    // positions from vertex indices
    for (u32 j = 0; j < 3; j++) {
      Vec3* v = (Vec3*)get_array_element(&mesh->vertices, face->vertex_indices[j]);
      tri.positions[j] = v ? *v : vec3(0, 0, 0);
    }

    // Build props (colors, tex_coords, texture)
    RasterTriangleProps* props = (RasterTriangleProps*)malloc(sizeof(RasterTriangleProps));
    props->texture = mesh->texture;

    for (u32 j = 0; j < 3; j++) {
      // colors
      if (mesh->colors.length > 0) {
        Color* c = (Color*)get_array_element(&mesh->colors, face->vertex_indices[j]);
        props->colors[j] = c ? *c : default_color;
      }
      else {
        props->colors[j] = default_color;
      }
      // texture coordinates
      if (mesh->tex_coords.length > 0) {
        Vec2* tc = (Vec2*)get_array_element(&mesh->tex_coords, face->tex_coord_indices[j]);
        props->tex_coords[j] = tc ? *tc : vec2(0, 0);
      }
      else {
        props->tex_coords[j] = vec2(0, 0);
      }
    }

    tri.props = props;
    push_array(&triangles, &tri);
  }
  return triangles;
}


#endif /* TELA_C */
