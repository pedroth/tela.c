#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "Utils/types.h"
#include "Vector/Vec3.h"
#include "Vector/Vec2.h"
#include "Color/Color.h"
#include "Tela/Tela.h"
#include "Geometry.h" // Ensure this header defines GeometryType
#include "AABB.h"
#include <stdlib.h>
#include <string.h>

typedef struct Triangle {
  char *name;
  Color colors[3];
  Vec3 normals[3];
  Tela *texture;
  Vec2 texture_coords[3];
  Vec3 positions[3];
  GeometryType type; // Ensure GeometryType is defined in the included headers
} Triangle;

Triangle build_triangle(const char *name, Vec3 positions[3], Vec3 normals[3], Vec2 texture_coords[3],
                        Color colors[3], Tela *texture);
AABB get_bounding_box_triangle(const Triangle *a);

#endif // TRIANGLE_H