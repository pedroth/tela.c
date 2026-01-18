#ifndef AABB_H
#define AABB_H

#include "Vector/Vec3.h"

typedef struct AABB
{
    bool is_empty;
    Vec3 min;
    Vec3 max;
    Vec3 center;
    Vec3 diagonal;
} AABB;

#define EMPTY_AABB \
    (AABB){.is_empty = true, .min = {0, 0, 0}, .max = {0, 0, 0}, .center = {0, 0, 0}, .diagonal = {0, 0, 0}}


AABB build_aabb(Vec3 min, Vec3 max);
AABB union_aabb(const AABB *a, const AABB *b);

#endif // AABB_H