#ifndef WINDOW_H
#define WINDOW_H

#include <SDL2/SDL.h>
#include "../Utils/types.h"
#include "Tela.h"

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

  void (*on_mouse_down_callback)(Window *window, i32 x, i32 y, u32 button,
                                 void *context);
  void *on_mouse_down_context;

  void (*on_mouse_up_callback)(Window *window, i32 x, i32 y, u32 button,
                               void *context);
  void *on_mouse_up_context;

  void (*on_mouse_move_callback)(Window *window, i32 x, i32 y, void *context);
  void *on_mouse_move_context;

  void (*on_mouse_scroll_callback)(Window *window, i32 scroll_y, void *context);
  void *on_mouse_scroll_context;
};

Window *new_window(i32 width, i32 height, const char *title);
Window *set_window_title(Window *window, const char *title);
Window *paint_window(Window *window, Tela *tela);
Window *on_close_window(Window *window, void (*callback)(Window *, void *),
                        void *context);
Window *on_mouse_down_window(Window *window,
                             void (*callback)(Window *, i32, i32, u32, void *),
                             void *context);
Window *on_mouse_up_window(Window *window,
                           void (*callback)(Window *, i32, i32, u32, void *),
                           void *context);
Window *on_mouse_move_window(Window *window,
                             void (*callback)(Window *, i32, i32, void *),
                             void *context);
Window *on_mouse_scroll_window(Window *window,
                               void (*callback)(Window *, i32, void *),
                               void *context);

void free_window(Window *window);

#endif // WINDOW_H
