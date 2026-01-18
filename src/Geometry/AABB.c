#include "Geometry/AABB.h"
#include "Vector/Vec3.h"
#include <math.h>

AABB build_aabb(Vec3 min, Vec3 max) {
    AABB box;
    box.min = vec3(
        fminf(min.x, max.x),
        fminf(min.y, max.y),
        fminf(min.z, max.z)
    );
    box.max = vec3(
        fmaxf(min.x, max.x),
        fmaxf(min.y, max.y),
        fmaxf(min.z, max.z)
    );
    box.center = scale_vec3(add_vec3(box.min, box.max), 0.5f);
    box.diagonal = vec3(
        box.max.x - box.min.x,
        box.max.y - box.min.y,
        box.max.z - box.min.z
    );
    box.is_empty = box.diagonal.x < 0.0f || box.diagonal.y < 0.0f || box.diagonal.z < 0.0f;
    return box;
}

AABB union_aabb(const AABB *a, const AABB *b) {
    if (a->is_empty) return *b;
    if (b->is_empty) return *a;

    Vec3 new_min = vec3(
        fminf(a->min.x, b->min.x),
        fminf(a->min.y, b->min.y),
        fminf(a->min.z, b->min.z)
    );
    Vec3 new_max = vec3(
        fmaxf(a->max.x, b->max.x),
        fmaxf(a->max.y, b->max.y),
        fmaxf(a->max.z, b->max.z)
    );
    return build_aabb(new_min, new_max);
}
