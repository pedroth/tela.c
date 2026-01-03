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
} Mesh;

bool filter_obj_face_element(void *element, u32 index) {
  char *str = *(char **)element;
  return (str != NULL) && (str[0] != '\0');
}

/* Triangulate a polygon (array of vertex tokens) into an Array of triangle
   Arrays. Each triangle is itself an Array of `char *` tokens. Caller is
   responsible for freeing the returned Array and each inner Array's data.
*/
Array triangulate(Array *polygon) {
  Array triangles = new_array(0, sizeof(Array));
  if (polygon->length == 3) {
    Array tri = slice_array(polygon, 0, 3);
    array_push(&triangles, &tri);
    return triangles;
  }
  if (polygon->length == 4) {
    Array t1 = slice_array(polygon, 0, 3);
    Array t2 = new_array(3, polygon->element_size);
    char *p;
    p = *(char **)get_array_element(polygon, 2);
    array_push(&t2, &p);
    p = *(char **)get_array_element(polygon, 3);
    array_push(&t2, &p);
    p = *(char **)get_array_element(polygon, 0);
    array_push(&t2, &p);
    array_push(&triangles, &t1);
    array_push(&triangles, &t2);
    return triangles;
  }
  return triangles;
}

/* Parse a triangle's vertex token list (e.g. "1/2/3") and push vertex
   indices (zero-based) into `faces` as `u32` values in order.
*/
void parse_face(Array *vertexInfo, Array *faces) {
  if (!vertexInfo || vertexInfo->length == 0 || !faces) return;
  for (u32 i = 0; i < vertexInfo->length; i++) {
    char *token = *(char **)get_array_element(vertexInfo, i);
    if (!token) continue;
    const char *s = token;
    int idx = 0;
    while (*s) {
      char buf[32];
      int bi = 0;
      while (*s && *s != '/') {
        if (bi < (int)sizeof(buf) - 1) buf[bi++] = *s;
        s++;
      }
      buf[bi] = '\0';
      if (bi > 0 && idx == 0) {
        int val = atoi(buf);
        u32 z = 0;
        if (val > 0) z = (u32)(val - 1);
        array_push(faces, &z);
      }
      if (*s == '/') s++;
      idx++;
    }
  }
}

u32 MESH_COUNTER = 0;

Mesh parse_obj_mesh(const char *obj_data, const char *name) {
  char *mesh_name = NULL;
  if (!name) {
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "mesh%u", MESH_COUNTER++);
    name = tmp;
  }
  mesh_name = (char *)malloc(strlen(name) + 1);
  if (mesh_name) strcpy(mesh_name, name);
  Array vertices = new_array(7, sizeof(Vec3));
  Array normals = new_array(7, sizeof(Vec3));
  Array texture_coords = new_array(7, sizeof(Vec2));
  Array faces = new_array(7, sizeof(u32));
  Array lines = split_str(obj_data, "\n");
  for (u32 i = 0; i < lines.length; i++) {
    char *line = (char *)get_array_element(&lines, i);
    Array tmp = split_str(line, " ");
    Array spaces = filter_array(&tmp, filter_obj_face_element);
    free_array(&tmp);
    char *type = (char *)get_array_element(&spaces, 0);
    if (!type) {
      free_array(&spaces);
      continue;
    }
    if (strcmp(type, "v") == 0) {
      f32 x = atof(*(char **)get_array_element(&spaces, 1));
      f32 y = atof(*(char **)get_array_element(&spaces, 2));
      f32 z = atof(*(char **)get_array_element(&spaces, 3));
      Vec3 vertex = vec3(x, y, z);
      array_push(&vertices, &vertex);
    } else if (strcmp(type, "vn") == 0) {
      f32 x = atof(*(char **)get_array_element(&spaces, 1));
      f32 y = atof(*(char **)get_array_element(&spaces, 2));
      f32 z = atof(*(char **)get_array_element(&spaces, 3));
      Vec3 normal = vec3(x, y, z);
      array_push(&normals, &normal);
    } else if (strcmp(type, "vt") == 0) {
      f32 u = atof(*(char **)get_array_element(&spaces, 1));
      f32 v = atof(*(char **)get_array_element(&spaces, 2));
      Vec2 tex_coord = vec2(u, v);
      array_push(&texture_coords, &tex_coord);
    } else if (strcmp(type, "f") == 0) {
      Array face_elements = slice_array(&spaces, 1, spaces.length);
      Array tris = triangulate(&face_elements);
      for (u32 ti = 0; ti < tris.length; ti++) {
        Array *tri_ptr = (Array *)get_array_element(&tris, ti);
        if (tri_ptr) {
          parse_face(tri_ptr, &faces);
          free_array(tri_ptr);
        }
      }
      free_array(&tris);
      free_array(&face_elements);
    }
    free_array(&spaces);
  }

  Mesh mesh = {.vertices = (Vec3 *)vertices.data,
               .normals = (Vec3 *)normals.data,
               .texture_coords = (Vec2 *)texture_coords.data,
               .name = NULL,
               .texture = NULL,
               .faces = (u32 *)faces.data,
               .colors = NULL};

  return mesh;
}

AABB *get_mesh_aabb(Mesh mesh) {
    AABB box = EMPTY_AABB;
    u32 num_faces = mesh.faces ? (mesh.faces->length / 3) : 0;
    for (u32 i = 0; i < num_faces; i++) {
        for (u32 j = 0; j < 3; j++) {
            u32 vertex_index = mesh.faces[i * 3 + j];
            if (vertex_index < mesh.vertices->length) {
                Vec3 vertex = mesh.vertices[vertex_index];
                box = union_aabb(box, &build_aabb(vertex, vertex));
            }
        }
    }
    return box;
}

#endif // SRC_MESH_MESH_H
