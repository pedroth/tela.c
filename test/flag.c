/**
 * Flag Simulation
 *
 * Ports the JS flag physics simulation demo to tela.c.
 * Controls:
 *   - Left Drag: Interact with flag
 *   - Right Drag: Rotate camera
 *   - Mouse Wheel: Zoom camera
 */

 // gcc -O3 -fopenmp -o app test/flag.c -lSDL2 -lm && ./app
 // gcc -O3 -fopenmp -ffast-math -o app test/flag.c -lSDL2 -lm -march=native && ./app

#include "../src/index.c"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const u32 WIDTH = 640;
static const u32 HEIGHT = 480;
#define FLAG_SIZE 25
#define NUM_TRIANGLES ((FLAG_SIZE - 1) * (FLAG_SIZE - 1) * 2)

typedef struct {
  u32 row[3];
  u32 col[3];
} TriangleIndex;

typedef struct {
  Tela* tela;
  Window* window;
  Camera camera;
  Scene scene;

  Vec3 positions[FLAG_SIZE][FLAG_SIZE];
  Vec3 rest_positions[FLAG_SIZE][FLAG_SIZE];
  Vec3 velocities[FLAG_SIZE][FLAG_SIZE];

  TriangleIndex triangle_indices[NUM_TRIANGLES];
  u32 num_triangles;

  bool left_mouse_down;
  bool right_mouse_down;
  Vec2 mouse;
  bool has_intersection_point;
  Vec3 intersection_point;
} App;

static void init_flag(App* app, Tela* texture) {
  f32 size_minus_1 = (f32)(FLAG_SIZE - 1);

  for (u32 i = 0; i < FLAG_SIZE; i++) {
    for (u32 j = 0; j < FLAG_SIZE; j++) {
      f32 y = (f32)j / size_minus_1 - 0.5f;
      f32 z = (f32)i / size_minus_1 - 0.5f;
      app->positions[i][j] = vec3(0.0f, 3.0f * y, 3.0f * z);
      app->rest_positions[i][j] = app->positions[i][j];
      app->velocities[i][j] = vec3(0.0f, 0.0f, 0.0f);
    }
  }

  u32 tri_idx = 0;
  Color white = { 1.0f, 1.0f, 1.0f, 1.0f };

  for (u32 i = 0; i < FLAG_SIZE - 1; i++) {
    for (u32 j = 0; j < FLAG_SIZE - 1; j++) {
      // First triangle
      RasterTriangleProps* props1 =
          (RasterTriangleProps*)malloc(sizeof(RasterTriangleProps));
      props1->colors[0] = white;
      props1->colors[1] = white;
      props1->colors[2] = white;
      props1->tex_coords[0] = vec2((f32)j / size_minus_1, (f32)i / size_minus_1);
      props1->tex_coords[1] =
          vec2((f32)j / size_minus_1, (f32)(i + 1) / size_minus_1);
      props1->tex_coords[2] =
          vec2((f32)(j + 1) / size_minus_1, (f32)i / size_minus_1);
      props1->texture = texture;
      props1->material = NULL;

      Triangle tri1 = build_triangle(
          app->positions[i][j],
          app->positions[i + 1][j],
          app->positions[i][j + 1]
      );
      tri1.props = props1;
      add_scene_elem_scene(&app->scene, build_scene_elem_triangle(tri1));

      app->triangle_indices[tri_idx] = (TriangleIndex){
        .row = { i, i + 1, i },
        .col = { j, j, j + 1 }
      };
      tri_idx++;

      // Second triangle
      RasterTriangleProps* props2 =
          (RasterTriangleProps*)malloc(sizeof(RasterTriangleProps));
      props2->colors[0] = white;
      props2->colors[1] = white;
      props2->colors[2] = white;
      props2->tex_coords[0] =
          vec2((f32)j / size_minus_1, (f32)(i + 1) / size_minus_1);
      props2->tex_coords[1] =
          vec2((f32)(j + 1) / size_minus_1, (f32)(i + 1) / size_minus_1);
      props2->tex_coords[2] =
          vec2((f32)(j + 1) / size_minus_1, (f32)i / size_minus_1);
      props2->texture = texture;
      props2->material = NULL;

      Triangle tri2 = build_triangle(
          app->positions[i + 1][j],
          app->positions[i + 1][j + 1],
          app->positions[i][j + 1]
      );
      tri2.props = props2;
      add_scene_elem_scene(&app->scene, build_scene_elem_triangle(tri2));

      app->triangle_indices[tri_idx] = (TriangleIndex){
        .row = { i + 1, i + 1, i },
        .col = { j, j + 1, j + 1 }
      };
      tri_idx++;
    }
  }
  app->num_triangles = tri_idx;
}

