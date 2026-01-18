#include "Vector/Vec3.h"
#include <math.h>

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

Vec3 cross_vec3(const Vec3 a, const Vec3 b) {
  Vec3 result;
  result.x = a.y * b.z - a.z * b.y;
  result.y = a.z * b.x - a.x * b.z;
  result.z = a.x * b.y - a.y * b.x;
  return result;
}

Vec3 normalize_vec3(const Vec3 v) {
  f32 len = length_vec3(v);
  if (len < EPSILON) {
    return vec3(0.0f, 0.0f, 0.0f); // return zero vector if length is too small
  }
  return scale_vec3(v, 1.0f / len);
}

bool equals_vec3(const Vec3 a, const Vec3 b) {
  return fabsf(a.x - b.x) < EPSILON && fabsf(a.y - b.y) < EPSILON &&
         fabsf(a.z - b.z) < EPSILON;
}
