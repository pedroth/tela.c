#include "Scene/Scene.h"
#include "Geometry/Triangle.h"
#include "Utils/Array.h"

Scene new_scene() {
  Scene scene;
  scene.triangles = new_array(0, sizeof(Triangle));
  return scene;
}

void free_scene(Scene *scene) {
  if (!scene) {
    return;
  }
  free_array(&scene->triangles);
}

void add_triangle_to_scene(Scene *scene, const Triangle *triangle) {
  if (!scene || !triangle) {
    return;
  }
  Array *triangles = &scene->triangles;
  if (triangles->data == NULL) {
    init_array(triangles, sizeof(Triangle), 4);
  }
  push_array(triangles, (void *)triangle);
}

Array get_triangles_scene(const Scene *scene) {
  if (!scene) {
    Array empty = {0};
    return empty;
  }
  return scene->triangles;
}