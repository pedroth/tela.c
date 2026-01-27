#ifndef TELA_H
#define TELA_H

#include "../Utils/types.h"
#include "../Color/Color.h"
#include "../Geometry/AABB.h"
#include "../Vector/Vec2.h"

typedef struct {
    i32 width;
    i32 height;
    i32 channels;
    f32* image;
    AABB box;
} Tela;

#define COLOR_CHANNELS 4

// Tela API
Tela* new_tela(u32 width, u32 height);
Tela* map_tela(Tela *tela, Color* (*func)(u32, u32, void const *), void const *context);
Tela* fill_tela(Tela *tela, Color color);
Tela* draw_triangle_tela(Tela *tela, Vec2 triangle_pos[3],
                         Color *(*lambda)(u32, u32, void const *),
                         void const *context);
Tela* draw_convex_polygon_tela(Tela *tela, Vec2 *polygon_pos, u32 vertex_count,
                               Color *(*lambda)(u32, u32, void const *),
                               void const *context);
Vec2 canvas2grid(Tela *tela, u32 x, u32 y);
void free_tela(Tela *tela);

#endif // TELA_H