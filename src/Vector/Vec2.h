#ifndef SRC_VECTOR_VEC2_H
#define SRC_VECTOR_VEC2_H

#include "utils/types.h"
#include <math.h>

#define EPSILON 0.0001f

typedef struct {
  f32 x;
  f32 y;
} Vec2;

Vec2 vec2(f32 x, f32 y);
Vec2 add_vec2(const Vec2 a, const Vec2 b);
Vec2 sub_vec2(const Vec2 a, const Vec2 b);
Vec2 scale_vec2(const Vec2 v, f32 scalar);
f32 dot_vec2(const Vec2 a, const Vec2 b);
f32 length_vec2(const Vec2 v);
bool equals_vec2(const Vec2 a, const Vec2 b);

#endif // SRC_VECTOR_VEC2_H