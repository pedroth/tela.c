#ifndef TELA_H
#define TELA_H

#include "../Utils/types.h"
#include "../Color/Color.h"

typedef struct {
    i32 width;
    i32 height;
    i32 channels;
    f32* image;
} Tela;

#define COLOR_CHANNELS 4

// Tela API
Tela* new_tela(u32 width, u32 height);
Tela* map_tela(Tela *tela, Color* (*func)(u32, u32, void const *), void const *context);
void free_tela(Tela *tela);

#endif // TELA_H