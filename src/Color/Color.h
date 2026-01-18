#ifndef COLOR_H
#define COLOR_H

#include "../Utils/types.h"

typedef struct {
  f32 red;
  f32 green;
  f32 blue;
  f32 alpha;
} Color;

// Constructors
Color* new_color(f32 r, f32 g, f32 b);
Color* new_color_rgba(f32 r, f32 g, f32 b, f32 a);
void free_color(Color* color);
bool equal_colors(Color *c1, Color *c2);

#endif // COLOR_H