#include "Geometry/Triangle.h"
#include "Geometry/AABB.h"
#include <stdlib.h>
#include <string.h>

Triangle build_triangle(const char *name, Vec3 positions[3], Vec3 normals[3], Vec2 texture_coords[3],
                        Color colors[3], Tela *texture) {
  Triangle triangle;
  if (name) {
    u32 name_len = strlen(name);
    triangle.name = (char *)malloc(name_len + 1);
    strcpy(triangle.name, name);
  } else {
    triangle.name = NULL;
  }
  for (u32 i = 0; i < 3; i++) {
    triangle.positions[i] = positions[i];
    triangle.normals[i] = normals[i];
    triangle.texture_coords[i] = texture_coords[i];
    triangle.colors[i] = colors[i];
  }
  triangle.texture = texture;
  triangle.type = TRIANGLE;
  return triangle;
}

AABB get_bounding_box_triangle(const Triangle *a) {
    AABB box = EMPTY_AABB;
    for(u32 i = 0; i < 3; i++) {
        AABB vertex_box = build_aabb(a->positions[i], a->positions[i]);
        box = union_aabb(&box, &vertex_box);
    }
    return box;
}
