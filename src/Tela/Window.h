#ifndef WINDOW_H
#define WINDOW_H

#include "../Utils/types.h"
#include "Tela.h"
#include <SDL2/SDL.h>

// need to forward declare because of self-referential callback
typedef struct Window Window;
struct Window {
  i32 width;
  i32 height;
  char *title;
  SDL_Window *sdl_window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  u32 *pixels;
  void (*on_close_callback)(Window *window, void *context);
  void *on_close_context;
};

Window *new_window(i32 width, i32 height, const char *title);
Window *set_window_title(Window *window, const char *title);
Window *paint_window(Window *window, Tela *tela);
Window* on_close_window(Window *window, void (*callback)(Window *, void *), void *context);

void free_window(Window *window);

#endif // WINDOW_H
