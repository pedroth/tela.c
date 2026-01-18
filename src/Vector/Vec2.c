#include "Vector/Vec2.h"
#include <math.h>

Vec2 vec2(f32 x, f32 y) {
  Vec2 v;
  v.x = x;
  v.y = y;
  return v;
}

Vec2 add_vec2(const Vec2 a, const Vec2 b) {
  Vec2 result;
  result.x = a.x + b.x;
  result.y = a.y + b.y;
  return result;
}

Vec2 sub_vec2(const Vec2 a, const Vec2 b) {
  Vec2 result;
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  return result;
}

Vec2 scale_vec2(const Vec2 v, f32 scalar) {
  Vec2 result;
  result.x = v.x * scalar;
  result.y = v.y * scalar;
  return result;
}

f32 dot_vec2(const Vec2 a, const Vec2 b) {
  return a.x * b.x + a.y * b.y;
}

f32 length_vec2(const Vec2 v) {
  return sqrtf(v.x * v.x + v.y * v.y);
}

bool equals_vec2(const Vec2 a, const Vec2 b) {
  return fabsf(a.x - b.x) < EPSILON && fabsf(a.y - b.y) < EPSILON;
}
