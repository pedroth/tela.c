#include "Window.h"
#include "Tela.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void transform_mouse_coordinates(Window *window, i32 x, i32 y, i32 out[2]) {
  i32 transformed_x = x;
  i32 transformed_y = window->height - 1 - y;
  out[0] = transformed_x;
  out[1] = transformed_y;
}

void process_window_events(Window *window) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT ||
        (event.type == SDL_WINDOWEVENT &&
         event.window.event == SDL_WINDOWEVENT_CLOSE)) {

      // This is where your callback gets triggered
      if (window->on_close_callback) {
        window->on_close_callback(window, window->on_close_context);
      }
    } else if (event.type == SDL_MOUSEBUTTONDOWN) {
      if (window->on_mouse_down_callback) {
        i32 transformed_coords[2];
        transform_mouse_coordinates(window, event.button.x, event.button.y,
                                    transformed_coords);
        window->on_mouse_down_callback(
            window, transformed_coords[0], transformed_coords[1],
            event.button.button, window->on_mouse_down_context);
      }
    } else if (event.type == SDL_MOUSEBUTTONUP) {
      if (window->on_mouse_up_callback) {
        i32 transformed_coords[2];
        transform_mouse_coordinates(window, event.button.x, event.button.y,
                                    transformed_coords);
        window->on_mouse_up_callback(window, transformed_coords[0],
                                     transformed_coords[1], event.button.button,
                                     window->on_mouse_up_context);
      }
    } else if (event.type == SDL_MOUSEMOTION) {
      if (window->on_mouse_move_callback) {
        i32 transformed_coords[2];
        transform_mouse_coordinates(window, event.motion.x, event.motion.y,
                                    transformed_coords);
        window->on_mouse_move_callback(window, transformed_coords[0],
                                       transformed_coords[1],
                                       window->on_mouse_move_context);
      }
    } else if (event.type == SDL_MOUSEWHEEL) {
      if (window->on_mouse_scroll_callback) {
        window->on_mouse_scroll_callback(window, event.wheel.y,
                                         window->on_mouse_scroll_context);
      }
    }
  }
}

Window *new_window(i32 width, i32 height, const char *title) {
  Window *window = (Window *)malloc(sizeof(Window));
  window->width = width;
  window->height = height;
  window->title = (char *)malloc(strlen(title) + 1);
  window->pixels = (u32 *)malloc(width * height * sizeof(u32));
  strcpy(window->title, title);

  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
    return NULL;
  }

  // Create SDL window
  SDL_Window *sdl_window =
      SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       width, height, SDL_WINDOW_SHOWN);
  // Initialize renderer and texture
  SDL_Renderer *renderer = SDL_CreateRenderer(
      sdl_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                           SDL_TEXTUREACCESS_STREAMING,
                                           window->width, window->height);
  if (!sdl_window || !renderer || !texture) {
    fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(sdl_window);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(texture);
    window->sdl_window = NULL;
    return NULL;
  }
  window->sdl_window = sdl_window;
  window->renderer = renderer;
  window->texture = texture;
  return window;
}

