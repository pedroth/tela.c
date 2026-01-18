#ifndef CAMERA_H
#define CAMERA_H

#include "Vector/Vec2.h"
#include "Vector/Vec3.h"

typedef struct {
  Vec3 position;
  Vec3 look_at;
  f32 distance_to_plane;
  Vec3 orbit_coords; // radius, theta, phi
  Vec2 orientation;  // theta, phi
  Vec3 basis[3];     // matrix transforming camera space to world space
} Camera;

Camera set_orient_camera(Camera *camera, f32 theta, f32 phi);
Camera set_orbit_camera(Camera *camera, f32 radius, f32 theta, f32 phi);
Vec3 get_camera_orbit(const Camera *camera);

#endif // CAMERA_H