#ifndef RASTER_H
#define RASTER_H

#include <stddef.h>
#include "utils/types.h"
#include "Color/Color.h"

#include "Camera/Camera.h"
#include "Scene/Scene.h"
#include "Tela/Tela.h"

typedef struct RasterParams {
  bool cull_backfaces;
  bool bilinear_texture;
  bool clip_camera_plane;
  bool clear_screen;
  Color background_color;
  bool perspective_correct;
} RasterParams;

void raster_scene(const Camera *camera, const Scene *scene, Tela *tela,
                  RasterParams *params);

#endif // RASTER_H