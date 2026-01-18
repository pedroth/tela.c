#include "io.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

void tela_to_p3(Tela *tela, const char *filename) {
  if (!tela || !tela->image) {
    return;
  }
  const u32 width = (u32)tela->width;
  const u32 height = (u32)tela->height;
  const u32 channels = tela->channels;
  f32 *pixel_data = tela->image;

  FILE *file = fopen(filename, "w");
  if (!file) {
    return;
  }

  // Write P3 header
  fprintf(file, "P3\n");
  fprintf(file, "%u %u\n", width, height);
  fprintf(file, "255\n");

  for(u32 k = 0; k < width * height * channels; k += channels) {
    // Clamp and convert to [0,255]
    u8 r = (u8)(fminf(fmaxf(pixel_data[k + 0], 0.0f), 1.0f) * 255.0f);
    u8 g = (u8)(fminf(fmaxf(pixel_data[k + 1], 0.0f), 1.0f) * 255.0f);
    u8 b = (u8)(fminf(fmaxf(pixel_data[k + 2], 0.0f), 1.0f) * 255.0f);
    fprintf(file, "%u %u %u ", r, g, b);
  }

  fclose(file);
}

void tela_to_image(Tela *tela, const char *filename) {
  // transform into p3 then use ffmpeg to convert to image format
  const char *temp_ppm = "temp_output.ppm";
  tela_to_p3(tela, temp_ppm);
  char command[256];
  snprintf(command, sizeof(command), "ffmpeg -y -i %s %s", temp_ppm, filename);
  printf("Executing command: %s\n", command);
  int ret = system(command);
  if (ret == 0) {
    remove(temp_ppm);
  }
}

char *io_read_file(const char *filename) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  long length = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *buffer = (char *)malloc(length + 1);
  if (!buffer) {
    fclose(file);
    return NULL;
  }

  fread(buffer, 1, length, file);
  buffer[length] = '\0';

  fclose(file);
  return buffer;
}