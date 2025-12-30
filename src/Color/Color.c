#include "Color.h"
#include <stdlib.h>

Color* new_color(f32 r, f32 g, f32 b) {
  Color* color = (Color*)malloc(sizeof(Color));
  color->red = r;
  color->green = g;
  color->blue = b;
  color->alpha = 1.0f;
  return color;
}

Color* new_color_rgba(f32 r, f32 g, f32 b, f32 a) {
  Color* color = (Color*)malloc(sizeof(Color));
  color->red = r;
  color->green = g;
  color->blue = b;
  color->alpha = a;
  return color;
}

void free_color(Color* color) {
  free(color);
}

bool_t equal_colors(Color *c1, Color *c2) {
  return (c1->red == c2->red) && (c1->green == c2->green) && (c1->blue == c2->blue) &&
         (c1->alpha == c2->alpha);
}
