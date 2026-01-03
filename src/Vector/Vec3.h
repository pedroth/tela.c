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

Vec3 vec3(f32 x, f32 y, f32 z) {
  Vec3 v;
  v.x = x;
  v.y = y;
  v.z = z;
  return v;
}
Vec3 add_vec3(const Vec3 a, const Vec3 b) {
  Vec3 result;
  result.x = a.x + b.x;
  result.y = a.y + b.y;
  result.z = a.z + b.z;
  return result;
}
Vec3 sub_vec3(const Vec3 a, const Vec3 b) {
  Vec3 result;
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  result.z = a.z - b.z;
  return result;
}
Vec3 scale_vec3(const Vec3 v, f32 scalar) {
  Vec3 result;
  result.x = v.x * scalar;
  result.y = v.y * scalar;
  result.z = v.z * scalar;
  return result;
}
f32 dot_vec3(const Vec3 a, const Vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

f32 length_vec3(const Vec3 v) {
  return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

bool equals_vec3(const Vec3 a, const Vec3 b) {
  return fabsf(a.x - b.x) < EPSILON && fabsf(a.y - b.y) < EPSILON &&
         fabsf(a.z - b.z) < EPSILON;
}

#endif // SRC_VECTOR_VEC3_H