static Vec3 add_tension_spring(App* app, u32 i, u32 j, u32 neighbor_i, u32 neighbor_j) {
  f32 stiffness = 150.0f;
  Vec3 displacement = sub_vec3(app->positions[neighbor_i][neighbor_j], app->positions[i][j]);
  f32 len = length_vec3(displacement);
  Vec3 rest_disp = sub_vec3(app->rest_positions[neighbor_i][neighbor_j], app->rest_positions[i][j]);
  f32 rest_len = length_vec3(rest_disp);
  if (len > rest_len && len > 1e-6f) {
    return scale_vec3(displacement, stiffness * (len - rest_len) / len);
  }
  return vec3(0.0f, 0.0f, 0.0f);
}

static Vec3 compute_acceleration(App* app, u32 i, u32 j) {
  Vec3 g = vec3(0.0f, 0.0f, -1.0f);
  f32 friction = 2.0f;
  Vec3 spring_force = vec3(0.0f, 0.0f, 0.0f);

  if (i > 0)
    spring_force = add_vec3(spring_force, add_tension_spring(app, i, j, i - 1, j));
  if (i < FLAG_SIZE - 1)
    spring_force = add_vec3(spring_force, add_tension_spring(app, i, j, i + 1, j));
  if (j > 0)
    spring_force = add_vec3(spring_force, add_tension_spring(app, i, j, i, j - 1));
  if (j < FLAG_SIZE - 1)
    spring_force = add_vec3(spring_force, add_tension_spring(app, i, j, i, j + 1));

  Vec3 mouse_force = vec3(0.0f, 0.0f, 0.0f);
  if (app->has_intersection_point) {
    Vec3 displacement =
        sub_vec3(app->intersection_point, app->positions[i][j]);
    f32 sq_length = dot_vec3(displacement, displacement);
    if (sq_length > 1e-6f) {
      mouse_force = add_vec3(
          mouse_force, scale_vec3(displacement, 10.0f / sq_length)
      );
    }
  }

  Vec3 total = add_vec3(g, spring_force);
  total = add_vec3(total, mouse_force);
  Vec3 friction_force = scale_vec3(app->velocities[i][j], friction);
  return sub_vec3(total, friction_force);
}

static f32 g_simulation_time = 0.0f;

static void flag_physics(App* app, f32 dt) {
  f32 max_step = 1.0f / 240.0f;
  i32 steps = (i32)ceilf(dt / max_step);
  if (steps < 1)
    steps = 1;
  f32 step_dt = dt / (f32)steps;

  for (i32 step = 0; step < steps; step++) {
    g_simulation_time += step_dt;
    for (u32 i = 0; i < FLAG_SIZE; i++) {
      app->positions[i][0] = app->rest_positions[i][0];
      app->velocities[i][0] = vec3(0.0f, 0.0f, 0.0f);
      for (u32 j = 1; j < FLAG_SIZE; j++) {
        Vec3 accel = compute_acceleration(app, i, j);
        app->velocities[i][j] =
            add_vec3(app->velocities[i][j], scale_vec3(accel, step_dt));
        app->positions[i][j] = add_vec3(
            app->positions[i][j], scale_vec3(app->velocities[i][j], step_dt)
        );
      }
    }
  }

  // Update scene triangles positions
  Array* elems = get_scene_elems_scene(&app->scene);
  for (u32 k = 0; k < app->num_triangles; k++) {
    SceneElem* elem = (SceneElem*)get_array_element(elems, k);
    Triangle* triangle = &elem->as.triangle;
    TriangleIndex idx = app->triangle_indices[k];
    triangle->positions[0] = app->positions[idx.row[0]][idx.col[0]];
    triangle->positions[1] = app->positions[idx.row[1]][idx.col[1]];
    triangle->positions[2] = app->positions[idx.row[2]][idx.col[2]];
  }
}

