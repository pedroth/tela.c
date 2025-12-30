#ifndef WINDOW_H
#define WINDOW_H

#include "../Utils/types.h"
#include "Tela.h"
#include <SDL2/SDL.h>

typedef struct {
  i32 width;
  i32 height;
  char *title;
  SDL_Window *sdl_window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  u32 *pixels;
} Window;

Window *new_window(i32 width, i32 height, const char *title);
Window *set_window_title(Window *window, const char *title);
Window *paint_window(Window *window, Tela *tela);
void free_window(Window *window);

#endif // WINDOW_H
