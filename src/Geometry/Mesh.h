#ifndef SRC_MESH_MESH_H
#define SRC_MESH_MESH_H

#include "Color/Color.h"
#include "Tela/Tela.h"
#include "Utils/Array.h"
#include "Utils/strings.h"
#include "Utils/types.h"
#include "Vector/Vec2.h"
#include "Vector/Vec3.h"
#include "Geometry/AABB.h"
#include <stdio.h>

typedef struct {
  Vec3 *vertices;
  Vec3 *normals;
  Vec2 *texture_coords;
  char *name;
  Tela *texture;
  u32 *faces;
  Color *colors;
  u32 vertex_count;
  u32 face_count;
} Mesh;

bool filter_obj_face_element(void *element, u32 index);
Array triangulate(Array *polygon);
void parse_face(Array *vertexInfo, Array *faces);
Mesh parse_obj_mesh(const char *obj_data, const char *name);
AABB get_mesh_aabb(Mesh* mesh);

#endif // SRC_MESH_MESH_H
