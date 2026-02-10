#include "../src/index.c"

#define WIDTH 1280
#define HEIGHT 720

#define N_SIM 100
#define AMP 10.0f
#define FRICTION 0.1f
#define WAVE_SCALAR_SPEED 10.0f
#define SPREAD 100.0f

u32 mod(u32 n, u32 m) { return (n % m + m) % m; }

f32 wave[N_SIM][N_SIM];
f32 wave_speed[N_SIM][N_SIM];

void initialize_wave() {
  for (u32 i = 0; i < N_SIM; i++) {
    for (u32 j = 0; j < N_SIM; j++) {
      f32 x = (j - (N_SIM / 2.0f)) / (f32)N_SIM;
      f32 y = (i - (N_SIM / 2.0f)) / (f32)N_SIM;
      wave[i][j] = (AMP * exp(-SPREAD * ((x - 0.25f) * (x - 0.25f) + y * y)) +
                    AMP * exp(-SPREAD * ((x + 0.25f) * (x + 0.25f) + y * y)) +
                    AMP * exp(-SPREAD * (x * x + (y - 0.25f) * (y - 0.25f))));
      wave_speed[i][j] = 0.0f;
    }
  }
}

typedef struct {
  Tela *tela;
  Window *window;
} AnimeContext;

typedef struct {
  f32 min_wave;
  f32 max_wave;
  f32 max_abs_speed;
} ShaderContext;

Color shader(u32 x, u32 y, const void *context) {
  ShaderContext *shader_context = (ShaderContext *)context;
  f32 min_wave = shader_context->min_wave;
  f32 max_wave = shader_context->max_wave;
  f32 max_abs_speed = shader_context->max_abs_speed;

  u32 xi = x;
  u32 yi = y;
  f32 red_color = (wave[yi][xi] - min_wave) / (max_wave - min_wave);
  f32 blue_color = 1 - (wave[yi][xi] - min_wave) / (max_wave - min_wave);
  f32 green_color = fabsf(wave_speed[yi][xi]) / max_abs_speed;
  return (Color){red_color, green_color, blue_color, 1.0f};
}

void anime_lambda(f32 dt, f32 time, void *context) {
  AnimeContext *anime_context = (AnimeContext *)context;

  f32 max_wave = -__FLT_MAX__;
  f32 min_wave = __FLT_MAX__;
  f32 max_abs_speed = -__FLT_MAX__;
  // update wave
  for (u32 i = 0; i < N_SIM; i++) {
    for (u32 j = 0; j < N_SIM; j++) {
      /**
       * Sympletic integration
       */
      // compute acceleration
      f32 laplacian = wave[i][mod(j + 1, N_SIM)] + wave[i][mod(j - 1, N_SIM)] +
                      wave[mod(i + 1, N_SIM)][j] + wave[mod(i - 1, N_SIM)][j] -
                      4 * wave[i][j];
      f32 acceleration =
          WAVE_SCALAR_SPEED * laplacian - FRICTION * wave_speed[i][j];

      // update speed
      wave_speed[i][j] = wave_speed[i][j] + dt * acceleration;

      // update position
      wave[i][j] = wave[i][j] + dt * wave_speed[i][j];

      // get max min values of wave
      max_wave = max_wave <= wave[i][j] ? wave[i][j] : max_wave;
      min_wave = min_wave > wave[i][j] ? wave[i][j] : min_wave;
      f32 abs_speed = fabsf(wave_speed[i][j]);
      max_abs_speed = max_abs_speed <= abs_speed ? abs_speed : max_abs_speed;
    }
  }
  ShaderContext shader_context = {min_wave, max_wave, max_abs_speed};
  Tela *tela = map_tela(anime_context->tela, shader, &shader_context);
  set_window_title(anime_context->window,
                   format_string("FPS: %.2f", 1.0f / dt));
  paint_window(anime_context->window, tela);
}

void on_close_lambda(Window *window, void *context) {
  Loop *anime_loop = (Loop *)context;
  stop_loop(anime_loop);
}

bool is_mouse_down = false;
void mouse_down(Window *window, i32 x, i32 y, u32 button, void *context) {
  is_mouse_down = true;
  printf("Mouse down at (%d, %d, %u)\n", x, y, button);
}
void mouse_up(Window *window, i32 x, i32 y, u32 button, void *context) {
  is_mouse_down = false;
}
void mouse_move(Window *window, i32 x, i32 y, void *context) {
  if (!is_mouse_down)
    return;
  const u32 xi = (u32)((x / (f32)WIDTH) * N_SIM);
  const u32 yi = (u32)((y / (f32)HEIGHT) * N_SIM);
  const u32 i = mod(yi - N_SIM + 1, N_SIM);
  const u32 j = mod(xi, N_SIM);
  i32 steps[] = {-1, 0, 1};
  // To use a bigger brush, change the declaration above to:
  // i32 steps[] = { -2, -1, 0, 1, 2 };
  // brush
  const u32 n = sizeof(steps) / sizeof(steps[0]); // equals to steps length in C
  const u32 nn = n * n;
  for (u32 k = 0; k < nn; k++) {
    const u32 u = k / n;
    const u32 v = k % n;
    wave[mod(i + steps[u], N_SIM)][mod(j + steps[v], N_SIM)] = AMP;
  }
}

void handle_window_events(Window *window) {
  /* register handlers on the window */
  on_mouse_down_window(window, mouse_down, NULL);
  on_mouse_up_window(window, mouse_up, NULL);
  on_mouse_move_window(window, mouse_move, NULL);
}

int main() {
  initialize_wave();
  Tela *tela = new_tela(N_SIM, N_SIM);
  Window *window = new_window(WIDTH, HEIGHT, "Amazing Shader");
  AnimeContext anime_context = {tela, window};
  Loop *anime_loop = loop(anime_lambda, &anime_context);
  on_close_window(window, on_close_lambda, anime_loop);
  handle_window_events(window);
  // must be last function to be called in main
  play_loop(anime_loop);
  return 0;
}
