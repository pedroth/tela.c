#include "Camera/raster.h"
#include "Camera/Camera.h"
#include "Color/Color.h"
#include "Scene/Scene.h"
#include "Tela/Tela.h"
#include "Utils/Array.h"
#include "Vector/Vec2.h"
#include "Vector/Vec3.h"
#include <math.h>

f32 ditheringMatrix4x4[16] = {
    0 / 16, 8 / 16,  2 / 16, 10 / 16, 12 / 16, 4 / 16, 14 / 16, 6 / 16,
    3 / 16, 11 / 16, 1 / 16, 9 / 16,  15 / 16, 7 / 16, 13 / 16, 5 / 16};

void raster_scene(const Camera *camera, const Scene *scene, Tela *tela,
                  RasterParams *params) {
  RasterParams default_params = {.cull_backfaces = true,
                                 .bilinear_texture = false,
                                 .clip_camera_plane = true,
                                 .clear_screen = true,
                                 .background_color =
                                     (Color){0.0f, 0.0f, 0.0f, 1.0f},
                                 .perspective_correct = true};
  if (params == NULL) {
    params = &default_params;
  }

  if (params->clear_screen && tela != NULL) {
    tela_fill(tela, params->background_color);
  }

  const u32 w = tela->width;
  const u32 h = tela->height;
  f32 z_buffer[w * h];
  for (u32 i = 0; i < w * h; i++) {
    z_buffer[i] = INFINITY;
  }
  Array triangles = get_triangles_scene(scene);
  for (u32 i = 0; i < triangles.length; i++) {
    Triangle *triangle = (Triangle *)triangles.data[i];
    raster_triangle(tela, camera, triangle, w, h, z_buffer, params);
  }
}

typedef struct {
  Vec2 int_points[3];
  Vec3 points_in_cam_coord[3];
  Vec2 u;
  Vec2 v;
  f32 inv_det;
  f32 c1[3];
  f32 c2[3];
  f32 c3[3];
  Vec2 tex_coords[3];
  Tela *texture;
  bool have_textures;
  bool perspective_correct;
  bool bilinear_texture;
  u32 w;
  u32 h;
  Tela *tela;
  f32 *z_buffer;
} ShaderContext;

Color get_bilinear_tex_color(Vec2 uv, Tela *texture) {}
Color get_tex_color(Vec2 uv, Tela *texture) {}

void shader_triangle(u32 x, u32 y, void *context) {
  ShaderContext *ctx = (ShaderContext *)context;
  Vec2 *int_points = ctx->int_points;
  Vec3 *points_in_cam_coord = ctx->points_in_cam_coord;
  Vec2 u = ctx->u;
  Vec2 v = ctx->v;
  f32 inv_det = ctx->inv_det;
  f32 *c1 = ctx->c1;
  f32 *c2 = ctx->c2;
  f32 *c3 = ctx->c3;
  Vec2 *tex_coords = ctx->tex_coords;
  Tela *texture = ctx->texture;
  bool have_textures = ctx->have_textures;
  bool perspective_correct = ctx->perspective_correct;
  bool bilinear_texture = ctx->bilinear_texture;
  u32 w = ctx->w;
  u32 h = ctx->h;
  Tela *tela = ctx->tela;
  f32 *z_buffer = ctx->z_buffer;

  f32 W = 1;
  f32 w_reciprocal = 1;
  Vec2 p = sub_vec2(vec2(x, y), int_points[0]);
  f32 alpha = -(v.x * p.y - v.y * p.x) * inv_det;
  f32 beta = (u.x * p.y - u.y * p.x) * inv_det;
  f32 gamma = 1 - alpha - beta;
  f32 zs[3] = {points_in_cam_coord[0].z, points_in_cam_coord[1].z,
               points_in_cam_coord[2].z};
  if (perspective_correct) {
    // wReciprocal is the weight for perspective correction of z coordinate
    W = (1 / zs[0]) * gamma + (1 / zs[1]) * alpha + (1 / zs[2]) * beta;
    w_reciprocal = 1 / W;
    alpha = (alpha / zs[1]) * w_reciprocal;
    beta = (beta / zs[2]) * w_reciprocal;
    gamma = (gamma / zs[0]) * w_reciprocal;
  } else {
    w_reciprocal = zs[0] * gamma + zs[1] * alpha + zs[2] * beta;
  }
  // compute color
  Color c = (Color){c1[0] * gamma + c2[0] * alpha + c3[0] * beta,
                    c1[1] * gamma + c2[1] * alpha + c3[1] * beta,
                    c1[2] * gamma + c2[2] * alpha + c3[2] * beta,
                    c1[3] * gamma + c2[3] * alpha + c3[3] * beta};
  if (have_textures && texture != NULL) {
    Vec2 texUV = add_vec2(scale_vec2(tex_coords[0], gamma),
                          add_vec2(scale_vec2(tex_coords[1], alpha),
                                   scale_vec2(tex_coords[2], beta)));
    Color texColor = bilinear_texture ? get_bilinear_tex_color(texUV, texture)
                                      : get_tex_color(texUV, texture);
    c = texColor;
  }
  Vec2 gridPos = canvas_to_grid(tela, x, y);
  u32 i = (u32)gridPos.x;
  u32 j = (u32)gridPos.y;
  u32 z_buffer_index = (u32)(w * i + j);
  if (w_reciprocal < z_buffer[z_buffer_index]) {
    const f32 matrixValue = ditheringMatrix4x4[(i % 4) * 4 + (j % 4)];
    Color *color = matrixValue < c.alpha ? &c : NULL;
    if (color != NULL) {
      z_buffer[z_buffer_index] =
          w_reciprocal; // if color is undefined, don't update z_buffer
    }
    return color;
  }
}

