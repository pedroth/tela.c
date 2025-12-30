#include "Tela.h"
#include "../Color/Color.h"
#include <stddef.h>
#include <stdlib.h>

Tela *new_tela(u32 width, u32 height) {
  Tela *tela = (Tela *)malloc(sizeof(Tela));
  tela->width = width;
  tela->height = height;
  tela->channels = COLOR_CHANNELS;
  tela->image = (f32 *)calloc(width * height * COLOR_CHANNELS, sizeof(f32));
  return tela;
}

Tela *map_tela(Tela *tela, Color *(*lambda)(u32, u32, void const *), void const *context) {
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

void free_tela(Tela *tela) {
  free(tela->image);
  tela->image = NULL;
  free(tela);
}