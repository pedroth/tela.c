#ifndef SRC_SCENE_SCENE_H
#define SRC_SCENE_SCENE_H

#include "Geometry/Triangle.h"
#include "Utils/Array.h"

typedef struct {
  Array triangles; // Array of Triangle
} NaiveScene;

typedef NaiveScene Scene;

Scene new_scene();
void free_scene(Scene *scene);
void add_triangle_to_scene(Scene *scene, const Triangle *triangle);
Array get_triangles_scene(const Scene *scene);

#endif // SRC_SCENE_SCENE_H
