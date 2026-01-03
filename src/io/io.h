#ifndef IO_H
#define IO_H

#include "../Tela/Tela.h"
#include "../Utils/types.h"

// Write a plain P3 (ASCII) PPM file
void tela_to_p3(Tela *tela, const char *filename);

// Write an image. If stb_image_write is available, define USE_STB to enable
// PNG output; otherwise this falls back to writing a P3 file.
void tela_to_image(Tela *tela, const char *filename);

char *io_read_file(const char *filename);

#endif // IO_H