Window *paint_window(Window *window, Tela *tela) {
  if (!window || !window->sdl_window || !tela || !tela->image) {
    return window;
  }

  process_window_events(window);

  const u32 pixel_count = (u32)(window->width * window->height);
  // Convert float image data to RGBA8888 format
  for (u32 k = 0; k < pixel_count; k++) {
    u32 i = k / window->width; // row
    u32 j = k % window->width; // column
    /* normalized coordinates in [0,1) */
    f32 nx = (f32)j / (f32)window->width;  /* x fraction */
    f32 ny = (f32)i / (f32)window->height; /* y fraction */
    u32 tx = (u32)(nx * (f32)tela->width);
    u32 ty = (u32)(ny * (f32)tela->height);
    const u32 channels = tela->channels;
    u32 tela_pixel = ty * tela->width + tx;
    u32 tela_index = tela_pixel * channels;

    u32 r = 0, g = 0, b = 0, a = 255;
    /* RGBA values are floats in [0,1] stored per-channel */
    r = (u8)(tela->image[tela_index + 0] * 255.0f);
    g = (u8)(tela->image[tela_index + 1] * 255.0f);
    b = (u8)(tela->image[tela_index + 2] * 255.0f);
    a = (u8)(tela->image[tela_index + 3] * 255.0f);

    /* Pack as RGBA8888 */
    u32 window_index = i * window->width + j;
    window->pixels[window_index] = (r << 24) | (g << 16) | (b << 8) | a;
  }

  // Update texture with converted pixel data
  SDL_UpdateTexture(window->texture, NULL, window->pixels,
                    window->width * sizeof(u32));

  // Render
  SDL_RenderClear(window->renderer);
  SDL_RenderCopy(window->renderer, window->texture, NULL, NULL);
  SDL_RenderPresent(window->renderer);
  return window;
}

Window *set_window_title(Window *window, const char *title) {
  if (!window || !window->sdl_window) {
    return window;
  }
  free(window->title);
  window->title = (char *)malloc(strlen(title) + 1);
  strcpy(window->title, title);
  SDL_SetWindowTitle(window->sdl_window, title);
  return window;
}

Window *on_close_window(Window *window, void (*callback)(Window *, void *),
                        void *context) {
  if (!window) {
    return window;
  }
  window->on_close_callback = callback;
  window->on_close_context = context;
  return window;
}

Window *on_mouse_down_window(Window *window,
                             void (*callback)(Window *, i32, i32, u32, void *),
                             void *context) {
  if (!window) {
    return window;
  }
  window->on_mouse_down_callback = callback;
  window->on_mouse_down_context = context;
  return window;
}

Window *on_mouse_up_window(Window *window,
                           void (*callback)(Window *, i32, i32, u32, void *),
                           void *context) {
  if (!window) {
    return window;
  }
  window->on_mouse_up_callback = callback;
  window->on_mouse_up_context = context;
  return window;
}

Window *on_mouse_move_window(Window *window,
                             void (*callback)(Window *, i32, i32, void *),
                             void *context) {
  if (!window) {
    return window;
  }
  window->on_mouse_move_callback = callback;
  window->on_mouse_move_context = context;
  return window;
}

Window *on_mouse_scroll_window(Window *window,
                               void (*callback)(Window *, i32, void *),
                               void *context) {
  if (!window) {
    return window;
  }
  window->on_mouse_scroll_callback = callback;
  window->on_mouse_scroll_context = context;
  return window;
}

void free_window(Window *window) {
  if (window->pixels) {
    free(window->pixels);
    window->pixels = NULL;
  }
  if (window->sdl_window) {
    SDL_DestroyWindow(window->sdl_window);
    window->sdl_window = NULL;
  }
  if (window->renderer) {
    SDL_DestroyRenderer(window->renderer);
    window->renderer = NULL;
  }
  if (window->texture) {
    SDL_DestroyTexture(window->texture);
    window->texture = NULL;
  }
  if (window->title) {
    free(window->title);
    window->title = NULL;
  }
  /* Clear any callback pointers/contexts to avoid dangling references */
  window->on_close_callback = NULL;
  window->on_close_context = NULL;
  window->on_mouse_down_callback = NULL;
  window->on_mouse_down_context = NULL;
  window->on_mouse_up_callback = NULL;
  window->on_mouse_up_context = NULL;
  window->on_mouse_move_callback = NULL;
  window->on_mouse_move_context = NULL;
  window->on_mouse_scroll_callback = NULL;
  window->on_mouse_scroll_context = NULL;

  /* Only quit SDL if it was initialized by this process */
  if (SDL_WasInit(SDL_INIT_VIDEO)) {
    SDL_Quit();
  }

  free(window);
}