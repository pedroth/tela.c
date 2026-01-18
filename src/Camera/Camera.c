#include "Camera/Camera.h"
#include "Vector/Vec2.h"
#include "Vector/Vec3.h"
#include <math.h>

Camera set_orient_camera(Camera *camera, f32 theta, f32 phi) {
  camera->orientation = vec2(theta, phi);

  f32 cosT = cosf(theta);
  f32 sinT = sinf(theta);
  f32 cosP = cosf(phi);
  f32 sinP = sinf(phi);

  // right hand coordinate system
  // z-axis
  camera->basis[2] = vec3(-cosP * cosT, -cosP * sinT, -sinP);
  // y-axis
  camera->basis[1] = vec3(-sinP * cosT, -sinP * sinT, cosP);
  // x-axis
  camera->basis[0] = vec3(-sinT, cosT, 0.0f);
  return *camera;
}

Camera set_orbit_camera(Camera *camera, f32 radius, f32 theta, f32 phi) {
  set_orient_camera(camera, theta, phi);

  f32 cosT = cosf(theta);
  f32 sinT = sinf(theta);
  f32 cosP = cosf(phi);
  f32 sinP = sinf(phi);

  Vec3 sphere_coords =
      vec3(radius * cosP * cosT, radius * cosP * sinT, radius * sinP);

  camera->orbit_coords = sphere_coords;
  camera->position = add_vec3(sphere_coords, camera->look_at);
  return *camera;
}

Vec3 get_camera_orbit(const Camera *camera) {
  return camera->orbit_coords;
}
