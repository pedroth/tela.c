#include "Camera/Camera.h"
#include "Color/Color.h"
#include "Geometry/Mesh.h"
#include "Geometry/Triangle.h"
#include "Scene/Scene.h"
#include "Tela/Tela.h"
#include "Tela/Window.h"
#include "Vector/Vec2.h"
#include "Vector/Vec3.h"
#include "io/io.h"
#include "utils/loop.h"
#include "utils/string.h"
#include "utils/time.h"
#include "utils/types.h"
#include <math.h>

#define WIDTH 1280
#define HEIGHT 720
#define MAX_RADIUS 3.0f
#define PI 3.14159265358979323846f

typedef struct {
  Tela *tela;
  Window *window;
  Camera *camera;
  Scene *scene;
} AnimeContext;

void anime_lambda(f32 dt, f32 time, void *context) {
  AnimeContext *anime_context = (AnimeContext *)context;
  set_window_title(anime_context->window,
                   format_string("FPS: %.2f", 1.0f / dt));
  raster_scene(anime_context->camera, anime_context->scene,
               anime_context->tela);
  paint_window(anime_context->window, anime_context->tela);
}

void on_close_lambda(Window *window, void *context) {
  Loop *anime_loop = (Loop *)context;
  stop_loop(anime_loop);
}

int is_mouse_down = false;
Vec2 mouse;

void mouse_down(Window *window, i32 x, i32 y, u32 button, void *context) {
  is_mouse_down = true;
  mouse = vec2((f32)x, (f32)y);
}
void mouse_up(Window *window, i32 x, i32 y, u32 button, void *context) {
  is_mouse_down = false;
  mouse = vec2(0, 0);
}
void mouse_move(Window *window, i32 x, i32 y, void *context) {
  Camera *camera = (Camera *)context;
  Vec2 new_mouse = vec2((f32)x, (f32)y);
  if (!is_mouse_down || equals_vec2(new_mouse, mouse)) {
    return;
  }
  Vec2 delta = sub_vec2(new_mouse, mouse);
  Vec3 newOrbit = add_vec3(get_camera_orbit(camera),
                           vec3(0.0f, -2.0f * PI * (delta.x / WIDTH),
                                2.0f * PI * (delta.y / HEIGHT)));
  set_orbit_camera(camera, newOrbit.x, newOrbit.y, newOrbit.z);
  mouse = new_mouse;
}

void mouse_scroll(Window *window, i32 scroll_y, void *context) {
  // Implement zooming functionality here if needed
}

void handle_window_events(Window *window, Camera *camera) {
  /* register handlers on the window */
  on_mouse_down_window(window, mouse_down, NULL);
  on_mouse_up_window(window, mouse_up, NULL);
  on_mouse_move_window(window, mouse_move, camera);
  on_mouse_scroll_window(window, mouse_scroll, NULL);
}

f32 get_max_fold(f32 a, f32 b) { return a > b ? a : b; }

Vec3 transform_mesh1(const Vec3 v) { return scale_vec3(v, 2.0f); }
Vec3 transform_mesh2(const Vec3 v) {
  return sub_vec3(v, vec3(0.0f, -0.5, 0.0f));
}
Vec3 transform_mesh3(const Vec3 v) { return vec3(v.x, v.z, v.y); }

int main() {
  NaiveScene scene = {0};
  Camera camera = {.position = vec3(3, 0, 0),
                   .look_at = vec3(0, 0, 0),
                   .distance_to_plane = 1.0f};
  set_orbit_camera(&camera, MAX_RADIUS, PI, 0.0f);

  Triangle triangle = build_triangle(
      "triangle1",
      (Vec3[]){vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f),
               vec3(0.0f, 0.0f, 1.0f)},
      NULL, (Vec2[]){vec2(0.0f, 0.0f), vec2(1.0f, 0.0f), vec2(0.5f, 1.0f)},
      (Color[]){color_from_rgb(1, 0, 0), color_from_rgb(0, 1, 0),
                color_from_rgb(0, 0, 1)},
      NULL);
  add_triangle_to_scene(&scene, &triangle);
  Window *window = new_window(WIDTH, HEIGHT, "Mesh");
  Tela *tela = new_tela(WIDTH, HEIGHT);
  AnimeContext anime_context = {tela, window, &camera, &scene};
  Loop *anime_loop = loop(anime_lambda, &anime_context);
  on_close_window(window, on_close_lambda, anime_loop);
  handle_window_events(window, &camera);
  // must be last function to be called in main
  play_loop(anime_loop);
}

int main0() {
  // NaiveScene scene = {0};
  Camera camera = {.position = vec3(3, 0, 0),
                   .look_at = vec3(0, 0, 0),
                   .distance_to_plane = 1.0f};
  set_orbit_camera(&camera, MAX_RADIUS, PI, 0.0f);

  char *obj_file = io_read_file("assets/megaman.obj");
  Mesh mesh = parse_obj_mesh(obj_file, "mesh");
  free(obj_file);
  AABB *box = get_mesh_aabb(mesh);
  // f32 scale_inv =
  //     2.0f / fold_vec(get_aabb_diagonal(box), get_max_fold, -INFINITY);
  // map_vertices_mesh(mesh, transform_mesh1);
  // map_vertices_mesh(mesh, transform_mesh2);
  // map_vertices_mesh(mesh, transform_mesh3);
  // add_texture_to_mesh(mesh, "test/assets/mesh/bunny_texture.png");

  // add_triangles_to_scene(scene, get_triangles(mesh));

  // Window *window = new_window(WIDTH, HEIGHT, "Mesh");
  // Tela *tela = new_tela(WIDTH, HEIGHT);

  // AnimeContext anime_context = {tela, window, &camera, &scene};
  // Loop *anime_loop = loop(anime_lambda, &anime_context);
  // on_close_window(window, on_close_lambda, anime_loop);
  // handle_window_events(window, &camera);
  // // must be last function to be called in main
  // play_loop(anime_loop);
  printf("Obj file content:\n%s\n", obj_file);
  return 0;
}
