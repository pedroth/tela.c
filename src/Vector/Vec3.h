#ifndef SRC_VECTOR_VEC3_H
#define SRC_VECTOR_VEC3_H

#include "utils/types.h"
#include <math.h>
#define EPSILON 0.0001f

typedef struct {
  f32 x;
  f32 y;
  f32 z;
} Vec3;

Vec3 vec3(f32 x, f32 y, f32 z);
Vec3 add_vec3(const Vec3 a, const Vec3 b);
Vec3 sub_vec3(const Vec3 a, const Vec3 b);
Vec3 scale_vec3(const Vec3 v, f32 scalar);
f32 dot_vec3(const Vec3 a, const Vec3 b);
f32 length_vec3(const Vec3 v);
Vec3 cross_vec3(const Vec3 a, const Vec3 b);
Vec3 normalize_vec3(const Vec3 v);
bool equals_vec3(const Vec3 a, const Vec3 b);

#endif // SRC_VECTOR_VEC3_H