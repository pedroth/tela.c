#include "Window.h"
#include "Tela.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
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

  const u32 pixel_count = (u32)(window->width * window->height);
  /* Convert float image data to ARGB8888 format
     Compute mapping from window (i=row, j=col) to tela (tx, ty) and
     account for color channels when indexing `tela->image`. */
  for (u32 k = 0; k < pixel_count; k++) {
    u32 i = k / window->width;   // row
    u32 j = k % window->width;   // column
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

    /* Pack as ARGB8888 */
    u32 window_index = i * window->width + j;
    window->pixels[window_index] = (a << 24) | (r << 16) | (g << 8) | b;
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
  SDL_Quit();
  free(window);
}