static void on_frame(f32 dt, f32 time, void* ctx) {
  (void)time;
  App* app = (App*)ctx;

  raster_scene(
      &app->scene,
      (RasterParams){
          .camera = &app->camera,
          .tela = app->tela,
          .cull_backfaces = false,
          .bilinear_texture = true,
          .clip_camera_plane = true,
          .clear_screen = true,
          .background_color = (Color){ 0.0f, 0.0f, 0.0f, 1.0f },
          .perspective_correct = true,
      }
  );

  flag_physics(app, dt);

  u32 fps = (dt > 0.0f) ? (u32)(1.0f / dt) : 0;
  set_window_title(
      app->window,
      format_string(
          "Flag Simulation | Left Click: Interact | Right Click: Rotate Camera | FPS: %u",
          fps
      )
  );

  paint_window(app->window, app->tela);
}

static void on_close(Window* window, void* ctx) {
  (void)window;
  Loop* animation = (Loop*)ctx;
  stop_loop(animation);
}

static void update_intersection(App* app, i32 x, i32 y) {
  Ray ray = ray_from_tela_camera(&app->camera, app->tela, (u32)x, (u32)y);
  Ray center_ray =
      ray_from_tela_camera(&app->camera, app->tela, WIDTH / 2, HEIGHT / 2);
  Vec3 normal = center_ray.dir;
  f32 denom = dot_vec3(normal, ray.dir);
  if (fabsf(denom) > 1e-6f) {
    f32 t = -dot_vec3(normal, ray.init) / denom;
    app->intersection_point = trace_ray(ray, t);
    app->has_intersection_point = true;
  }
}

static void on_mouse_down(Window* window, i32 x, i32 y, u32 button, void* ctx) {
  (void)window;
  App* app = (App*)ctx;
  app->mouse = vec2((f32)x, (f32)y);
  if (button == SDL_BUTTON_LEFT) {
    app->left_mouse_down = true;
    update_intersection(app, x, y);
  } else {
    app->right_mouse_down = true;
  }
}

static void on_mouse_up(Window* window, i32 x, i32 y, u32 button, void* ctx) {
  (void)window;
  (void)x;
  (void)y;
  (void)button;
  App* app = (App*)ctx;
  app->left_mouse_down = false;
  app->right_mouse_down = false;
  app->has_intersection_point = false;
  app->mouse = vec2(0.0f, 0.0f);
}

static void on_mouse_move(Window* window, i32 x, i32 y, void* ctx) {
  (void)window;
  App* app = (App*)ctx;
  Vec2 new_mouse = vec2((f32)x, (f32)y);
  f32 dx = new_mouse.x - app->mouse.x;
  f32 dy = new_mouse.y - app->mouse.y;

  if (app->left_mouse_down) {
    update_intersection(app, x, y);
  }

  if (app->right_mouse_down) {
    Vec3 orbit = get_camera_orbit(&app->camera);
    f32 new_theta = orbit.y - 2.0f * (f32)M_PI * (dx / (f32)WIDTH);
    f32 new_phi = orbit.z - 2.0f * (f32)M_PI * (dy / (f32)HEIGHT);
    set_orbit_camera(&app->camera, orbit.x, new_theta, new_phi);
  }

  app->mouse = new_mouse;
}

static void on_mouse_scroll(Window* window, i32 delta_y, void* ctx) {
  (void)window;
  App* app = (App*)ctx;
  Vec3 orbit = get_camera_orbit(&app->camera);
  f32 new_radius = orbit.x + (f32)delta_y * 0.1f;
  set_orbit_camera(&app->camera, new_radius, orbit.y, orbit.z);
}

static void register_input_handlers(Window* window, App* app) {
  on_mouse_down_window(window, on_mouse_down, app);
  on_mouse_up_window(window, on_mouse_up, app);
  on_mouse_move_window(window, on_mouse_move, app);
  on_mouse_scroll_window(window, on_mouse_scroll, app);
}

int main(void) {
  Tela* tela = new_tela(WIDTH, HEIGHT);
  Window* window = new_window(WIDTH, HEIGHT, "Flag Simulation");

  App app = {
    .tela = tela,
    .window = window,
    .camera = create_camera(vec3(5.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 1.0f),
    .scene = new_naive_scene(),
    .left_mouse_down = false,
    .right_mouse_down = false,
    .mouse = vec2(0.0f, 0.0f),
    .has_intersection_point = false,
    .intersection_point = vec3(0.0f, 0.0f, 0.0f)
  };

  set_orbit_camera(&app.camera, 5.0f, 0.0f, (f32)M_PI / 6.0f);

  Tela* texture = io_read_image("./assets/chapelle.jpg");

  init_flag(&app, texture);

  Loop* animation = loop(on_frame, &app);

  on_close_window(window, on_close, animation);
  register_input_handlers(window, &app);

  play_loop(animation);

  free_scene(&app.scene);

  return 0;
}