void raster_triangle(Tela *tela, const Camera *camera, Triangle *elem, u32 w,
                     u32 h, f32 *z_buffer, RasterParams *params) {
  f32 distanceToPlane = camera->distance_to_plane;
  Vec3 *positions = elem->positions;
  Color *colors = elem->colors;
  Vec2 *tex_coords = elem->texture_coords;
  Tela *texture = elem->texture;
  // camera coords
  Vec3 points_in_camera_coords[3];
  for (u32 i = 0; i < 3; i++) {
    to_camera_coords(camera, &positions[i], &points_in_camera_coords[i]);
  }

  // back face culling
  if (params->cull_backfaces) {
    Vec3 du = sub_vec3(points_in_camera_coords[1], points_in_camera_coords[0]);
    Vec3 dv = sub_vec3(points_in_camera_coords[2], points_in_camera_coords[0]);
    Vec3 n = cross_vec3(du, dv);
    Vec3 n_normalized = normalize_vec3(n);
    if (dot_vec3(n_normalized, points_in_camera_coords[0]) <= 0)
      return;
  }
  // frustum culling
  Array inFrustum = new_array(0, sizeof(Vec3));
  Array outFrustum = new_array(0, sizeof(Vec3));
  for (u32 i = 0; i < 3; i++) {
    f32 zCoord = points_in_camera_coords[i].z;
    if (zCoord < distanceToPlane) {
      push_array(&outFrustum, &i);
    } else {
      push_array(&inFrustum, &i);
    }
  }
  if (params->clip_camera_plane && outFrustum.length >= 1)
    return;
  // project
  Vec3 projected_points[3];
  Vec2 int_points[3];
  for (u32 i = 0; i < 3; i++) {
    projected_points[i] =
        scale_vec3(points_in_camera_coords[i],
                   distanceToPlane / points_in_camera_coords[i].z);
    int_points[i] = vec2((w / 2) + projected_points[i].x * w,
                         (h / 2) + projected_points[i].y * h);
    int_points[i].x = (f32)((i32)int_points[i].x);
    int_points[i].y = (f32)((i32)int_points[i].y);
  }
  Vec2 u = sub_vec2(int_points[1], int_points[0]);
  Vec2 v = sub_vec2(int_points[2], int_points[0]);
  f32 det = u.x * v.y - u.y * v.x; // wedge product
  if (det == 0.0f)
    return;
  f32 invDet = 1.0f / det;
  f32 c1[3] = {colors[0].red, colors[0].green, colors[0].blue};
  f32 c2[3] = {colors[1].red, colors[1].green, colors[1].blue};
  f32 c3[3] = {colors[2].red, colors[2].green, colors[2].blue};
  bool have_textures =
      tex_coords != NULL && (texture != NULL || elem->texture == NULL);

  canvas.drawTriangle(intPoints[0], intPoints[1], intPoints[2], shader);
}
