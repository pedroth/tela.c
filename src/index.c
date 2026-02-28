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

 /* POSIX features for clock_gettime and rand_r - must be before includes */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
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

#ifdef _OPENMP
#include <omp.h>
#endif

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

static __thread unsigned int _tela_rand_seed = 0;
static inline void _tela_seed_thread(void) {
  if (_tela_rand_seed == 0) {
#ifdef _OPENMP
    _tela_rand_seed = (unsigned int)time(NULL) ^ ((unsigned int)omp_get_thread_num() * 2654435761u);
#else
    _tela_rand_seed = (unsigned int)time(NULL);
#endif
  }
}
static inline double random_double(void) {
  _tela_seed_thread();
  return (double)rand_r(&_tela_rand_seed) / RAND_MAX;
}

//========================================================================================
/*                                                                                      *
 *                                         MATH                                         *
 *                                                                                      */
 //========================================================================================

#ifndef PI
#define PI 3.14159265358979323846
#endif

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

static inline i32 max_i32(i32 a, i32 b) { return a > b ? a : b; }
static inline i32 min_i32(i32 a, i32 b) { return a < b ? a : b; }

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

Array map_array(Array* a, void (*func)(void* element, u32 index, void* ctx), void* ctx) {
  Array ans = new_array(a->capacity, a->element_size);
  for (u32 i = 0; i < a->length; i++) {
    void* element = (char*)a->data + (i * a->element_size);
    func(element, i, ctx);
    push_array(&ans, element);
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

void set_array_element(Array* a, u32 index, const void* element) {
  if (index >= a->length) return;
  char* destination = (char*)a->data + (index * a->element_size);
  memcpy(destination, element, a->element_size);
}

void swap_array_elements(Array* a, u32 i, u32 j) {
  if (i >= a->length || j >= a->length || i == j) return;
  char* ei = (char*)a->data + (i * a->element_size);
  char* ej = (char*)a->data + (j * a->element_size);
  // Use stack buffer for small elements, heap for large
  char buf[64];
  char* temp = (a->element_size <= sizeof(buf)) ? buf : (char*)malloc(a->element_size);
  memcpy(temp, ei, a->element_size);
  memcpy(ei, ej, a->element_size);
  memcpy(ej, temp, a->element_size);
  if (temp != buf) free(temp);
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

i32 arg_min_array(Array* a, f32(*cost_function)(void* element, u32 index, void* ctx), void* ctx) {
  i32 argmin_index = -1;
  f32 cost = __FLT_MAX__;
  for (u32 i = 0; i < a->length; i++) {
    f32 new_cost = cost_function(get_array_element(a, i), i, ctx);
    if (new_cost < cost) {
      cost = new_cost;
      argmin_index = (i32)i;
    }
  }
  return argmin_index;
}

//========================================================================================
/*                                                                                      *
 *                                        PQUEUE                                        *
 *                                                                                      */
 //========================================================================================

typedef struct {
  Array data; // Array of void* pointers
  f32(*comparator_function)(void* a, void* b, void* ctx);
  void* priority_ctx;
} PQueue;

u32 length_pqueue(PQueue* pq) {
  return pq->data.length;
}

void* peek_pqueue(PQueue* pq) {
  if (pq->data.length == 0) {
    return NULL;
  }
  return get_array_element(&pq->data, 0);
}


static void _heapify_pqueue(PQueue* pq, u32 root_index) {
  u32 left_index = 2 * root_index + 1;
  u32 right_index = 2 * root_index + 2;
  u32 min_index = root_index;

  if (left_index < pq->data.length) {
    void* left_val = *(void**)get_array_element(&pq->data, left_index);
    void* root_val = *(void**)get_array_element(&pq->data, root_index);
    if (pq->comparator_function(left_val, root_val, pq->priority_ctx) < 0) {
      min_index = left_index;
    }
  }
  if (right_index < pq->data.length) {
    void* right_val = *(void**)get_array_element(&pq->data, right_index);
    void* min_val = *(void**)get_array_element(&pq->data, min_index);
    if (pq->comparator_function(right_val, min_val, pq->priority_ctx) < 0) {
      min_index = right_index;
    }
  }
  if (min_index != root_index) {
    swap_array_elements(&pq->data, root_index, min_index);
    _heapify_pqueue(pq, min_index);
  }
}

void push_pqueue(PQueue* pq, void* element) {
  push_array(&pq->data, &element);
  if (pq->data.length <= 1) return;
  u32 i = pq->data.length - 1;
  while (i > 0) {
    u32 parent_index = (i % 2 != 0) ? i / 2 : i / 2 - 1;
    void* parent_val = *(void**)get_array_element(&pq->data, parent_index);
    void* current_val = *(void**)get_array_element(&pq->data, i);
    if (pq->comparator_function(parent_val, current_val, pq->priority_ctx) <= 0) break;
    swap_array_elements(&pq->data, parent_index, i);
    i = parent_index;
  }
}

void* pop_pqueue(PQueue* pq) {
  if (pq->data.length == 0) return NULL;
  void* result = *(void**)get_array_element(&pq->data, 0);
  if (pq->data.length <= 1) {
    pq->data.length = 0;
    return result;
  }
  // Move last element to front and shrink
  void* last = *(void**)get_array_element(&pq->data, pq->data.length - 1);
  set_array_element(&pq->data, 0, &last);
  pq->data.length--;
  _heapify_pqueue(pq, 0);
  return result;
}

PQueue* of_array_pqueue(Array* elements, f32(*comparator_function)(void* a, void* b, void* ctx), void* priority_ctx) {
  PQueue* pq = (PQueue*)malloc(sizeof(PQueue));
  pq->data = new_array(elements->length > 0 ? elements->length : 4, sizeof(void*));
  pq->comparator_function = comparator_function;
  pq->priority_ctx = priority_ctx;
  for (u32 i = 0; i < elements->length; i++) {
    void* element = *(void**)get_array_element(elements, i);
    push_pqueue(pq, element);
  }
  return pq;
}

//========================================================================================
/*                                                                                      *
 *                                         ANIMA                                        *
 *                                                                                      */
 //========================================================================================

 /**
  * A behavior is a timed action: a callback that runs for a given duration.
  * The callback receives (tau, dt, ctx) where tau is local time within
  * the behavior [0, duration].
  */
typedef struct {
  void (*behavior)(f32 tau, f32 dt, void* ctx);
  f32 duration;
} AnimaBehavior;

/**
 * An entry in the animation sequence, with precomputed start/end times.
 */
typedef struct {
  void (*behavior)(f32 tau, f32 dt, void* ctx);
  f32 duration;
  f32 start;
  f32 end;
} AnimaEntry;

/**
 * Anima — a sequencer that plays a list of behaviors in order.
 * Uses Array of AnimaEntry internally.
 */
typedef struct {
  Array entries; // Array of AnimaEntry
} Anima;

/**
 * Create a behavior (callback + duration pair).
 */
static inline AnimaBehavior anima_behavior(void (*lambda)(f32, f32, void*), f32 duration) {
  return (AnimaBehavior) { .behavior = lambda, .duration = duration };
}

/**
 * Create a wait (no-op behavior for a given duration).
 */
static inline AnimaBehavior anima_wait(f32 duration) {
  return (AnimaBehavior) { .behavior = NULL, .duration = duration };
}

/**
 * Build an Anima from an Array of AnimaBehavior.
 * Precomputes start/end times for each entry.
 */
static inline Anima new_anima(Array behaviors) {
  Anima a;
  a.entries = new_array(behaviors.length, sizeof(AnimaEntry));
  f32 acc = 0.0f;
  for (u32 i = 0; i < behaviors.length; i++) {
    AnimaBehavior* b = (AnimaBehavior*)get_array_element(&behaviors, i);
    AnimaEntry entry = {
      .behavior = b->behavior,
      .duration = b->duration,
      .start = acc,
      .end = acc + b->duration,
    };
    acc = entry.end;
    push_array(&a.entries, &entry);
  }
  return a;
}

/**
 * Total duration of the animation sequence.
 */
static inline f32 anima_time(const Anima* a) {
  if (a->entries.length == 0) return 0.0f;
  AnimaEntry* last = (AnimaEntry*)get_array_element((Array*)&a->entries, a->entries.length - 1);
  return last->end;
}

/**
 * Play the animation at global time t.
 * Finds the active behavior and calls it with local time tau.
 */
static inline void anima_play(const Anima* a, f32 t, f32 dt, void* ctx) {
  if (a->entries.length == 0) return;

  // Find which behavior is active at time t
  f32 s = 0.0f;
  u32 i = 0;
  while (s < t && i < a->entries.length) {
    AnimaEntry* e = (AnimaEntry*)get_array_element((Array*)&a->entries, i);
    s += e->duration;
    i++;
  }
  u32 index = (i > 0) ? i - 1 : 0;
  AnimaEntry* entry = (AnimaEntry*)get_array_element((Array*)&a->entries, index);

  // Compute local time within this behavior
  f32 tau = t - entry->start;
  if (i >= a->entries.length) {
    // Past the end — clamp to final behavior's duration
    tau = fminf(tau, entry->duration);
  }
  // Snap to exact end if within one dt of finishing
  if (fabsf(tau - entry->duration) < dt) {
    tau = entry->duration;
  }

  if (entry->behavior) {
    entry->behavior(tau, dt, ctx);
  }
}

/**
 * Play the animation in a loop (wraps time around total duration).
 */
static inline void anima_loop(const Anima* a, f32 t, f32 dt, void* ctx) {
  if (a->entries.length == 0) return;
  f32 max_t = anima_time(a);
  anima_play(a, fmodf(t, max_t), dt, ctx);
}

/**
 * Free the animation sequence.
 */
static inline void free_anima(Anima* a) {
  free_array(&a->entries);
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

static inline Vec3 random_vec3() {
  return vec3(random_double(), random_double(), random_double());
}
static inline Vec3 random_point_in_sphere() {
  Vec3 random_in_sphere;
  while (true) {
    Vec3 random = vec3(
      2.0 * random_double() - 1.0,
      2.0 * random_double() - 1.0,
      2.0 * random_double() - 1.0
    );
    if (length_vec3(random) >= 1) continue;
    normalize_vec3(random, &random_in_sphere);
    break;
  }
  return random_in_sphere;
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

static inline Color add_color(Color a, Color b) {
  return (Color) { a.red + b.red, a.green + b.green, a.blue + b.blue, a.alpha + b.alpha };
}

static inline Color scale_color(Color c, f32 s) {
  return (Color) { c.red* s, c.green* s, c.blue* s, c.alpha* s };
}

static inline Color mul_color(Color a, Color b) {
  return (Color) { a.red* b.red, a.green* b.green, a.blue* b.blue, a.alpha* b.alpha };
}

static inline Color gamma_color(Color c, f32 gamma) {
  return (Color) {
    powf(fmaxf(c.red, 0.0f), gamma),
      powf(fmaxf(c.green, 0.0f), gamma),
      powf(fmaxf(c.blue, 0.0f), gamma),
      c.alpha
  };
}

static const Color COLOR_BLACK = { 0.0f, 0.0f, 0.0f, 1.0f };
static const Color COLOR_WHITE = { 1.0f, 1.0f, 1.0f, 1.0f };
static const Color COLOR_RED = { 1.0f, 0.0f, 0.0f, 1.0f };
static const Color COLOR_GREEN = { 0.0f, 1.0f, 0.0f, 1.0f };
static const Color COLOR_BLUE = { 0.0f, 0.0f, 1.0f, 1.0f };
static const Color COLOR_YELLOW = { 1.0f, 1.0f, 0.0f, 1.0f };

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

//
static inline AABB_2D inter_aabb_2d(const AABB_2D* box, const AABB_2D* other) {
  if (box->is_empty || other->is_empty)
    return EMPTY_AABB_2D;
  Vec2 new_min = op_vec2(box->min, other->min, fmaxf);
  Vec2 new_max = op_vec2(box->max, other->max, fminf);
  const Vec2 new_diag = sub_vec2(new_max, new_min);
  const bool is_all_positive = new_diag.x >= 0 && new_diag.y >= 0;
  return is_all_positive ? build_aabb_2d(new_min, new_max) : EMPTY_AABB_2D;
}

static inline bool collides_aabb_2d(const AABB_2D* box, const AABB_2D* other) {
  return !inter_aabb_2d(box, other).is_empty;
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

static inline AABB inter_aabb(const AABB* box, const AABB* other) {
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
  return !inter_aabb(box, other).is_empty;
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

static inline f32 box_distance_aabb(AABB* a, AABB* b) {
  return length_vec3(sub_vec3(a->min, b->min)) + length_vec3(sub_vec3(a->max, b->max));
}

static inline Vec3 sample_aabb(const AABB* box) {
  Vec3 uvw = random_vec3();
  return vec3(
    lerp_f32(box->min.x, box->max.x, uvw.x),
    lerp_f32(box->min.y, box->max.y, uvw.y),
    lerp_f32(box->min.z, box->max.z, uvw.z)
  );
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
  f32 radius;
  void* props;
} Triangle;

Triangle build_triangle(Vec3 p1, Vec3 p2, Vec3 p3) {
  Triangle triangle;
  triangle.positions[0] = p1;
  triangle.positions[1] = p2;
  triangle.positions[2] = p3;
  triangle.radius = 0.0f;
  return triangle;
}

AABB get_bounding_box_triangle(Triangle* triangle) {
  AABB box;
  AABB b1 = build_aabb(triangle->positions[0], triangle->positions[0]);
  AABB b2 = build_aabb(triangle->positions[1], triangle->positions[1]);
  AABB b3 = build_aabb(triangle->positions[2], triangle->positions[2]);
  box = union_aabb(&b1, &b2);
  box = union_aabb(&box, &b3);
  return box;
}

Vec3 get_bary_coord_triangle(Triangle* triangle, Vec3 p) {
  Vec3* positions = triangle->positions;
  Vec3 tangents[2] = {
    sub_vec3(positions[1], positions[0]),
    sub_vec3(positions[2], positions[0])
  };
  f32 a = dot_vec3(tangents[0], tangents[0]);
  f32 b = dot_vec3(tangents[0], tangents[1]);
  f32 c = b;
  f32 d = dot_vec3(tangents[1], tangents[1]);
  f32 detInv = 1 / (a * d - b * c);

  Vec2 inv_u1 = vec2(d * detInv, -b * detInv);
  Vec2 inv_u2 = vec2(-c * detInv, a * detInv);

  Vec3 r = sub_vec3(p, positions[0]);
  Vec2 x = vec2(dot_vec3(tangents[0], r), dot_vec3(tangents[1], r));
  Vec2 alpha = vec2(dot_vec2(inv_u1, x), dot_vec2(inv_u2, x));
  f32 sum = alpha.x + alpha.y;
  return vec3(alpha.x, alpha.y, 1 - sum);
}

f32 distance_to_point_triangle(Triangle* triangle, Vec3 p) {
  Vec3 alpha = get_bary_coord_triangle(triangle, p);
  const f32 sum = alpha.x + alpha.y + alpha.z;

  Vec3 tangents[2] = {
    sub_vec3(triangle->positions[1], triangle->positions[0]),
    sub_vec3(triangle->positions[2], triangle->positions[0])
  };

  alpha = vec3(alpha.x / sum, alpha.y / sum, alpha.z / sum);
  Vec3 point_on_triangle = add_vec3(triangle->positions[0],
    add_vec3(
      scale_vec3(tangents[0], alpha.x),
      scale_vec3(tangents[1], alpha.y)
    )
  );
  return length_vec3(sub_vec3(p, point_on_triangle)) - triangle->radius;
}

Vec3 normal_to_point_triangle(Triangle* triangle, Vec3 p) {
  if (triangle->radius == 0.0f) {
    // Flat triangle: use cross product face normal with proper orientation
    Vec3 tangents[2] = {
      sub_vec3(triangle->positions[1], triangle->positions[0]),
      sub_vec3(triangle->positions[2], triangle->positions[0])
    };
    Vec3 normal = cross_vec3(tangents[0], tangents[1]);
    normalize_vec3(normal, &normal);
    Vec3 r = sub_vec3(p, triangle->positions[0]);
    f32 d = dot_vec3(normal, r);
    return d < 1e-3f ? normal : scale_vec3(normal, -1.0f);
  }
  else {
    // Rounded triangle: numerical gradient of distance function
    f32 epsilon = 1e-6f;
    f32 f = distance_to_point_triangle(triangle, p);
    f32 sign = (f > 0) - (f < 0);
    Vec3 grad = vec3(
      distance_to_point_triangle(triangle, add_vec3(p, vec3(epsilon, 0, 0))) - f,
      distance_to_point_triangle(triangle, add_vec3(p, vec3(0, epsilon, 0))) - f,
      distance_to_point_triangle(triangle, add_vec3(p, vec3(0, 0, epsilon))) - f
    );
    normalize_vec3(grad, &grad);
    return scale_vec3(grad, sign);
  }
}

//========================================================================================
/*                                                                                      *
 *                                        SPHERE                                        *
 *                                                                                      */
 //========================================================================================

typedef struct {
  Vec3 position;
  f32 radius;
  void* props;
} Sphere;

Sphere build_sphere(Vec3 position, f32 radius) {
  Sphere sphere;
  sphere.position = position;
  sphere.radius = radius;
  return sphere;
}

AABB get_bounding_box_sphere(Sphere* sphere) {
  Vec3 r_vec = vec3(sphere->radius, sphere->radius, sphere->radius);
  AABB box;
  box = build_aabb(sub_vec3(sphere->position, r_vec), add_vec3(sphere->position, r_vec));
  return box;
}

f32 distance_to_point_sphere(Sphere* sphere, Vec3 p) {
  return length_vec3(sub_vec3(p, sphere->position)) - sphere->radius;
}

Vec3 normal_to_point_sphere(Sphere* sphere, Vec3 p) {
  Vec3 r = sub_vec3(p, sphere->position);
  f32 len = length_vec3(r);
  normalize_vec3(r, &r);
  return len >= sphere->radius ? r : scale_vec3(r, -1.0f);
}

//========================================================================================
/*                                                                                      *
 *                                       GEOMETRY                                       *
 *                                                                                      */
 //========================================================================================

typedef enum {
  TRIANGLE,
  SPHERE,
  AABB_GEOMETRY
} GeometryType;

typedef struct {
  Vec3 init;
  Vec3 dir;
} Ray;

static inline Ray build_ray(Vec3 init, Vec3 dir) { return (Ray) { init, dir }; }

static inline Vec3 trace_ray(Ray ray, f32 t) {
  return add_vec3(ray.init, scale_vec3(ray.dir, t));
}

typedef struct {
  bool hit;
  f32 t;
  Vec3 position;
  Triangle* triangle;
  Sphere* sphere;
  AABB* aabb;
  GeometryType geometry_type;
} SceneHit;

SceneHit intersect_with_ray_aabb(Ray ray, const AABB* box) {
  SceneHit hit;
  hit.t = INFINITY;
  hit.hit = false;

  f32 epsilon = 1e-3f;
  f32 tmin = -INFINITY;
  f32 tmax = INFINITY;
  if (box->is_empty) return hit;
  // Pad AABB slightly to compensate for f32 precision loss in slab test
  f32 pad = 1e-4f;
  f32 min_array[3] = { box->min.x - pad, box->min.y - pad, box->min.z - pad };
  f32 max_array[3] = { box->max.x + pad, box->max.y + pad, box->max.z + pad };
  f32 r_init[3] = { ray.init.x, ray.init.y, ray.init.z };
  f32 dir_inv[3] = { 1.0f / ray.dir.x, 1.0f / ray.dir.y, 1.0f / ray.dir.z };
  const u32 dim = 3;
  for (u32 i = 0; i < dim; ++i) {
    f32 t1 = (min_array[i] - r_init[i]) * dir_inv[i];
    f32 t2 = (max_array[i] - r_init[i]) * dir_inv[i];

    tmin = fmaxf(tmin, fminf(t1, t2));
    tmax = fminf(tmax, fmaxf(t1, t2));
  }
  if (tmax >= fmaxf(tmin, 0)) {
    hit.t = tmin - epsilon;
    hit.hit = true;
    hit.position = trace_ray(ray, tmin - epsilon);
    hit.geometry_type = AABB_GEOMETRY;
    hit.aabb = (AABB*)box;
    return hit;
  }
  return hit;
}

SceneHit intersect_with_ray_triangle(Triangle* triangle, Ray ray) {
  SceneHit hit;
  hit.hit = false;
  hit.t = INFINITY;
  if (triangle->radius == 0.0f) {
    const f32 epsilon = 1e-12f;
    const Vec3 v = ray.dir;
    const Vec3 p = sub_vec3(ray.init, triangle->positions[0]);
    Vec3 tangents[2] = {
      sub_vec3(triangle->positions[1], triangle->positions[0]),
      sub_vec3(triangle->positions[2], triangle->positions[0])
    };
    Vec3 n = cross_vec3(tangents[0], tangents[1]);
    normalize_vec3(n, &n);
    const f32 t = -dot_vec3(n, p) / dot_vec3(n, v);
    if (t <= -epsilon) return hit;
    const Vec3 x = trace_ray(ray, t);
    for (u32 i = 0; i < 3; i++) {
      const Vec3 xi = triangle->positions[i];
      const Vec3 u = sub_vec3(x, xi);
      const Vec3 ni = cross_vec3(n, sub_vec3(triangle->positions[(i + 1) % 3], xi));
      const f32 dot = dot_vec3(ni, u);
      if (dot <= -epsilon) return hit;
    }
    hit.hit = true;
    hit.t = t - epsilon;   /* t value offset for ordering (matches JS) */
    hit.geometry_type = TRIANGLE;
    hit.triangle = triangle;
    hit.position = x;      /* position at exact surface point t, not t-epsilon */
    return hit;
  }
  const u32 max_ite = 20;
  const f32 epsilon = 1e-3;
  Vec3 p = ray.init;
  f32 t = distance_to_point_triangle(triangle, p);
  f32 minT = t;
  for (u32 i = 0; i < max_ite; i++) {
    p = trace_ray(ray, t);
    const f32 d = distance_to_point_triangle(triangle, p);
    t += d;
    if (d < epsilon) {
      hit.hit = true;
      hit.t = t;
      hit.geometry_type = TRIANGLE;
      hit.triangle = triangle;
      hit.position = p;
      return hit;
    }
    if (d > minT) {
      break;
    }
    minT = d;
  }
  return hit;
}

bool is_inside_triangle(Triangle* triangle, Vec3 p) {
  Vec3 tangents[2] = {
    sub_vec3(triangle->positions[1], triangle->positions[0]),
    sub_vec3(triangle->positions[2], triangle->positions[0])
  };
  Vec3 normal = cross_vec3(tangents[0], tangents[1]);
  if (!normalize_vec3(normal, &normal)) {
    return false;  // Degenerate triangle
  }
  return dot_vec3(normal, sub_vec3(p, triangle->positions[0])) > 0;
}

f32 intersect_sphere_aux(Sphere* sphere, Ray ray) {
  Vec3 init = ray.init;
  Vec3 dir = ray.dir;
  const Vec3 diff = sub_vec3(init, sphere->position);
  const f32 b = 2 * dot_vec3(dir, diff);
  const f32 c = dot_vec3(diff, diff) - sphere->radius * sphere->radius;
  const f32 discriminant = b * b - 4 * c; // a = 1
  if (discriminant < 0) return INFINITY;
  const f32 sqrt_disc = sqrtf(discriminant);
  const f32 t1 = (-b - sqrt_disc) / 2;
  const f32 t2 = (-b + sqrt_disc) / 2;
  const f32 t = fminf(t1, t2);
  const f32 tM = fmaxf(t1, t2);
  if (t1 * t2 < 0) return tM;
  return t1 >= 0 && t2 >= 0 ? t : INFINITY;
}

SceneHit intersect_with_ray_sphere(Sphere* sphere, Ray ray) {
  const f32 epsilon = 1e-4;
  SceneHit hit;
  hit.hit = false;
  hit.t = INFINITY;

  f32 t = intersect_sphere_aux(sphere, ray);
  if (t == INFINITY) return hit;

  hit.hit = true;
  hit.t = t;
  hit.geometry_type = SPHERE;
  hit.sphere = sphere;
  hit.position = trace_ray(ray, t - epsilon);
  return hit;
}

bool is_inside_sphere(Sphere* sphere, Vec3 p) {
  return distance_to_point_sphere(sphere, p) < 0;
}

//========================================================================================
/*                                                                                      *
 *                                        SCENE                                         *
 *                                                                                      */
 //========================================================================================

/* Forward declaration */
typedef struct Scene Scene;

/* Scene virtual table — every scene implementation provides these */
typedef struct {
  void     (*add_triangle)(Scene* self, Triangle triangle);
  void     (*add_sphere)(Scene* self, Sphere sphere);
  void     (*add_triangles)(Scene* self, Array triangles);
  void     (*add_spheres)(Scene* self, Array spheres);
  void     (*clear_triangles)(Scene* self);
  void     (*clear_spheres)(Scene* self);
  SceneHit(*intersect)(Scene* self, Ray ray);
  Array* (*get_triangles)(Scene* self);
  Array* (*get_spheres)(Scene* self);
  void     (*free_scene)(Scene* self);
  Scene* (*rebuild_scene)(Scene* self);
} SceneVTable;

/* Generic Scene — holds a vtable pointer and opaque data */
typedef struct Scene {
  const SceneVTable* vtable;
  void* data;
} Scene;

/* Generic Scene API */
static inline void add_triangle_scene(Scene* s, Triangle t) { s->vtable->add_triangle(s, t); }
static inline void add_sphere_scene(Scene* s, Sphere sp) { s->vtable->add_sphere(s, sp); }
static inline void add_triangles_scene(Scene* s, Array triangles) { s->vtable->add_triangles(s, triangles); }
static inline void add_spheres_scene(Scene* s, Array spheres) { s->vtable->add_spheres(s, spheres); }
static inline void clear_triangles_scene(Scene* s) { s->vtable->clear_triangles(s); }
static inline void clear_spheres_scene(Scene* s) { s->vtable->clear_spheres(s); }
static inline SceneHit intersect_scene(Scene* s, Ray ray) { return s->vtable->intersect(s, ray); }
static inline Array* get_triangles_scene(Scene* s) { return s->vtable->get_triangles(s); }
static inline Array* get_spheres_scene(Scene* s) { return s->vtable->get_spheres(s); }
static inline void free_scene(Scene* s) { s->vtable->free_scene(s); }

//========================================================================================
/*                                                                                      *
 *                                      NAIVE_SCENE                                     *
 *                                                                                      */
 //========================================================================================

typedef struct {
  Array triangles;
  Array spheres;
} NaiveScene;

NaiveScene add_triangle_naive_scene(NaiveScene* scene, Triangle triangle) {
  if (scene->triangles.element_size == 0) {
    scene->triangles = new_array(4, sizeof(Triangle));
  }
  push_array(&scene->triangles, &triangle);
  return *scene;
}

NaiveScene add_sphere_naive_scene(NaiveScene* scene, Sphere sphere) {
  if (scene->spheres.element_size == 0) {
    scene->spheres = new_array(4, sizeof(Sphere));
  }
  push_array(&scene->spheres, &sphere);
  return *scene;
}

NaiveScene add_spheres_naive_scene(NaiveScene* scene, Array spheres) {
  for (u32 i = 0; i < spheres.length; i++) {
    Sphere* sph = (Sphere*)get_array_element(&spheres, i);
    add_sphere_naive_scene(scene, *sph);
  }
  return *scene;
}

NaiveScene add_triangles_naive_scene(NaiveScene* scene, Array triangles) {
  for (u32 i = 0; i < triangles.length; i++) {
    Triangle* tri = (Triangle*)get_array_element(&triangles, i);
    add_triangle_naive_scene(scene, *tri);
  }
  return *scene;
}

NaiveScene clear_spheres_naive_scene(NaiveScene* scene) {
  clear_array(&scene->spheres);
  return *scene;
}


NaiveScene clear_triangles_naive_scene(NaiveScene* scene) {
  clear_array(&scene->triangles);
  return *scene;
}


SceneHit intersect_naive_scene(NaiveScene* scene, Ray ray) {
  SceneHit closest_hit;
  closest_hit.t = INFINITY;
  closest_hit.hit = false;

  for (u32 i = 0; i < scene->triangles.length; i++) {
    Triangle* tri = (Triangle*)get_array_element(&scene->triangles, i);
    SceneHit hit = intersect_with_ray_triangle(tri, ray);
    if (hit.t < closest_hit.t) {
      closest_hit = hit;
    }
  }

  for (u32 i = 0; i < scene->spheres.length; i++) {
    Sphere* sph = (Sphere*)get_array_element(&scene->spheres, i);
    SceneHit hit = intersect_with_ray_sphere(sph, ray);
    if (hit.t < closest_hit.t) {
      closest_hit = hit;
    }
  }

  return closest_hit;
}

/* --- NaiveScene vtable wrappers --- */
static void _naive_add_triangle(Scene* self, Triangle t) {
  add_triangle_naive_scene((NaiveScene*)self->data, t);
}
static void _naive_add_sphere(Scene* self, Sphere sp) {
  add_sphere_naive_scene((NaiveScene*)self->data, sp);
}
static void _naive_add_triangles(Scene* self, Array triangles) {
  add_triangles_naive_scene((NaiveScene*)self->data, triangles);
}
static void _naive_add_spheres(Scene* self, Array spheres) {
  add_spheres_naive_scene((NaiveScene*)self->data, spheres);
}
static void _naive_clear_triangles(Scene* self) {
  clear_triangles_naive_scene((NaiveScene*)self->data);
}
static void _naive_clear_spheres(Scene* self) {
  clear_spheres_naive_scene((NaiveScene*)self->data);
}
static SceneHit _naive_intersect(Scene* self, Ray ray) {
  return intersect_naive_scene((NaiveScene*)self->data, ray);
}
static Array* _naive_get_triangles(Scene* self) {
  return &((NaiveScene*)self->data)->triangles;
}
static Array* _naive_get_spheres(Scene* self) {
  return &((NaiveScene*)self->data)->spheres;
}
static void _naive_free_scene(Scene* self) {
  NaiveScene* ns = (NaiveScene*)self->data;
  if (ns->triangles.data) free_array(&ns->triangles);
  if (ns->spheres.data) free_array(&ns->spheres);
  free(ns);
  self->data = NULL;
}

static Scene* _naive_rebuild_scene(Scene* self) {
  // No-op for naive scene since it doesn't have an acceleration structure
  return self;
}

static const SceneVTable NAIVE_SCENE_VTABLE = {
  .add_triangle = _naive_add_triangle,
  .add_sphere = _naive_add_sphere,
  .add_triangles = _naive_add_triangles,
  .add_spheres = _naive_add_spheres,
  .clear_triangles = _naive_clear_triangles,
  .clear_spheres = _naive_clear_spheres,
  .intersect = _naive_intersect,
  .get_triangles = _naive_get_triangles,
  .get_spheres = _naive_get_spheres,
  .free_scene = _naive_free_scene,
  .rebuild_scene = _naive_rebuild_scene
};

Scene new_naive_scene(void) {
  NaiveScene* ns = (NaiveScene*)calloc(1, sizeof(NaiveScene));
  return (Scene) { .vtable = &NAIVE_SCENE_VTABLE, .data = ns };
}

//========================================================================================
/*                                                                                      *
 *                                        KSCENE                                        *
 *                                                                                      */
 //========================================================================================
typedef struct NodeKScene {
  AABB box;
  bool is_leaf;
  u32 num_of_primitives;
  union {
    struct {
      struct NodeKScene* left;
      struct NodeKScene* right;
    };
    struct {
      Array triangles; // only valid if is_leaf is true
      Array spheres;   // only valid if is_leaf is true
    };
  };
} NodeKScene;

/* --- NodeKScene helpers --- */

static NodeKScene* new_node_k_scene(u32 k) {
  NodeKScene* node = (NodeKScene*)calloc(1, sizeof(NodeKScene));
  node->box = EMPTY_AABB;
  node->is_leaf = true;
  node->num_of_primitives = 0;
  node->triangles = new_array(4, sizeof(Triangle));
  node->spheres = new_array(4, sizeof(Sphere));
  return node;
}

static NodeKScene* join_node_k_scene(NodeKScene* scene, NodeKScene* node_or_leaf) {
  NodeKScene* new_node = (NodeKScene*)calloc(1, sizeof(NodeKScene));
  new_node->is_leaf = false;
  new_node->left = scene;
  new_node->right = node_or_leaf;
  new_node->box = union_aabb(&scene->box, &node_or_leaf->box);
  new_node->num_of_primitives = scene->num_of_primitives + node_or_leaf->num_of_primitives;
  return new_node;
}

static void free_node_k_scene(NodeKScene* node) {
  if (!node) return;
  if (node->is_leaf) {
    if (node->triangles.data) free_array(&node->triangles);
    if (node->spheres.data) free_array(&node->spheres);
  }
  else {
    free_node_k_scene(node->left);
    free_node_k_scene(node->right);
  }
  free(node);
}

/**
 * Temporary struct for clustering primitives by their bounding box center.
 */
typedef struct {
  Vec3 center;
  bool is_triangle; // true = triangle, false = sphere
  u32 index;        // index into the leaf's triangles or spheres array
} PrimRef;

/**
 * 2-means clustering of primitives (port of JS clusterLeafs).
 * Splits prim_refs into two groups: group_a and group_b.
 */
static void cluster_prims(
  const AABB* box,
  const PrimRef* refs, u32 count,
  Array* group_a, Array* group_b
) {
  if (count == 0) return;

  const u32 ITERATIONS = 10;

  // Initialize two cluster centers by sampling the bounding box
  Vec3 centers[2] = { sample_aabb(box), sample_aabb(box) };

  // Temp arrays for cluster assignment indices
  Array cluster_indices[2];
  cluster_indices[0] = new_array(count, sizeof(u32));
  cluster_indices[1] = new_array(count, sizeof(u32));

  for (u32 iter = 0; iter < ITERATIONS; iter++) {
    clear_array(&cluster_indices[0]);
    clear_array(&cluster_indices[1]);

    // Assign each primitive to nearest cluster
    for (u32 j = 0; j < count; j++) {
      Vec3 pos = refs[j].center;
      Vec3 d0 = sub_vec3(centers[0], pos);
      Vec3 d1 = sub_vec3(centers[1], pos);
      f32 dist0 = dot_vec3(d0, d0);
      f32 dist1 = dot_vec3(d1, d1);
      u32 ki = (dist0 <= dist1) ? 0 : 1;
      push_array(&cluster_indices[ki], &j);
    }

    // Fix empty clusters
    for (u32 j = 0; j < 2; j++) {
      if (cluster_indices[j].length == 0) {
        u32 other = (j + 1) % 2;
        u32 rand_idx = (u32)(random_double() * cluster_indices[other].length);
        if (rand_idx >= cluster_indices[other].length) rand_idx = 0;
        u32* picked = (u32*)get_array_element(&cluster_indices[other], rand_idx);
        push_array(&cluster_indices[j], picked);
      }
    }

    // Update cluster centers
    for (u32 j = 0; j < 2; j++) {
      Vec3 acc = vec3(0, 0, 0);
      for (u32 m = 0; m < cluster_indices[j].length; m++) {
        u32* idx = (u32*)get_array_element(&cluster_indices[j], m);
        acc = add_vec3(acc, refs[*idx].center);
      }
      centers[j] = scale_vec3(acc, 1.0f / (f32)cluster_indices[j].length);
    }
  }

  // Output: copy PrimRefs into group_a and group_b
  for (u32 j = 0; j < 2; j++) {
    Array* target = (j == 0) ? group_a : group_b;
    for (u32 m = 0; m < cluster_indices[j].length; m++) {
      u32* idx = (u32*)get_array_element(&cluster_indices[j], m);
      push_array(target, &refs[*idx]);
    }
  }

  free_array(&cluster_indices[0]);
  free_array(&cluster_indices[1]);
}

/**
 * Split a leaf node that has exceeded k primitives.
 * Clusters all primitives into two groups using 2-means,
 * then creates left/right children.
 */
static void split_node_k_scene(NodeKScene* node, u32 k) {
  // Collect all primitives with their centers
  u32 total = node->num_of_primitives;
  PrimRef* refs = (PrimRef*)malloc(total * sizeof(PrimRef));
  u32 idx = 0;

  for (u32 i = 0; i < node->triangles.length; i++) {
    Triangle* tri = (Triangle*)get_array_element(&node->triangles, i);
    AABB tri_box = get_bounding_box_triangle(tri);
    refs[idx++] = (PrimRef){ .center = tri_box.center, .is_triangle = true, .index = i };
  }
  for (u32 i = 0; i < node->spheres.length; i++) {
    Sphere* sph = (Sphere*)get_array_element(&node->spheres, i);
    refs[idx++] = (PrimRef){ .center = sph->position, .is_triangle = false, .index = i };
  }

  // Cluster into two groups
  Array group_a = new_array(total, sizeof(PrimRef));
  Array group_b = new_array(total, sizeof(PrimRef));
  cluster_prims(&node->box, refs, total, &group_a, &group_b);

  // Create left and right children
  NodeKScene* left = new_node_k_scene(k);
  NodeKScene* right = new_node_k_scene(k);

  // Populate left
  for (u32 i = 0; i < group_a.length; i++) {
    PrimRef* ref = (PrimRef*)get_array_element(&group_a, i);
    if (ref->is_triangle) {
      Triangle* tri = (Triangle*)get_array_element(&node->triangles, ref->index);
      push_array(&left->triangles, tri);
      AABB tri_box = get_bounding_box_triangle(tri);
      left->box = union_aabb(&left->box, &tri_box);
    }
    else {
      Sphere* sph = (Sphere*)get_array_element(&node->spheres, ref->index);
      push_array(&left->spheres, sph);
      AABB sph_box = get_bounding_box_sphere(sph);
      left->box = union_aabb(&left->box, &sph_box);
    }
    left->num_of_primitives++;
  }

  // Populate right
  for (u32 i = 0; i < group_b.length; i++) {
    PrimRef* ref = (PrimRef*)get_array_element(&group_b, i);
    if (ref->is_triangle) {
      Triangle* tri = (Triangle*)get_array_element(&node->triangles, ref->index);
      push_array(&right->triangles, tri);
      AABB tri_box = get_bounding_box_triangle(tri);
      right->box = union_aabb(&right->box, &tri_box);
    }
    else {
      Sphere* sph = (Sphere*)get_array_element(&node->spheres, ref->index);
      push_array(&right->spheres, sph);
      AABB sph_box = get_bounding_box_sphere(sph);
      right->box = union_aabb(&right->box, &sph_box);
    }
    right->num_of_primitives++;
  }

  // Free old leaf arrays before overwriting the union
  if (node->triangles.data) free_array(&node->triangles);
  if (node->spheres.data) free_array(&node->spheres);

  // Convert to internal node
  node->is_leaf = false;
  node->left = left;
  node->right = right;

  free(refs);
  free_array(&group_a);
  free_array(&group_b);
}

/**
 * Add a triangle to a node (recursive, splits when leaf exceeds k).
 * Port of JS Node.add().
 */
static void add_triangle_node_k_scene(NodeKScene* node, Triangle triangle, u32 k) {
  node->num_of_primitives++;
  AABB tri_box = get_bounding_box_triangle(&triangle);
  node->box = union_aabb(&node->box, &tri_box);

  if (node->is_leaf) {
    push_array(&node->triangles, &triangle);

    if (node->num_of_primitives > k) {
      split_node_k_scene(node, k);
    }
  }
  else {
    // Insert into closer child
    f32 dist_left = box_distance_aabb(&node->left->box, &tri_box);
    f32 dist_right = box_distance_aabb(&node->right->box, &tri_box);
    if (dist_left <= dist_right) {
      add_triangle_node_k_scene(node->left, triangle, k);
    }
    else {
      add_triangle_node_k_scene(node->right, triangle, k);
    }
  }
}

/**
 * Add a sphere to a node (recursive, splits when leaf exceeds k).
 */
static void add_sphere_node_k_scene(NodeKScene* node, Sphere sphere, u32 k) {
  node->num_of_primitives++;
  AABB sph_box = get_bounding_box_sphere(&sphere);
  node->box = union_aabb(&node->box, &sph_box);

  if (node->is_leaf) {
    push_array(&node->spheres, &sphere);

    if (node->num_of_primitives > k) {
      split_node_k_scene(node, k);
    }
  }
  else {
    f32 dist_left = box_distance_aabb(&node->left->box, &sph_box);
    f32 dist_right = box_distance_aabb(&node->right->box, &sph_box);
    if (dist_left <= dist_right) {
      add_sphere_node_k_scene(node->left, sphere, k);
    }
    else {
      add_sphere_node_k_scene(node->right, sphere, k);
    }
  }
}

/**
 * Intersect ray with all primitives in a leaf node.
 */
static SceneHit leaf_intersect_node_k_scene(NodeKScene* node, Ray ray) {
  SceneHit closest = { .hit = false, .t = INFINITY };

  if (node->triangles.data) {
    for (u32 i = 0; i < node->triangles.length; i++) {
      Triangle* tri = (Triangle*)get_array_element(&node->triangles, i);
      SceneHit hit = intersect_with_ray_triangle(tri, ray);
      if (hit.hit && hit.t < closest.t) closest = hit;
    }
  }
  if (node->spheres.data) {
    for (u32 i = 0; i < node->spheres.length; i++) {
      Sphere* sph = (Sphere*)get_array_element(&node->spheres, i);
      SceneHit hit = intersect_with_ray_sphere(sph, ray);
      if (hit.hit && hit.t < closest.t) closest = hit;
    }
  }
  return closest;
}

/**
 * Intersect ray with the BVH (port of JS Node.interceptWithRay).
 * Traverses near child first, skips far child when possible.
 */
static SceneHit intersect_node_k_scene(NodeKScene* node, Ray ray) {
  if (!node) return (SceneHit) { .hit = false, .t = INFINITY };

  if (node->is_leaf) {
    return leaf_intersect_node_k_scene(node, ray);
  }

  SceneHit left_box_hit = intersect_with_ray_aabb(ray, &node->left->box);
  SceneHit right_box_hit = intersect_with_ray_aabb(ray, &node->right->box);

  if (!left_box_hit.hit && !right_box_hit.hit)
    return (SceneHit) { .hit = false, .t = INFINITY };

  NodeKScene* first = (left_box_hit.t <= right_box_hit.t) ? node->left : node->right;
  NodeKScene* second = (left_box_hit.t > right_box_hit.t) ? node->left : node->right;
  f32 second_t = fmaxf(left_box_hit.t, right_box_hit.t);

  SceneHit first_hit = intersect_node_k_scene(first, ray);

  // If first hit is closer than the second AABB entry, skip second
  if (first_hit.hit && first_hit.t < second_t) return first_hit;

  SceneHit second_hit = intersect_node_k_scene(second, ray);

  if (second_hit.hit && second_hit.t < (first_hit.hit ? first_hit.t : INFINITY))
    return second_hit;

  return first_hit;
}

/* --- KScene --- */

typedef struct {
  Array triangles;
  Array spheres;
  u32 k; // max number of primitives per leaf
  NodeKScene* root;
} KScene;

/* --- KScene implementation functions --- */

static void add_triangle_kscene(KScene* ks, Triangle triangle) {
  if (ks->triangles.element_size == 0) {
    ks->triangles = new_array(4, sizeof(Triangle));
  }
  push_array(&ks->triangles, &triangle);

  if (!ks->root) ks->root = new_node_k_scene(ks->k);
  add_triangle_node_k_scene(ks->root, triangle, ks->k);
}

static void add_sphere_kscene(KScene* ks, Sphere sphere) {
  if (ks->spheres.element_size == 0) {
    ks->spheres = new_array(4, sizeof(Sphere));
  }
  push_array(&ks->spheres, &sphere);

  if (!ks->root) ks->root = new_node_k_scene(ks->k);
  add_sphere_node_k_scene(ks->root, sphere, ks->k);
}

static void add_triangles_kscene(KScene* ks, Array triangles) {
  for (u32 i = 0; i < triangles.length; i++) {
    Triangle* tri = (Triangle*)get_array_element(&triangles, i);
    add_triangle_kscene(ks, *tri);
  }
}

static void add_spheres_kscene(KScene* ks, Array spheres) {
  for (u32 i = 0; i < spheres.length; i++) {
    Sphere* sph = (Sphere*)get_array_element(&spheres, i);
    add_sphere_kscene(ks, *sph);
  }
}

static void clear_triangles_kscene(KScene* ks) {
  if (ks->triangles.data) clear_array(&ks->triangles);
  if (ks->root) { free_node_k_scene(ks->root); ks->root = NULL; }
}

static void clear_spheres_kscene(KScene* ks) {
  if (ks->spheres.data) clear_array(&ks->spheres);
  if (ks->root) { free_node_k_scene(ks->root); ks->root = NULL; }
}

static SceneHit intersect_kscene(KScene* ks, Ray ray) {
  if (!ks->root) return (SceneHit) { .hit = false, .t = INFINITY };
  return intersect_node_k_scene(ks->root, ray);
}

static Array* get_triangles_kscene(KScene* ks) {
  return &ks->triangles;
}

static Array* get_spheres_kscene(KScene* ks) {
  return &ks->spheres;
}

static void free_kscene(KScene* ks) {
  if (ks->triangles.data) free_array(&ks->triangles);
  if (ks->spheres.data) free_array(&ks->spheres);
  if (ks->root) { free_node_k_scene(ks->root); ks->root = NULL; }
}

/**
 * Comparator for PQueue in rebuild: largest group first.
 * a, b are Array* pointers (groups of PrimRefs).
 */
static f32 _rebuild_group_comparator(void* a, void* b, void* ctx) {
  (void)ctx;
  Array* ga = (Array*)a;
  Array* gb = (Array*)b;
  return (f32)((i32)gb->length - (i32)ga->length);
}

/**
 * Check if any group in the PQueue exceeds k primitives.
 */
static bool _any_group_exceeds_k(PQueue* pq, u32 k) {
  for (u32 i = 0; i < pq->data.length; i++) {
    Array* group = *(Array**)get_array_element(&pq->data, i);
    if (group->length > k) return true;
  }
  return false;
}

/**
 * Compute bounding box from a group of PrimRefs referencing into ks.
 */
static AABB _compute_group_box(Array* group, KScene* ks) {
  AABB box = EMPTY_AABB;
  for (u32 i = 0; i < group->length; i++) {
    PrimRef* ref = (PrimRef*)get_array_element(group, i);
    if (ref->is_triangle) {
      Triangle* tri = (Triangle*)get_array_element(&ks->triangles, ref->index);
      AABB tri_box = get_bounding_box_triangle(tri);
      box = union_aabb(&box, &tri_box);
    }
    else {
      Sphere* sph = (Sphere*)get_array_element(&ks->spheres, ref->index);
      AABB sph_box = get_bounding_box_sphere(sph);
      box = union_aabb(&box, &sph_box);
    }
  }
  return box;
}

/**
 * Rebuild the KScene BVH from scratch for better quality.
 * Port of JS KScene.rebuild().
 *
 * Algorithm:
 *  1. Cluster all primitives into two groups using 2-means.
 *  2. Use a PQueue (max by group size) to iteratively split groups > k.
 *  3. Create a leaf NodeKScene for each final group.
 *  4. Bottom-up merge: repeatedly join the closest pair of nodes.
 */
static KScene rebuild_kscene(KScene* ks) {
  u32 total = ks->triangles.length + ks->spheres.length;
  if (total == 0) return *ks;

  // Free old tree
  if (ks->root) {
    free_node_k_scene(ks->root);
    ks->root = NULL;
  }

  // Step 1: Build PrimRef array and overall bounding box
  AABB overall_box = EMPTY_AABB;
  Array all_refs = new_array(total, sizeof(PrimRef));

  for (u32 i = 0; i < ks->triangles.length; i++) {
    Triangle* tri = (Triangle*)get_array_element(&ks->triangles, i);
    AABB tri_box = get_bounding_box_triangle(tri);
    overall_box = union_aabb(&overall_box, &tri_box);
    PrimRef ref = { .center = tri_box.center, .is_triangle = true, .index = i };
    push_array(&all_refs, &ref);
  }
  for (u32 i = 0; i < ks->spheres.length; i++) {
    Sphere* sph = (Sphere*)get_array_element(&ks->spheres, i);
    AABB sph_box = get_bounding_box_sphere(sph);
    overall_box = union_aabb(&overall_box, &sph_box);
    PrimRef ref = { .center = sph->position, .is_triangle = false, .index = i };
    push_array(&all_refs, &ref);
  }

  // Step 2: Initial clustering into two groups
  Array* group_a = (Array*)malloc(sizeof(Array));
  *group_a = new_array(total, sizeof(PrimRef));
  Array* group_b = (Array*)malloc(sizeof(Array));
  *group_b = new_array(total, sizeof(PrimRef));
  cluster_prims(&overall_box, (PrimRef*)all_refs.data, all_refs.length, group_a, group_b);

  // Step 3: PQueue sorted by group size (largest first via comparator)
  PQueue groups_queue;
  groups_queue.data = new_array(16, sizeof(void*));
  groups_queue.comparator_function = _rebuild_group_comparator;
  groups_queue.priority_ctx = NULL;
  push_pqueue(&groups_queue, group_a);
  push_pqueue(&groups_queue, group_b);

  // Step 4: Split groups exceeding k
  while (_any_group_exceeds_k(&groups_queue, ks->k)) {
    Array* top = (Array*)*(void**)peek_pqueue(&groups_queue);
    if (top->length > ks->k) {
      Array* group = (Array*)pop_pqueue(&groups_queue);
      AABB group_box = _compute_group_box(group, ks);

      Array* left_g = (Array*)malloc(sizeof(Array));
      *left_g = new_array(group->length, sizeof(PrimRef));
      Array* right_g = (Array*)malloc(sizeof(Array));
      *right_g = new_array(group->length, sizeof(PrimRef));
      cluster_prims(&group_box, (PrimRef*)group->data, group->length, left_g, right_g);

      push_pqueue(&groups_queue, left_g);
      push_pqueue(&groups_queue, right_g);

      free_array(group);
      free(group);
    }
  }

  // Step 5: Create a leaf NodeKScene for each group
  u32 num_groups = groups_queue.data.length;
  NodeKScene** node_stack = (NodeKScene**)malloc(num_groups * sizeof(NodeKScene*));
  u32 node_count = 0;

  for (u32 g = 0; g < groups_queue.data.length; g++) {
    Array* group = *(Array**)get_array_element(&groups_queue.data, g);
    NodeKScene* node = new_node_k_scene(ks->k);
    for (u32 i = 0; i < group->length; i++) {
      PrimRef* ref = (PrimRef*)get_array_element(group, i);
      if (ref->is_triangle) {
        Triangle* tri = (Triangle*)get_array_element(&ks->triangles, ref->index);
        push_array(&node->triangles, tri);
        AABB tri_box = get_bounding_box_triangle(tri);
        node->box = union_aabb(&node->box, &tri_box);
      }
      else {
        Sphere* sph = (Sphere*)get_array_element(&ks->spheres, ref->index);
        push_array(&node->spheres, sph);
        AABB sph_box = get_bounding_box_sphere(sph);
        node->box = union_aabb(&node->box, &sph_box);
      }
      node->num_of_primitives++;
    }
    node_stack[node_count++] = node;
    free_array(group);
    free(group);
  }

  free_array(&groups_queue.data);
  free_array(&all_refs);

  // Step 6: Bottom-up merge — repeatedly join closest pair
  while (node_count > 1) {
    NodeKScene* first = node_stack[0];
    // Shift array left (remove first element)
    for (u32 i = 0; i < node_count - 1; i++) {
      node_stack[i] = node_stack[i + 1];
    }
    node_count--;

    // Find the node closest to 'first'
    i32 min_index = 0;
    f32 min_dist = INFINITY;
    for (u32 i = 0; i < node_count; i++) {
      f32 dist = box_distance_aabb(&first->box, &node_stack[i]->box);
      if (dist < min_dist) {
        min_dist = dist;
        min_index = (i32)i;
      }
    }

    NodeKScene* nearest = node_stack[min_index];
    // Remove nearest from stack
    for (u32 i = (u32)min_index; i < node_count - 1; i++) {
      node_stack[i] = node_stack[i + 1];
    }
    node_count--;

    // Join and push back
    NodeKScene* joined = join_node_k_scene(first, nearest);
    node_stack[node_count++] = joined;
  }

  ks->root = (node_count > 0) ? node_stack[0] : NULL;
  free(node_stack);

  return *ks;
}

/* --- KScene vtable wrappers --- */

static void _kscene_add_triangle(Scene* self, Triangle t) {
  add_triangle_kscene((KScene*)self->data, t);
}
static void _kscene_add_sphere(Scene* self, Sphere sp) {
  add_sphere_kscene((KScene*)self->data, sp);
}
static void _kscene_add_triangles(Scene* self, Array triangles) {
  add_triangles_kscene((KScene*)self->data, triangles);
}
static void _kscene_add_spheres(Scene* self, Array spheres) {
  add_spheres_kscene((KScene*)self->data, spheres);
}
static void _kscene_clear_triangles(Scene* self) {
  clear_triangles_kscene((KScene*)self->data);
}
static void _kscene_clear_spheres(Scene* self) {
  clear_spheres_kscene((KScene*)self->data);
}
static SceneHit _kscene_intersect(Scene* self, Ray ray) {
  return intersect_kscene((KScene*)self->data, ray);
}
static Array* _kscene_get_triangles(Scene* self) {
  return get_triangles_kscene((KScene*)self->data);
}
static Array* _kscene_get_spheres(Scene* self) {
  return get_spheres_kscene((KScene*)self->data);
}
static void _kscene_free_scene(Scene* self) {
  KScene* ks = (KScene*)self->data;
  free_kscene(ks);
  free(ks);
  self->data = NULL;
}

static Scene* _kscene_rebuild_scene(Scene* self) {
  KScene* ks = (KScene*)self->data;
  rebuild_kscene(ks);
  return self;
}

static const SceneVTable KSCENE_VTABLE = {
  .add_triangle = _kscene_add_triangle,
  .add_sphere = _kscene_add_sphere,
  .add_triangles = _kscene_add_triangles,
  .add_spheres = _kscene_add_spheres,
  .clear_triangles = _kscene_clear_triangles,
  .clear_spheres = _kscene_clear_spheres,
  .intersect = _kscene_intersect,
  .get_triangles = _kscene_get_triangles,
  .get_spheres = _kscene_get_spheres,
  .free_scene = _kscene_free_scene,
  .rebuild_scene = _kscene_rebuild_scene,
};

Scene new_kscene(u32 k) {
  if (k == 0) {
    k = 10; // default value
  }
  KScene* ks = (KScene*)calloc(1, sizeof(KScene));
  ks->k = k;
  return (Scene) { .vtable = &KSCENE_VTABLE, .data = ks };
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
  f32* image; // RGBA format, row-major order width * height * channels in size
  AABB_2D box;
  u32 iterations; // used for exposed tela
} Tela;

static inline Tela* new_tela(u32 width, u32 height) {
  Tela* tela = (Tela*)malloc(sizeof(Tela));
  tela->width = width;
  tela->height = height;
  tela->channels = COLOR_CHANNELS;
  tela->box = build_aabb_2d(vec2(0.0f, 0.0f), vec2((f32)width, (f32)height));
  tela->image = (f32*)calloc(width * height * COLOR_CHANNELS, sizeof(f32));
  tela->iterations = 1;
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

static inline Tela* map_tela_parallel(Tela* tela,
  Color(*lambda)(u32, u32, void const*),
  void const* context) {
  const u32 w = tela->width;
  const u32 h = tela->height;
  const u32 c = tela->channels;
  const u32 size = w * h * c;
#pragma omp parallel for schedule(dynamic, 64)
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

static inline Tela* set_pxl_tela(Tela* tela, u32 x, u32 y, Color color) {
  const u32 w = tela->width;
  const u32 h = tela->height;
  Vec2 grid = to_grid_tela(tela, x, y);
  u32 i = (u32)grid.x;
  u32 j = (u32)grid.y;
  i = mod_u32(i, h);
  j = mod_u32(j, w);
  u32 index = COLOR_CHANNELS * (w * i + j);
  tela->image[index] = color.red;
  tela->image[index + 1] = color.green;
  tela->image[index + 2] = color.blue;
  tela->image[index + 3] = color.alpha;
  return tela;
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
  const AABB_2D finalBox = inter_aabb_2d(&canvasBox, &boundingBox);
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
      (Vec2) {
-e0.y, e0.x
},
(Vec2) {
-e1.y, e1.x
},
(Vec2) {
-e2.y, e2.x
},
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
 *                                     EXPOSED_TELA                                     *
 *                                                                                      */
 //========================================================================================

Tela* fill_exposed_tela(Tela* exposed, Color color) {
  const u32 n = exposed->width * exposed->height * COLOR_CHANNELS;
  if (color.alpha == 0) return exposed;
  f32* img = exposed->image;
  u32 it = exposed->iterations;
  for (u32 k = 0; k < n; k += 4) {
    img[k] = img[k] + (color.red - img[k]) / it;
    img[k + 1] = img[k + 1] + (color.green - img[k + 1]) / it;
    img[k + 2] = img[k + 2] + (color.blue - img[k + 2]) / it;
    img[k + 3] = img[k + 3] + (color.alpha - img[k + 3]) / it;
  }
  if (exposed->iterations < UINT32_MAX) exposed->iterations++;
  return exposed;
}

Tela* map_exposed_tela(Tela* exposed, Color(*lambda)(u32, u32, void const*), void const* context) {
  const u32 n = exposed->width * exposed->height * COLOR_CHANNELS;
  const u32 w = exposed->width;
  const u32 h = exposed->height;

  f32* img = exposed->image;
  u32 it = exposed->iterations;
  for (u32 k = 0; k < n; k += 4) {
    const u32 i = k / (4 * w);
    const u32 j = (k / 4) % w;
    const u32 x = j;
    const u32 y = h - 1 - i;
    const Color color = lambda(x, y, context);
    if (color.alpha == 0.0f) continue;
    if (isnan(color.red) || isnan(color.green) || isnan(color.blue)) continue;
    img[k] = img[k] + (color.red - img[k]) / it;
    img[k + 1] = img[k + 1] + (color.green - img[k + 1]) / it;
    img[k + 2] = img[k + 2] + (color.blue - img[k + 2]) / it;
    img[k + 3] = img[k + 3] + (color.alpha - img[k + 3]) / it;
  }
  if (exposed->iterations < UINT32_MAX) exposed->iterations++;
  return exposed;
}

Tela* map_exposed_tela_parallel(Tela* exposed, Color(*lambda)(u32, u32, void const*), void const* context) {
  const u32 w = exposed->width;
  const u32 h = exposed->height;
  const u32 total_pixels = w * h;
  f32* img = exposed->image;
  u32 it = exposed->iterations;

#pragma omp parallel for schedule(dynamic, 64)
  for (u32 p = 0; p < total_pixels; p++) {
    const u32 i = p / w;
    const u32 j = p % w;
    const u32 x = j;
    const u32 y = h - 1 - i;
    const Color color = lambda(x, y, context);
    if (color.alpha == 0.0f) continue;
    if (isnan(color.red) || isnan(color.green) || isnan(color.blue)) continue;
    const u32 k = p * 4;
    img[k] = img[k] + (color.red - img[k]) / it;
    img[k + 1] = img[k + 1] + (color.green - img[k + 1]) / it;
    img[k + 2] = img[k + 2] + (color.blue - img[k + 2]) / it;
    img[k + 3] = img[k + 3] + (color.alpha - img[k + 3]) / it;
  }
  if (exposed->iterations < UINT32_MAX) exposed->iterations++;
  return exposed;
}

Tela* set_pxl_exposed_tela(Tela* exposed, u32 x, u32 y, Color color) {
  const u32 w = exposed->width;
  f32* img = exposed->image;
  u32 it = exposed->iterations;
  const Vec2 ij = to_grid_tela(exposed, x, y);
  u32 i = (u32)ij.x;
  u32 j = (u32)ij.y;
  u32 index = 4 * (w * i + j);
  img[index] = img[index] + (color.red - img[index]) / it;
  img[index + 1] = img[index + 1] + (color.green - img[index + 1]) / it;
  img[index + 2] = img[index + 2] + (color.blue - img[index + 2]) / it;
  img[index + 3] = img[index + 3] + (color.alpha - img[index + 3]) / it;
  if (it < UINT32_MAX) it++;
  return exposed;
}

Color get_pxl_exposed_tela(const Tela* exposed, u32 x, u32 y) {
  const u32 w = exposed->width;
  const u32 h = exposed->height;
  Vec2 grid = to_grid_tela(exposed, x, y);
  u32 i = (u32)grid.x;
  u32 j = (u32)grid.y;
  i = mod_u32(i, h);
  j = mod_u32(j, w);
  u32 index = COLOR_CHANNELS * (w * i + j);
  return (Color) {
    exposed->image[index],
      exposed->image[index + 1],
      exposed->image[index + 2],
      exposed->image[index + 3]
  };
};

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
    }
    else if (strncmp(data + line_start, "HEIGHT ", 7) == 0) {
      sscanf(data + line_start + 7, "%u", &height);
    }
    else if (strncmp(data + line_start, "DEPTH ", 6) == 0) {
      sscanf(data + line_start + 6, "%u", &depth);
    }
    else if (strncmp(data + line_start, "MAXVAL ", 7) == 0) {
      sscanf(data + line_start + 7, "%u", &max_val);
    }
    else if (strncmp(data + line_start, "ENDHDR", 6) == 0) {
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

  void (*on_key_down_callback)(Window* window, u32 keycode, void* context);
  void* on_key_down_context;

  void (*on_key_up_callback)(Window* window, u32 keycode, void* context);
  void* on_key_up_context;
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
          -event.wheel.y,
          window->on_mouse_scroll_context
        );
      }
    }
    else if (event.type == SDL_KEYDOWN) {
      if (window->on_key_down_callback) {
        window->on_key_down_callback(
          window,
          event.key.keysym.sym,
          window->on_key_down_context
        );
      }
    }
    else if (event.type == SDL_KEYUP) {
      if (window->on_key_up_callback) {
        window->on_key_up_callback(
          window,
          event.key.keysym.sym,
          window->on_key_up_context
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
  window->on_key_down_callback = NULL;
  window->on_key_down_context = NULL;
  window->on_key_up_callback = NULL;
  window->on_key_up_context = NULL;

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
    r = (u8)(clamp(tela->image[tela_index + 0], 0.0f, 1.0f) * 255.0f);
    g = (u8)(clamp(tela->image[tela_index + 1], 0.0f, 1.0f) * 255.0f);
    b = (u8)(clamp(tela->image[tela_index + 2], 0.0f, 1.0f) * 255.0f);
    a = (u8)(clamp(tela->image[tela_index + 3], 0.0f, 1.0f) * 255.0f);

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

static inline Window*
on_key_down_window(Window* window, void (*callback)(Window*, u32, void*),
  void* context) {
  if (!window) {
    return window;
  }
  window->on_key_down_callback = callback;
  window->on_key_down_context = context;
  return window;
}

static inline Window*
on_key_up_window(Window* window, void (*callback)(Window*, u32, void*),
  void* context) {
  if (!window) {
    return window;
  }
  window->on_key_up_callback = callback;
  window->on_key_up_context = context;
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

Vec2 get_camera_orient(const Camera* camera) { return camera->orient_coords; }

static inline Vec3 to_world_coord_camera(const Camera* camera, Vec3 cam_vec) {
  Vec3 x = vec3(0, 0, 0);
  f32 components[] = { cam_vec.x, cam_vec.y, cam_vec.z };
  for (i32 i = 0; i < 3; i++) {
    x = add_vec3(x, scale_vec3(camera->basis[i], components[i]));
  }
  return x;
}

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
  Color c = lambda_context->lambdaWithRays(build_ray(camera->position, dir_norm),
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

Tela* ray_map_camera_parallel(Camera* camera, Tela* tela,
  Color(*ray_scene)(Ray, void*), void* context) {

  LambdaRayContext lambda_context = {
      .camera = camera,
      .tela = tela,
      .lambda_context = context,
      .lambdaWithRays = ray_scene,
  };
  return map_tela_parallel(tela, lambda_tela_from_ray, &lambda_context);
}

Vec3 to_local_coords_camera(const Camera* camera, Vec3 world_coords) {
  Vec3 p = sub_vec3(world_coords, camera->position);
  return vec3(
    dot_vec3(camera->basis[0], p),
    dot_vec3(camera->basis[1], p),
    dot_vec3(camera->basis[2], p)
  );
}

Ray ray_from_tela_camera(const Camera* camera, const Tela* tela, u32 x, u32 y) {
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
  return build_ray(camera->position, dir_norm);
};

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

typedef enum {
  METALLIC,
  DIFFUSE,
  EMISSIVE,
  ALPHA,
  DIELECTRIC,
} MaterialType;

typedef struct {
  bool emissive;
  Ray(*scatter)(Ray, SceneHit);
  void* data; // data of materials
} Material;

typedef struct {
  Color colors[3];
  Vec2 tex_coords[3];
  Tela* texture;
  Material* material;
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
    if (points_in_cam_coords[i].z < 0 ) {
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
  if (fabsf(det) < 1e-6f) return;  // Degenerate triangle in screen space
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

typedef struct {
  Color color;
  Vec2 tex_coord;
  Tela* texture;
  Material* material;
} RasterSphereProps;

typedef struct {
  Sphere* sphere;
  Camera* camera;
  Tela* tela;
  RasterParams* params;
  f32* zBuffer; // size will be tela->width * tela->height
} RasterSphereInput;

void raster_sphere(RasterSphereInput* input) {
  Sphere* sphere = input->sphere;
  Camera* camera = input->camera;
  Tela* tela = input->tela;
  f32* zBuffer = input->zBuffer;

  const u32 w = tela->width;
  const u32 h = tela->height;
  const f32 distance_to_plane = camera->distance_to_plane;

  RasterSphereProps* props = (RasterSphereProps*)sphere->props;
  Color color = props ? props->color : (Color) { 1.0f, 1.0f, 1.0f, 1.0f };
  Vec2 tex_coord = props ? props->tex_coord : vec2(0, 0);
  Tela* texture = props ? props->texture : NULL;

  // camera coords
  Vec3 point_in_cam = to_local_coords_camera(camera, sphere->position);

  // frustum culling
  f32 z = point_in_cam.z;
  if (z < distance_to_plane) return;

  // project
  f32 proj_scale = distance_to_plane / z;
  Vec3 projected = scale_vec3(point_in_cam, proj_scale);

  // screen coords
  i32 x = (i32)floorf((f32)w / 2.0f + projected.x * (f32)w);
  i32 y = (i32)floorf((f32)h / 2.0f + projected.y * (f32)h);

  if (x < 0 || x >= (i32)w || y < 0 || y >= (i32)h) return;

  // projected radius in pixels
  i32 int_radius = (i32)ceilf(sphere->radius * proj_scale * (f32)w);
  i32 int_radius_sq = int_radius * int_radius;

  // final color (blend with texture if available)
  Color final_color = color;
  if (texture && (tex_coord.x != 0 || tex_coord.y != 0)) {
    Color tex_color = get_tex_color(texture, tex_coord);
    final_color = scale_color(add_color(final_color, tex_color), 0.5f);
  }

  // rasterize the projected disc
  for (i32 l = -int_radius; l < int_radius; l++) {
    for (i32 k = -int_radius; k < int_radius; k++) {
      i32 sq_len = k * k + l * l;
      if (sq_len > int_radius_sq) continue;

      i32 xl = max_i32(0, min_i32((i32)w - 1, x + k));
      i32 yl = (i32)floorf((f32)(y + l));
      if (yl < 0 || yl >= (i32)h) continue;

      Vec2 grid = to_grid_tela(tela, (u32)xl, (u32)yl);
      u32 gi = (u32)grid.x;
      u32 gj = (u32)grid.y;
      u32 z_index = w * gi + gj;

      if (z < zBuffer[z_index]) {
        zBuffer[z_index] = z;
        set_pxl_tela(tela, (u32)xl, (u32)yl, final_color);
      }
    }
  }
}


Tela* raster_scene(Scene* scene, RasterParams params) {
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
  Array* triangles = get_triangles_scene(scene);
  for (u32 i = 0; i < triangles->length; i++) {
    Triangle* triangle = (Triangle*)get_array_element(triangles, i);
    raster_triangle(&(RasterTriangleInput) {
      .triangle = triangle,
        .camera = camera,
        .tela = tela,
        .params = &params,
        .zBuffer = z_buffer
    });
  }
  Array* spheres = get_spheres_scene(scene);
  for (u32 i = 0; i < spheres->length; i++) {
    Sphere* sphere = (Sphere*)get_array_element(spheres, i);
    raster_sphere(&(RasterSphereInput) {
      .sphere = sphere,
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
 *                                       RAY_TRACE                                       *
 *                                                                                      */
 //========================================================================================

typedef struct {
  Vec3 direction;
  f32 sharpness;
}DirectionalLightParams;

typedef struct {
  u32 samples_per_pixel;
  u32 bounces;
  f32 variance;
  f32 gamma;
  bool bilinear_texture;
  bool is_biased;
  Color(*render_background)(Ray, void*);
  void* render_background_context;
  Tela* exposed_tela;
  Camera* camera;
  DirectionalLightParams* directional_light;
} RaytraceParams;

typedef struct {
  Scene* scene;
  RaytraceParams* params;
} RayTraceLambdaInput;

Ray scatter_diffuse(Ray ray, SceneHit hit) {
  Vec3 normal = { 0, 0, 0 };
  if (hit.geometry_type == TRIANGLE) {
    normal = normal_to_point_triangle(hit.triangle, hit.position);
  }
  else if (hit.geometry_type == SPHERE) {
    normal = normal_to_point_sphere(hit.sphere, hit.position);
  }
  const Vec3 randomInSphere = random_point_in_sphere();
  // Offset origin slightly along normal to prevent f32 self-intersection
  const Vec3 origin = add_vec3(hit.position, scale_vec3(normal, 1e-4f));
  if (dot_vec3(randomInSphere, normal) >= 0) return build_ray(origin, randomInSphere);
  return build_ray(origin, scale_vec3(randomInSphere, -1));
}

Material build_emissive_material() {
  return (Material) {
    .emissive = true,
      .scatter = NULL,
      .data = NULL
  };
}

Material build_diffuse_material() {
  return (Material) {
    .emissive = false,
      .scatter = scatter_diffuse,
      .data = NULL
  };
}

// Forward declaration
Material get_material_from_hit(SceneHit hit);

Ray scatter_metallic(Ray ray, SceneHit hit) {
  f32 fuzz = *((f32*)get_material_from_hit(hit).data);

  fuzz = fmin(1, fmax(0, fuzz));
  Vec3 normal = { 0, 0, 0 };
  if (hit.geometry_type == TRIANGLE) {
    normal = normal_to_point_triangle(hit.triangle, hit.position);
  }
  else if (hit.geometry_type == SPHERE) {
    normal = normal_to_point_sphere(hit.sphere, hit.position);
  }
  Vec3 v = ray.dir;
  Vec3 reflected = sub_vec3(v, scale_vec3(normal, 2 * dot_vec3(v, normal)));
  reflected = add_vec3(reflected, scale_vec3(random_point_in_sphere(), fuzz));
  normalize_vec3(reflected, &reflected);
  return build_ray(hit.position, reflected);
}

Material build_metallic_material(f32 fuzz) {
  f32* data = (f32*)malloc(sizeof(f32));
  *data = fuzz;
  return (Material) {
    .emissive = false,
      .scatter = scatter_metallic,
      .data = data,
  };
}

Ray scatter_alpha(Ray ray, SceneHit hit) {
  f32 alpha = *((f32*)get_material_from_hit(hit).data);
  Vec3 point = hit.position;
  if (random_double() <= alpha) return scatter_diffuse(ray, hit);
  Vec3 v = sub_vec3(point, ray.init);
  f32 t = INFINITY;
  if (ray.dir.x != 0) t = v.x / ray.dir.x;
  if (ray.dir.y != 0) t = v.y / ray.dir.y;
  if (ray.dir.z != 0) t = v.z / ray.dir.z;
  return build_ray(trace_ray(ray, t + 1e-2), ray.dir);

}

Material build_alpha_material(f32 alpha) {
  f32* data = (f32*)malloc(sizeof(f32));
  *data = fmin(fmax(alpha, 0), 1);
  return (Material) {
    .emissive = false,
      .scatter = scatter_alpha,
      .data = data,
  };
}

Ray scatter_dielectric(Ray ray, SceneHit hit) {
  f32 index_of_refraction = *((f32*)get_material_from_hit(hit).data);
  Vec3 point = hit.position;
  Vec3 p = sub_vec3(point, ray.init);
  f32 t = INFINITY;
  if (ray.dir.x != 0) t = p.x / ray.dir.x;
  if (ray.dir.y != 0) t = p.y / ray.dir.y;
  if (ray.dir.z != 0) t = p.z / ray.dir.z;

  Vec3 normal = { 0, 0, 0 };
  if (hit.geometry_type == TRIANGLE) {
    normal = normal_to_point_triangle(hit.triangle, hit.position);
  }
  else if (hit.geometry_type == SPHERE) {
    normal = normal_to_point_sphere(hit.sphere, hit.position);
  }
  Vec3 v_in = ray.dir;
  bool is_outside = dot_vec3(v_in, normal) < 0;
  f32 refraction_ration = is_outside ? 1 / index_of_refraction : index_of_refraction;
  Vec3 n = is_outside ? scale_vec3(normal, -1) : normal;
  f32 cos_theta_in = fminf(1.0f, dot_vec3(v_in, n));
  f32 sin_theta_in = sqrtf(1.0f - cos_theta_in * cos_theta_in);
  f32 sin_theta_out = refraction_ration * sin_theta_in;
  if (sin_theta_out > 1) {
    // reflect
    Vec3 v_out = sub_vec3(v_in, scale_vec3(n, -2 * cos_theta_in));
    return build_ray(trace_ray(ray, t + 1e-2), v_out);
  }
  // refract
  f32 cos_theta_out = sqrt(1 - sin_theta_out * sin_theta_out);
  Vec3 vp = scale_vec3(n, cos_theta_in);
  Vec3 vo = sub_vec3(v_in, vp);
  normalize_vec3(vo, &vo);

  Vec3 v_out = add_vec3(scale_vec3(n, cos_theta_out), scale_vec3(vo, sin_theta_out));

  return build_ray(trace_ray(ray, t + 1e-2), v_out);
}

Material build_dielectric_material(f32 index_of_refraction) {
  f32* data = (f32*)malloc(sizeof(f32));
  *data = index_of_refraction;
  return (Material) {
    .emissive = false,
      .scatter = scatter_dielectric,
      .data = data,
  };
}

Color get_color_from_hit(SceneHit hit, Ray ray, bool bilinear_texture) {
  if (hit.geometry_type == TRIANGLE) {
    RasterTriangleProps* props = (RasterTriangleProps*)hit.triangle->props;
    Triangle* tri = hit.triangle;

    // Compute barycentric coordinates via tangent-based projection
    Vec3 u1 = sub_vec3(tri->positions[1], tri->positions[0]);
    Vec3 u2 = sub_vec3(tri->positions[2], tri->positions[0]);
    Vec3 v = sub_vec3(ray.init, tri->positions[0]);
    Vec3 r = ray.dir;
    f32 det_inv = 1.0f / dot_vec3(cross_vec3(u1, u2), r);
    f32 alpha = dot_vec3(cross_vec3(v, u2), r) * det_inv;
    f32 beta = dot_vec3(cross_vec3(u1, v), r) * det_inv;
    f32 gamma = 1.0f - alpha - beta;


    // Texture sampling if available
    bool have_texture = props->texture != NULL
      && !(props->tex_coords[0].x == 0 && props->tex_coords[0].y == 0
        && props->tex_coords[1].x == 0 && props->tex_coords[1].y == 0
        && props->tex_coords[2].x == 0 && props->tex_coords[2].y == 0);

    if (have_texture) {
      Vec2 tex_uv = add_vec2(
        add_vec2(
          scale_vec2(props->tex_coords[0], gamma),
          scale_vec2(props->tex_coords[1], alpha)
        ),
        scale_vec2(props->tex_coords[2], beta)
      );
      return bilinear_texture
        ? get_bilinear_tex_color(props->texture, tex_uv)
        : get_tex_color(props->texture, tex_uv);
    }

    // Barycentric color interpolation
    return add_color(
      add_color(
        scale_color(props->colors[0], gamma),
        scale_color(props->colors[1], alpha)
      ),
      scale_color(props->colors[2], beta)
    );
  }
  if (hit.geometry_type == SPHERE) {
    RasterSphereProps* props = (RasterSphereProps*)hit.sphere->props;
    return props->color;
  }
  return (Color) { 0, 0, 0, 0 };
}

Material get_material_from_hit(SceneHit hit) {
  if (hit.geometry_type == TRIANGLE) {
    RasterTriangleProps* props = (RasterTriangleProps*)hit.triangle->props;
    return *props->material;
  }
  if (hit.geometry_type == SPHERE) {
    RasterSphereProps* props = (RasterSphereProps*)hit.sphere->props;
    return *props->material;
  }
  return (Material) { 0 };
}

Color render_miss_scene(Ray ray, void* context) {
  RayTraceLambdaInput* input = (RayTraceLambdaInput*)context;
  RaytraceParams* params = input->params;
  Scene* scene = input->scene;
  DirectionalLightParams* directional_light = params->directional_light;
  Color(*render_background)(Ray, void*) = params->render_background;
  void* render_background_context = params->render_background_context;

  Color sky_color = render_background != NULL ? render_background(ray, render_background_context) : COLOR_BLACK;
  if (directional_light == NULL) {
    return sky_color;
  }
  SceneHit hit = intersect_scene(scene, build_ray(ray.init, directional_light->direction));
  if (hit.hit) {
    return lerp_color(sky_color, COLOR_BLACK, 0.5f);
  }

  f32 dot = fmaxf(0, dot_vec3(directional_light->direction, ray.dir));

  f32 sun_intensity = powf(dot, directional_light->sharpness);

  return lerp_color(sky_color, COLOR_WHITE, sun_intensity);
}

Color trace_ray_scene(Ray ray, RayTraceLambdaInput* input, u32 bounces) {
  Scene* scene = input->scene;
  RaytraceParams* params = input->params;
  Color(*render_background)(Ray, void*) = params->render_background;
  void* render_background_context = params->render_background_context;
  bool bilinear_texture = params->bilinear_texture;

  if (bounces == 0) {
    return render_miss_scene(ray, input);
  }

  SceneHit intersection = intersect_scene(scene, ray);
  if (!intersection.hit) {
    return render_miss_scene(ray, input);
  }

  GeometryType hit_geometry_type = intersection.geometry_type;
  Color albedo = get_color_from_hit(intersection, ray, bilinear_texture);
  Material material = get_material_from_hit(intersection);

  bool is_emissive = material.emissive;
  if (is_emissive) {
    return albedo;
  }

  Ray scatter_ray = material.scatter(ray, intersection);
  Color scattered_color = trace_ray_scene(scatter_ray, input, bounces - 1);
  Vec3 normal = { 0, 0, 0 };
  if (hit_geometry_type == TRIANGLE) {
    normal = normal_to_point_triangle(intersection.triangle, intersection.position);
  }
  else if (hit_geometry_type == SPHERE) {
    normal = normal_to_point_sphere(intersection.sphere, intersection.position);
  }
  f32 attenuation = fabs(dot_vec3(normal, scatter_ray.dir));
  Color final_color = mul_color(albedo, scattered_color);
  final_color = scale_color(final_color, attenuation);
  final_color.alpha = 1.0f;
  return final_color;
}

Color ray_trace_lambda(Ray ray, void* context) {
  RayTraceLambdaInput* ray_trace_inputs = (RayTraceLambdaInput*)context;
  RaytraceParams* params = ray_trace_inputs->params;
  u32 samples = params->samples_per_pixel;
  u32 bounces = params->bounces;
  f32 variance = params->variance;
  f32 gamma = params->gamma;
  bool bilinear_texture = params->bilinear_texture;
  bool is_biased = params->is_biased;
  Tela* exposed_tela = params->exposed_tela;
  Camera* camera = params->camera;
  DirectionalLightParams* directional_light = params->directional_light;

  f32 inv_samples = (is_biased ? bounces : 1.0f) / samples;
  Color accumulated_color = { 0, 0, 0, 0 };
  for (u32 i = 0; i < samples; i++) {
    const Vec3 epsilon = scale_vec3(random_point_in_sphere(), variance);
    const Vec3 epsilon_ortho = sub_vec3(epsilon, scale_vec3(ray.dir, dot_vec3(epsilon, ray.dir)));
    Vec3 new_dir = add_vec3(ray.dir, epsilon_ortho);
    normalize_vec3(new_dir, &new_dir);
    Ray jittered_ray = build_ray(ray.init, new_dir);
    accumulated_color = add_color(accumulated_color, trace_ray_scene(jittered_ray, ray_trace_inputs, bounces));
  }
  return gamma_color(scale_color(accumulated_color, inv_samples), gamma);
}

Tela* ray_map_camera_exposed(Camera* camera, Tela* tela,
  Color(*ray_scene)(Ray, void*), void* context) {

  LambdaRayContext lambda_context = {
      .camera = camera,
      .tela = tela,
      .lambda_context = context,
      .lambdaWithRays = ray_scene,
  };
  return map_exposed_tela(tela, lambda_tela_from_ray, &lambda_context);
}

Tela* ray_map_camera_exposed_parallel(Camera* camera, Tela* tela,
  Color(*ray_scene)(Ray, void*), void* context) {

  LambdaRayContext lambda_context = {
      .camera = camera,
      .tela = tela,
      .lambda_context = context,
      .lambdaWithRays = ray_scene,
  };
  return map_exposed_tela_parallel(tela, lambda_tela_from_ray, &lambda_context);
}

Tela* ray_trace_scene(Scene* scene, RaytraceParams* params) {
  RayTraceLambdaInput lambda_input = {
    .scene = scene,
    .params = params
  };
  return ray_map_camera_exposed(params->camera, params->exposed_tela, ray_trace_lambda, &lambda_input);
}

Tela* ray_trace_scene_parallel(Scene* scene, RaytraceParams* params) {
  RayTraceLambdaInput lambda_input = {
    .scene = scene,
    .params = params
  };
  return ray_map_camera_exposed_parallel(params->camera, params->exposed_tela, ray_trace_lambda, &lambda_input);
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

Mesh map_triangles_materials_mesh(Mesh* mesh, Material(*mapper)(Face, void*), void* context) {
  mesh->materials = new_array(mesh->faces.length, sizeof(Material));
  for (u32 i = 0; i < mesh->faces.length; i++) {
    Face* face = (Face*)get_array_element(&mesh->faces, i);
    Material m = mapper(*face, context);
    push_array(&mesh->materials, &m);
  }
  return *mesh;
}

Mesh map_colors_mesh(Mesh* mesh, Color(*mapper)(Vec3, void*), void* context) {
  mesh->colors = new_array(mesh->vertices.length, sizeof(Color));
  for (u32 i = 0; i < mesh->vertices.length; i++) {
    Vec3* v = (Vec3*)get_array_element(&mesh->vertices, i);
    Color c = mapper(*v, context);
    push_array(&mesh->colors, &c);
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
    tri.radius = 0.0f;
    tri.props = NULL;
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

    // material
    if (mesh->materials.length > 0) {
      Material* mat = (Material*)get_array_element(&mesh->materials, i);
      if (mat) {
        Material* mat_copy = (Material*)malloc(sizeof(Material));
        *mat_copy = *mat;
        props->material = mat_copy;
      }
      else {
        props->material = NULL;
      }
    }
    else {
      props->material = NULL;
    }

    tri.props = props;
    push_array(&triangles, &tri);
  }
  return triangles;
}


Array get_spheres_mesh(Mesh* mesh, f32 radius) {
  Array spheres = new_array(mesh->vertices.length, sizeof(Sphere));
  Color default_color = { 1.0f, 1.0f, 1.0f, 1.0f };

  // Track which vertices have already been added (deduplication)
  bool visited[mesh->vertices.length];
  memset(visited, 0, mesh->vertices.length);

  for (u32 i = 0; i < mesh->faces.length; i++) {
    Face* face = (Face*)get_array_element(&mesh->faces, i);

    for (u32 j = 0; j < 3; j++) {
      u32 vi = face->vertex_indices[j];
      if (visited[vi]) continue;
      visited[vi] = true;

      Vec3* v = (Vec3*)get_array_element(&mesh->vertices, vi);
      Sphere sphere = build_sphere(v ? *v : vec3(0, 0, 0), radius);

      // Build props (color, tex_coord, texture)
      RasterSphereProps* props = (RasterSphereProps*)malloc(sizeof(RasterSphereProps));
      props->texture = mesh->texture;

      if (mesh->colors.length > 0) {
        Color* c = (Color*)get_array_element(&mesh->colors, vi);
        props->color = c ? *c : default_color;
      }
      else {
        props->color = default_color;
      }

      if (mesh->tex_coords.length > 0) {
        Vec2* tc = (Vec2*)get_array_element(&mesh->tex_coords, face->tex_coord_indices[j]);
        props->tex_coord = tc ? *tc : vec2(0, 0);
      }
      else {
        props->tex_coord = vec2(0, 0);
      }

      sphere.props = props;
      push_array(&spheres, &sphere);
    }
  }

  return spheres;
}

//========================================================================================
/*                                                                                      *
 *                                OBJ PARSING MESH UTILS                                *
 *                                                                                      */
 //========================================================================================

typedef struct {
  u32 start;
  u32 end;
} ObjLine;

typedef struct {
  u32 vertex;
  u32 tex_coord;
  u32 normal;
} ObjFaceToken;

static inline u32 obj_next_line(String str, u32 pos, ObjLine* line) {
  line->start = pos;
  while (pos < str.length && str.data[pos] != '\n' && str.data[pos] != '\r') pos++;
  line->end = pos;
  while (pos < str.length && (str.data[pos] == '\n' || str.data[pos] == '\r')) pos++;
  return pos;
}

static inline u32 obj_skip_whitespace(const char* data, u32 pos, u32 end) {
  while (pos < end && (data[pos] == ' ' || data[pos] == '\t')) pos++;
  return pos;
}

static inline bool obj_match_prefix(const char* data, u32 pos, u32 end, const char* prefix) {
  u32 len = (u32)strlen(prefix);
  for (u32 i = 0; i < len; i++) {
    if (pos + i >= end || data[pos + i] != prefix[i]) return false;
  }
  u32 after = pos + len;
  return after >= end || data[after] == ' ' || data[after] == '\t';
}

static inline u32 obj_parse_u32(const char* data, u32 pos, u32 end, u32* out) {
  u32 val = 0;
  while (pos < end && data[pos] >= '0' && data[pos] <= '9') {
    val = val * 10 + (data[pos] - '0');
    pos++;
  }
  *out = val;
  return pos;
}

static inline u32 obj_parse_face_token(const char* data, u32 pos, u32 end, ObjFaceToken* token) {
  *token = (ObjFaceToken){ 0 };
  pos = obj_parse_u32(data, pos, end, &token->vertex);
  if (token->vertex == 0) return pos;
  if (pos < end && data[pos] == '/') {
    pos++;
    if (pos < end && data[pos] >= '0' && data[pos] <= '9') {
      pos = obj_parse_u32(data, pos, end, &token->tex_coord);
    }
    if (pos < end && data[pos] == '/') {
      pos++;
      if (pos < end && data[pos] >= '0' && data[pos] <= '9') {
        pos = obj_parse_u32(data, pos, end, &token->normal);
      }
    }
  }
  return pos;
}

static inline void obj_parse_vertex(const char* data, u32 pos, Array* vertices) {
  f32 x = 0, y = 0, z = 0;
  sscanf(data + pos, "%f %f %f", &x, &y, &z);
  Vec3 v = vec3(x, y, z);
  push_array(vertices, &v);
}

static inline void obj_parse_normal(const char* data, u32 pos, Array* normals) {
  f32 nx = 0, ny = 0, nz = 0;
  sscanf(data + pos, "%f %f %f", &nx, &ny, &nz);
  Vec3 n = vec3(nx, ny, nz);
  push_array(normals, &n);
}

static inline void obj_parse_tex_coord(const char* data, u32 pos, Array* tex_coords) {
  f32 u = 0, v = 0;
  sscanf(data + pos, "%f %f", &u, &v);
  Vec2 tc = vec2(u, v);
  push_array(tex_coords, &tc);
}

static inline void obj_triangulate_face(ObjFaceToken* tokens, u32 count, Array* faces) {
  for (u32 k = 1; k + 1 < count; k++) {
    u32 tri[3] = { 0, k, k + 1 };
    Face face;
    for (u32 j = 0; j < 3; j++) {
      face.vertex_indices[j] = tokens[tri[j]].vertex - 1;
      face.tex_coord_indices[j] = tokens[tri[j]].tex_coord > 0 ? tokens[tri[j]].tex_coord - 1 : 0;
      face.normal_indices[j] = tokens[tri[j]].normal > 0 ? tokens[tri[j]].normal - 1 : 0;
    }
    push_array(faces, &face);
  }
}

static inline void obj_parse_face(const char* data, u32 pos, u32 end, Array* faces) {
  ObjFaceToken tokens[16];
  u32 count = 0;
  while (pos < end && count < 16) {
    pos = obj_skip_whitespace(data, pos, end);
    if (pos >= end) break;
    ObjFaceToken token;
    pos = obj_parse_face_token(data, pos, end, &token);
    if (token.vertex == 0) break;
    tokens[count++] = token;
  }
  obj_triangulate_face(tokens, count, faces);
}

Mesh read_obj_mesh(String obj_file, char* mesh_name) {
  Mesh mesh = { 0 };
  mesh.name = create_string(mesh_name);
  mesh.vertices = new_array(64, sizeof(Vec3));
  mesh.tex_coords = new_array(64, sizeof(Vec2));
  mesh.normals = new_array(64, sizeof(Vec3));
  mesh.faces = new_array(64, sizeof(Face));

  u32 pos = 0;

  while (pos < obj_file.length) {
    ObjLine line;
    pos = obj_next_line(obj_file, pos, &line);
    if (line.end == line.start) continue;

    u32 i = obj_skip_whitespace(obj_file.data, line.start, line.end);
    if (i >= line.end || obj_file.data[i] == '#') continue;

    if (obj_match_prefix(obj_file.data, i, line.end, "vn")) {
      obj_parse_normal(obj_file.data, i + 2, &mesh.normals);
    }
    else if (obj_match_prefix(obj_file.data, i, line.end, "vt")) {
      obj_parse_tex_coord(obj_file.data, i + 2, &mesh.tex_coords);
    }
    else if (obj_match_prefix(obj_file.data, i, line.end, "v")) {
      obj_parse_vertex(obj_file.data, i + 1, &mesh.vertices);
    }
    else if (obj_match_prefix(obj_file.data, i, line.end, "f")) {
      obj_parse_face(obj_file.data, i + 1, line.end, &mesh.faces);
    }
  }
  return mesh;
}



#endif /* TELA_C */
