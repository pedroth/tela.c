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

static inline Vec2 canvas2grid(Tela* tela, u32 x, u32 y) {
  const u32 h = tela->height;
  const u32 j = x;
  const u32 i = h - 1 - y;
  return vec2((f32)j, (f32)i);
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

static inline bool is_inside_convex(const Vec2* vertices, u32 vertex_count, Vec2 point) {
  const u32 m = vertex_count;
  Vec2 v[m];
  Vec2 n[m];
  for (u32 i = 0; i < m; i++) {
    const Vec2 p1 = vertices[(i + 1) % m];
    const Vec2 p0 = vertices[i];
    v[i] = sub_vec2(p1, p0);
    n[i] = (Vec2){ -v[i].y, v[i].x };
  }
  const int orientation = wedge_vec2(v[0], v[1]) >= 0 ? 1 : -1;
  for (u32 i = 0; i < m; i++) {
    const Vec2 r = sub_vec2(point, vertices[i]);
    const f32 myDot = dot_vec2(r, n[i]) * orientation;
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
  AABB_2D boundingBox = { 0 };
  for (u32 i = 0; i < 3; i++) {
    AABB_2D pointBox = build_aabb_from_vec2(triangle->positions[i]);
    boundingBox = union_aabb_2d(&boundingBox, &pointBox);
  }
  const AABB_2D finalBox = sub_aabb_2d(&canvasBox, &boundingBox);
  if (finalBox.is_empty) return tela;
  const Vec2 xmin = finalBox.min;
  const Vec2 xmax = finalBox.max;

  for (u32 x = xmin.x; x < xmax.x; x++) {
    for (u32 y = xmin.y; y < xmax.y; y++) {
      if (is_inside_convex(triangle->positions, 3, (Vec2) { x, y })) {
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
    fprintf(file, "%u %u %u ", r, g, b);
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

static inline char* io_read_file(const char* filename) {
  FILE* file = fopen(filename, "rb");
  if (!file) {
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  long length = ftell(file);
  fseek(file, 0, SEEK_SET);

  char* buffer = (char*)malloc(length + 1);
  if (!buffer) {
    fclose(file);
    return NULL;
  }

  fread(buffer, 1, length, file);
  buffer[length] = '\0';

  fclose(file);
  return buffer;
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
  const f32* zBuffer; // size will be tela->width * tela->height
} RasterTriangleInput;

Color raster_triangle_shader(u32 x, u32 y, const Triangle_2D* triangle, void* context) {
  //   let W = 1;
  // let wReciprocal = 1;
  // const p = Vec2(x, y).sub(intPoints[0]);
  // let alpha = -(v.x * p.y - v.y * p.x) * invDet;
  // let beta = (u.x * p.y - u.y * p.x) * invDet;
  // let gamma = 1 - alpha - beta;
  // const zs = pointsInCamCoord.map(p = > p.z);
  // if (params.perspectiveCorrect) {
  //   // wReciprocal is the weight for perspective correction of z coordinate
  //   W = (1 / zs[0]) * gamma + (1 / zs[1]) * alpha + (1 / zs[2]) * beta;
  //   wReciprocal = 1 / W;
  //   alpha = (alpha / zs[1]) * wReciprocal;
  //   beta = (beta / zs[2]) * wReciprocal;
  //   gamma = (gamma / zs[0]) * wReciprocal;
  // }
  // else {
  //   wReciprocal = zs[0] * gamma + zs[1] * alpha + zs[2] * beta;
  // }
  // // compute color
  // let c = Color.ofRGB(
  //   c1[0] * gamma + c2[0] * alpha + c3[0] * beta,
  //   c1[1] * gamma + c2[1] * alpha + c3[1] * beta,
  //   c1[2] * gamma + c2[2] * alpha + c3[2] * beta,
  //   c1[3] * gamma + c2[3] * alpha + c3[3] * beta,
  //   );
  // if (haveTextures) {
  //   const texUV = texCoords[0].scale(gamma)
  //     .add(texCoords[1].scale(alpha))
  //     .add(texCoords[2].scale(beta))
  //     const texColor = texture ?
  //     params.bilinearTexture ?
  //     getBiLinearTexColor(texUV, texture) :
  //     getTexColor(texUV, texture) :
  //     c ? c : getDefaultTexColor(texUV); // TODO: review this
  //   c = texColor;
  // }
  // const [i, j] = canvas.canvas2grid(x, y);
  // const zBufferIndex = Math.floor(w * i + j);
  // if (wReciprocal < zBuffer[zBufferIndex]) {
  //   const matrixValue = ditheringMatrix4x4[(i % 4) * 4 + (j % 4)];
  //   const color = matrixValue < c.alpha ? c : undefined;
  //   if (color) zBuffer[zBufferIndex] = wReciprocal; // if color is undefined, don't update zBuffer
  //   return color;
  // }
  return (Color) { 1.0f, 0.0f, 0.0f, 1.0f };
}

void raster_triangle(RasterTriangleInput* input) {
  Triangle* triangle = input->triangle;
  Camera* camera = input->camera;
  Tela* tela = input->tela;
  RasterParams* params = input->params;
  const f32* zBuffer = input->zBuffer;

  const u32 w = tela->width;
  const u32 h = tela->height;
  const f32 distanceToPlane = camera->distance_to_plane;
  const Vec3* positions = triangle->positions;

  RasterTriangleProps* props = (RasterTriangleProps*)triangle->props;
  const Color* colors = props->colors;
  const Vec2* tex_coords = props->tex_coords;
  const Tela* texture = props->texture;
  // camera coords
  Vec3 points_in_cam_coords[3];
  for (u32 i = 0; i < 3; i++) {
    Vec3 p = positions[i];
    points_in_cam_coords[i] = to_local_coords_camera(camera, p);
  }
  // back face culling
  if (params->cull_backfaces) {
    const Vec3 du = sub_vec3(points_in_cam_coords[1], points_in_cam_coords[0]);
    const Vec3 dv = sub_vec3(points_in_cam_coords[2], points_in_cam_coords[0]);
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
  const Vec2 u = sub_vec2(intPoints[1], intPoints[0]);
  const Vec2 v = sub_vec2(intPoints[2], intPoints[0]);
  const f32 det = wedge_vec2(u, v); // wedge product
  if (det == 0) return;
  const f32 invDet = 1 / det;
  const Color c1 = colors[0];
  const Color c2 = colors[1];
  const Color c3 = colors[2];
  const bool haveTextures = texture != NULL && tex_coords != NULL;

  RasterTriangleProps* new_triangle_props = &(RasterTriangleProps) {
    .colors = { c1, c2, c3 },
      .tex_coords = { tex_coords[0], tex_coords[1], tex_coords[2] },
      .texture = (Tela*)texture
  };
  const Triangle_2D* triangle_2d = &(Triangle_2D) {
    .positions = { intPoints[0], intPoints[1], intPoints[2] },
      .props = new_triangle_props
  };
  draw_triangle_tela(
    tela,
    triangle_2d,
    raster_triangle_shader,
    NULL
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




#endif /* TELA_C */
