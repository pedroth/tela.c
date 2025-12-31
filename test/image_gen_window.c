#include "Color/Color.h"
#include "Tela/Tela.h"
#include "Tela/Window.h"
#include "utils/types.h"
#include "io/io.h"

const u32 WIDTH = 640;
const u32 HEIGHT = 480;

Color *shader(u32 x, u32 y) {
  f32 fx = (f32)x / (f32)(WIDTH);
  f32 fy = (f32)y / (f32)(HEIGHT);
  return new_color(fx, fy, 0.0f);
}

int main() {
  Tela *tela = new_tela(WIDTH, HEIGHT);
  map_tela(tela, shader, NULL);
  tela_to_image(tela, "output.png");
  return 0;
}
