#include "Tela.h"
#include "../Color/Color.h"
#include "../Vector/Vec2.h"
#include <stddef.h>
#include <stdlib.h>

Tela *new_tela(u32 width, u32 height) {
  Tela *tela = (Tela *)malloc(sizeof(Tela));
  tela->width = width;
  tela->height = height;
  tela->channels = COLOR_CHANNELS;
  tela->box = build_aabb(vec3(0.0f, 0.0f, 0.0f), vec3((f32)width, (f32)height, 0.0f));
  tela->image = (f32 *)calloc(width * height * COLOR_CHANNELS, sizeof(f32));
  return tela;
}

Tela *map_tela(Tela *tela, Color *(*lambda)(u32, u32, void const *),
               void const *context) {
  const u32 w = tela->width;
  const u32 h = tela->height;
  const u32 c = tela->channels;
  const u32 size = w * h * c;
  for (u32 k = 0; k < size; k += c) {
    u32 i = k / (c * w);
    u32 j = (k / c) % w;
    const u32 x = j;
    const u32 y = h - 1 - i;
    Color *color = lambda(x, y, context);
    if (color == NULL) {
      continue;
    }
    tela->image[k + 0] = color->red;
    tela->image[k + 1] = color->green;
    tela->image[k + 2] = color->blue;
    tela->image[k + 3] = color->alpha;
    free_color(color);
  }
  return tela;
}

Tela *fill_tela(Tela *tela, Color color) {
  const u32 w = tela->width;
  const u32 h = tela->height;
  const u32 c = tela->channels;
  const u32 size = w * h * c;
  for (u32 k = 0; k < size; k += c) {
    tela->image[k + 0] = color.red;
    tela->image[k + 1] = color.green;
    tela->image[k + 2] = color.blue;
    tela->image[k + 3] = color.alpha;
  }
  return tela;
}

Tela *draw_convex_polygon_tela(Tela *tela, Vec2 *polygon_pos, u32 vertex_count,
                               Color *(*lambda)(u32, u32, void const *),
                               void const *context) {
  (void)polygon_pos;
  (void)vertex_count;
  (void)lambda;
  (void)context;
  // TODO: implement convex polygon drawing
  return tela;
}

Tela *draw_triangle_tela(Tela *tela, Vec2 triangle_pos[3],
                         Color *(*lambda)(u32, u32, void const *),
                         void const *context) {
  return draw_convex_polygon_tela(tela, triangle_pos, 3, lambda, context);
}


Vec2 canvas2grid(Tela *tela, u32 x, u32 y) {
  const u32 h = tela->height;
  const u32 j = x;
  const u32 i = h - 1 - y;
  return vec2((f32)j, (f32)i);
}

void free_tela(Tela *tela) {
  free(tela->image);
  tela->image = NULL;
  free(tela);
}