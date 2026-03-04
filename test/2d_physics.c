/**
 * 2D Physics Demo
 *
 * Draw closed shapes with the mouse, then watch them fall under gravity
 * with shape-preserving constraints, collision detection, and boundary enforcement.
 *
 * Controls:
 *   mouse-left:  draw a figure
 *   mouse-right: mouse force field (attract vertices)
 *   r:           reset scene
 *   t:           toggle triangle fill mode on/off
 *
 * Compile:
 *   gcc -O3 -o 2d_physics test/2d_physics.c -lSDL2 -lm
 */

#include "../src/index.c"

 // gcc -O3 -o 2d_physics test/2d_physics.c -lSDL2 -lm

 /* =============================================================================
  * Constants
  * ========================================================================== */

#define WIDTH 640
#define HEIGHT 640

  /* =============================================================================
   * Utility
   * ========================================================================== */

static inline i32 mod_i(i32 n, i32 m) {
  return ((n % m) + m) % m;
}

/* =============================================================================
 * Types
 * ========================================================================== */

typedef struct {
  Array points;       // Array of Vec2 - current positions in world coords
  Array speeds;       // Array of Vec2 - per-vertex velocities
  Array shape;        // Array of Vec2 - centered reference shape
} PhysicsBody;

typedef struct {
  Vec2 a, b;
  Color color;
} DebugLine;

typedef struct {
  Tela* tela;
  Window* window;
  Camera_2D camera;
  Array bodies;       // Array of PhysicsBody
  Array draft_points; // Array of Vec2 - current drawing path (pixel coords stored as world)
  Array debug_lines;  // Array of DebugLine - collision debug visualization
  bool is_filled;
} App;

/* =============================================================================
 * Global Mouse State
 * ========================================================================== */

static bool g_mouse_left_down = false;
static bool g_right_mouse_down = false;
static Vec2 g_mouse_world = { 0 };   // mouse position in world coordinates

/* =============================================================================
 * Path Geometry Helpers
 * ========================================================================== */

 /**
  * Remove near-duplicate vertices from a path.
  */
static Array clean_path(const Array* path) {
  const f32 epsilon = 1e-3f;
  Array cleaned = new_array(path->length, sizeof(Vec2));
  for (u32 i = 0; i < path->length; i++) {
    Vec2 pi = *(Vec2*)get_array_element((Array*)path, i);
    bool duplicate = false;
    for (u32 j = 0; j < cleaned.length; j++) {
      Vec2 cj = *(Vec2*)get_array_element(&cleaned, j);
      if (length_vec2(sub_vec2(cj, pi)) < epsilon) {
        duplicate = true;
        break;
      }
    }
    if (!duplicate) {
      push_array(&cleaned, &pi);
    }
  }
  return cleaned;
}

/**
 * Compute the center of a path and return centered vertices.
 */
static Array centered_shape(const Array* path) {
  u32 n = path->length;
  Vec2 center = vec2(0, 0);
  for (u32 i = 0; i < n; i++) {
    center = add_vec2(center, *(Vec2*)get_array_element((Array*)path, i));
  }
  center = scale_vec2(center, 1.0f / (f32)n);
  Array shape = new_array(n, sizeof(Vec2));
  for (u32 i = 0; i < n; i++) {
    Vec2 p = sub_vec2(*(Vec2*)get_array_element((Array*)path, i), center);
    push_array(&shape, &p);
  }
  return shape;
}

/**
 * Point-in-polygon test using ray casting.
 */
static bool is_inside(Vec2 x, const Array* path) {
  u32 n = path->length;
  i32 count = 0;
  for (u32 i = 0; i < n; i++) {
    Vec2 a = *(Vec2*)get_array_element((Array*)path, i);
    Vec2 b = *(Vec2*)get_array_element((Array*)path, mod_i((i32)i + 1, (i32)n));
    Vec2 v = sub_vec2(b, a);
    if (v.y == 0.0f) continue;
    Vec2 r = sub_vec2(x, a);
    f32 t = -wedge_vec2(r, v) / v.y;
    f32 s = r.y / v.y;
    if (t > 0 && s >= 0 && s <= 1) count++;
  }
  return (count % 2) == 1;
}

/**
 * Compute winding orientation of a polygon.
 * Returns +1 or -1.
 */
static f32 compute_path_orientation(const Array* path) {
  u32 n = path->length;
  Vec2 x = vec2(0, 0);
  f32 theta = 0;
  for (u32 j = 0; j + 1 < n; j++) {
    Vec2 pj = *(Vec2*)get_array_element((Array*)path, j);
    Vec2 pj1 = *(Vec2*)get_array_element((Array*)path, j + 1);
    Vec2 u = sub_vec2(x, pj);
    Vec2 v = sub_vec2(x, pj1);
    theta += wedge_vec2(u, v);
  }
  return (0.5f * theta) >= 0 ? 1.0f : -1.0f;
}

/* =============================================================================
 * Physics Body Creation
 * ========================================================================== */

static PhysicsBody create_body(const Array* cleaned_path) {
  PhysicsBody body;
  u32 n = cleaned_path->length;

  // Copy points
  body.points = new_array(n, sizeof(Vec2));
  for (u32 i = 0; i < n; i++) {
    Vec2 p = *(Vec2*)get_array_element((Array*)cleaned_path, i);
    push_array(&body.points, &p);
  }

  // Centered shape reference
  body.shape = centered_shape(cleaned_path);

  // Initialize speeds to zero
  body.speeds = new_array(n, sizeof(Vec2));
  for (u32 i = 0; i < n; i++) {
    Vec2 zero = vec2(0, 0);
    push_array(&body.speeds, &zero);
  }

  return body;
}

static void free_body(PhysicsBody* body) {
  free_array(&body->points);
  free_array(&body->speeds);
  free_array(&body->shape);
}

/* =============================================================================
 * Physics Constraints
 * ========================================================================== */

 /**
  * argmin cost function: distance from point to query point.
  */
static f32 distance_cost(void* element, u32 index, void* ctx) {
  (void)index;
  Vec2* p = (Vec2*)element;
  Vec2* query = (Vec2*)ctx;
  return length_vec2(sub_vec2(*query, *p));
}

static void  enforce_constraints(
  PhysicsBody* body,
  Array* all_bodies,   // Array of PhysicsBody
  u32 body_index,
  f32 dt,
  Array* debug_lines
) {
  if (dt == 0.0f) return;
  Array* path = &body->points;
  i32 n = (i32)path->length;

  // Shape matching
  Vec2 center = vec2(0, 0);
  for (i32 i = 0; i < n; i++) {
    center = add_vec2(center, *(Vec2*)get_array_element(path, (u32)i));
  }
  center = scale_vec2(center, 1.0f / (f32)n);

  // Compute centered path
  Array centered = new_array((u32)n, sizeof(Vec2));
  for (i32 i = 0; i < n; i++) {
    Vec2 p = sub_vec2(*(Vec2*)get_array_element(path, (u32)i), center);
    push_array(&centered, &p);
  }

  // Average rotation angle
  f32 sumSin = 0;
  f32 sumCos = 0;
  for (i32 i = 0; i < n; i++) {
    Vec2 si = *(Vec2*)get_array_element(&body->shape, (u32)i);
    Vec2 ci = *(Vec2*)get_array_element(&centered, (u32)i);
    sumSin += wedge_vec2(si, ci);
    sumCos += dot_vec2(si, ci);
  }
  f32 avg_angle = atan2f(sumSin, sumCos);
  f32 cos_a = cosf(avg_angle);
  f32 sin_a = sinf(avg_angle);
  for (i32 i = 0; i < n; i++) {
    Vec2 si = *(Vec2*)get_array_element(&body->shape, (u32)i);
    Vec2 rotated = vec2(
      cos_a * si.x - sin_a * si.y,
      sin_a * si.x + cos_a * si.y
    );
    Vec2 target = add_vec2(rotated, center);
    Vec2 pi = *(Vec2*)get_array_element(path, (u32)i);
    Vec2 grad = sub_vec2(pi, target);
    pi = add_vec2(pi, scale_vec2(grad, -10.0f * dt));
    set_array_element(path, (u32)i, &pi);
  }
  free_array(&centered);

  // Collision handling with other bodies
  AABB_2D path_box = EMPTY_AABB_2D;
  for (i32 i = 0; i < n; i++) {
    Vec2 pi = *(Vec2*)get_array_element(path, (u32)i);
    AABB_2D pb = build_aabb_from_vec2(pi);
    path_box = union_aabb_2d(&path_box, &pb);
  }

  for (u32 k = 0; k < all_bodies->length; k++) {
    if (k == body_index) continue;
    PhysicsBody* other = (PhysicsBody*)get_array_element(all_bodies, k);
    Array* other_path = &other->points;

    AABB_2D other_box = EMPTY_AABB_2D;
    for (u32 j = 0; j < other_path->length; j++) {
      Vec2 oj = *(Vec2*)get_array_element(other_path, j);
      AABB_2D ob = build_aabb_from_vec2(oj);
      other_box = union_aabb_2d(&other_box, &ob);
    }

    AABB_2D intersection = inter_aabb_2d(&path_box, &other_box);
    if (!intersection.is_empty) {
      for (i32 j = 0; j < n; j++) {
        Vec2 pj = *(Vec2*)get_array_element(path, (u32)j);
        if (is_inside(pj, other_path)) {
          i32 idx = arg_min_array(other_path, distance_cost, &pj);
          Vec2 grad = vec2(0, 0);
          if (idx >= 0) {
            Vec2 closest = *(Vec2*)get_array_element(other_path, (u32)idx);
            grad = sub_vec2(pj, closest);
          }
          Vec2 line_end = add_vec2(pj, scale_vec2(grad, -dt));
          DebugLine dl = { .a = pj, .b = line_end, .color = COLOR_PURPLE };
          push_array(debug_lines, &dl);
          pj = add_vec2(pj, scale_vec2(grad, -10.0f * dt));
          set_array_element(path, (u32)j, &pj);
        }
      }
    }
  }

  // Boundary constraints (keep within [0,1] x [0,1])
  for (i32 i = 0; i < n; i++) {
    Vec2 pi = *(Vec2*)get_array_element(path, (u32)i);
    if (pi.y < 0)    pi.y = 1e-3f;
    if (pi.y > 1.0f) pi.y = 1.0f - 1e-3f;
    if (pi.x < 0)    pi.x = 1e-3f;
    if (pi.x > 1.0f) pi.x = 1.0f - 1e-3f;
    set_array_element(path, (u32)i, &pi);
  }
}

static void update_speed(PhysicsBody* body, const Array* prev_path, f32 dt) {
  if (dt == 0.0f) return;
  u32 n = body->points.length;
  for (u32 i = 0; i < n; i++) {
    Vec2 pi = *(Vec2*)get_array_element(&body->points, i);
    Vec2 prev_pi = *(Vec2*)get_array_element((Array*)prev_path, i);
    Vec2 speed = scale_vec2(sub_vec2(pi, prev_pi), 1.0f / dt);
    set_array_element(&body->speeds, i, &speed);
  }
}

/* =============================================================================
 * Drawing Helpers
 * ========================================================================== */

static Color line_color_shader(u32 x, u32 y, Line_2D* line, void* ctx) {
  (void)x; (void)y; (void)line;
  Color* color = (Color*)ctx;
  return *color;
}

static Color triangle_color_shader(u32 x, u32 y, const Triangle_2D* triangle, void* ctx) {
  (void)x; (void)y; (void)triangle;
  Color* color = (Color*)ctx;
  return *color;
}

/**
 * Draw a closed polygon as lines in world coordinates.
 */
static void draw_path_lines(
  Tela* tela,
  const Camera_2D* camera,
  const Array* path,
  Color color
) {
  u32 n = path->length;
  for (u32 i = 0; i < n; i++) {
    u32 j = (i + 1) % n;
    Vec2 p0 = to_canvas_coord_camera_2d(camera, *(Vec2*)get_array_element((Array*)path, i), tela);
    Vec2 p1 = to_canvas_coord_camera_2d(camera, *(Vec2*)get_array_element((Array*)path, j), tela);
    Line_2D line = { .positions = { p0, p1 }, .props = NULL };
    draw_line_tela(tela, &line, line_color_shader, &color);
  }
}

/**
 * Draw a closed polygon as filled triangles in world coordinates.
 */
static void draw_path_triangles(
  Tela* tela,
  const Camera_2D* camera,
  const Array* path,
  Color color
) {
  // Determine orientation and ensure CCW for triangulation
  f32 orientation = compute_path_orientation(path);
  Array ordered = new_array(path->length, sizeof(Vec2));
  if (orientation > 0) {
    // Reverse the path
    for (i32 i = (i32)path->length - 1; i >= 0; i--) {
      Vec2 p = *(Vec2*)get_array_element((Array*)path, (u32)i);
      push_array(&ordered, &p);
    }
  }
  else {
    for (u32 i = 0; i < path->length; i++) {
      Vec2 p = *(Vec2*)get_array_element((Array*)path, i);
      push_array(&ordered, &p);
    }
  }

  Array triangles = triangulate_polygon(&ordered);
  for (u32 i = 0; i < triangles.length; i++) {
    Tri2D* tri = (Tri2D*)get_array_element(&triangles, i);
    Vec2 p0 = to_canvas_coord_camera_2d(camera, tri->v[0], tela);
    Vec2 p1 = to_canvas_coord_camera_2d(camera, tri->v[1], tela);
    Vec2 p2 = to_canvas_coord_camera_2d(camera, tri->v[2], tela);
    Triangle_2D t2d = { .positions = { p0, p1, p2 }, .props = NULL };
    draw_triangle_tela(tela, &t2d, triangle_color_shader, &color);
  }

  free_array(&ordered);
  free_array(&triangles);
}

/**
 * Draw a line segment in world coordinates with given color.
 */
static void draw_world_line(
  Tela* tela,
  const Camera_2D* camera,
  Vec2 a,
  Vec2 b,
  Color color
) {
  Vec2 ca = to_canvas_coord_camera_2d(camera, a, tela);
  Vec2 cb = to_canvas_coord_camera_2d(camera, b, tela);
  Line_2D line = { .positions = { ca, cb }, .props = NULL };
  draw_line_tela(tela, &line, line_color_shader, &color);
}

/**
 * Draw a filled circle in world coordinates.
 */
static void draw_world_circle(
  Tela* tela,
  const Camera_2D* camera,
  Vec2 pos,
  f32 radius,
  Color color
) {
  Vec2 center_canvas = to_canvas_coord_camera_2d(camera, pos, tela);
  Vec2 edge_canvas = to_canvas_coord_camera_2d(camera, add_vec2(pos, vec2(radius, 0)), tela);
  f32 r_canvas = edge_canvas.x - center_canvas.x;
  draw_circle_tela(tela, center_canvas, r_canvas, color);
}

/* =============================================================================
 * Physics Update
 * ========================================================================== */

static void update_physics(App* app, f32 dt) {
  const Vec2 gravity = vec2(0, -0.2f);
  const i32 sub_steps = 10;
  const f32 delta = dt / (f32)sub_steps;

  clear_array(&app->debug_lines);

  for (u32 i = 0; i < app->bodies.length; i++) {
    PhysicsBody* body = (PhysicsBody*)get_array_element(&app->bodies, i);
    u32 L = body->points.length;

    for (i32 k = 0; k < sub_steps; k++) {
      // Save previous positions
      Array prev_path = new_array(L, sizeof(Vec2));
      for (u32 j = 0; j < L; j++) {
        Vec2 pj = *(Vec2*)get_array_element(&body->points, j);
        push_array(&prev_path, &pj);
      }

      // Integrate forces
      for (u32 j = 0; j < L; j++) {
        Vec2 pj = *(Vec2*)get_array_element(&body->points, j);
        Vec2 sj = *(Vec2*)get_array_element(&body->speeds, j);

        // Mouse attraction force (only when right mouse is down)
        Vec2 mouse_coord = sub_vec2(pj, g_mouse_world);
        Vec2 mouse_force = scale_vec2(mouse_coord, g_right_mouse_down ? -1.0f : 0.0f);

        // Friction (damping)
        Vec2 friction = scale_vec2(sj, -0.5f);

        // Total acceleration
        Vec2 accel = add_vec2(gravity, add_vec2(mouse_force, friction));

        // Semi-implicit Euler
        sj = add_vec2(sj, scale_vec2(accel, delta));
        pj = add_vec2(pj, scale_vec2(sj, delta));

        set_array_element(&body->speeds, j, &sj);
        set_array_element(&body->points, j, &pj);
      }

      // Enforce constraints
      enforce_constraints(body, &app->bodies, i, delta, &app->debug_lines);

      // Update speeds from position changes
      update_speed(body, &prev_path, delta);

      free_array(&prev_path);
    }
  }
}

/* =============================================================================
 * Render
 * ========================================================================== */

static void render(App* app) {
  fill_tela(app->tela, COLOR_BLACK);

  // Draw draft path (currently being drawn)
  if (app->draft_points.length >= 2) {
    for (u32 i = 0; i + 1 < app->draft_points.length; i++) {
      Vec2 a = *(Vec2*)get_array_element(&app->draft_points, i);
      Vec2 b = *(Vec2*)get_array_element(&app->draft_points, i + 1);
      draw_world_line(app->tela, &app->camera, a, b, COLOR_CYAN);
    }
  }

  // Draw all bodies
  Color fill_color = { 0.9f, 0.9f, 0.9f, 1.0f };
  for (u32 i = 0; i < app->bodies.length; i++) {
    PhysicsBody* body = (PhysicsBody*)get_array_element(&app->bodies, i);
    if (app->is_filled) {
      draw_path_triangles(app->tela, &app->camera, &body->points, fill_color);
    }
    else {
      draw_path_lines(app->tela, &app->camera, &body->points, COLOR_WHITE);
    }
  }

  // Draw debug collision lines
  for (u32 i = 0; i < app->debug_lines.length; i++) {
    DebugLine* dl = (DebugLine*)get_array_element(&app->debug_lines, i);
    draw_world_line(app->tela, &app->camera, dl->a, dl->b, dl->color);
  }

  // Draw mouse indicator when right mouse is down
  if (g_right_mouse_down) {
    draw_world_circle(app->tela, &app->camera, g_mouse_world, 0.01f, COLOR_RED);
  }
}

/* =============================================================================
 * Main Loop Callback
 * ========================================================================== */

static void on_frame(f32 dt, f32 time, void* ctx) {
  (void)time;
  App* app = (App*)ctx;

  // Clamp dt to avoid physics explosion on hiccups
  if (dt > 0.05f) dt = 0.05f;

  update_physics(app, dt);
  render(app);

  set_window_title(app->window, format_string("FPS: %.2f", 1.0f / dt));
  paint_window(app->window, app->tela);
}

/* =============================================================================
 * Event Handlers
 * ========================================================================== */

static void on_close(Window* w, void* ctx) {
  stop_loop((Loop*)ctx);
}

static void on_mouse_down(Window* w, i32 x, i32 y, u32 button, void* ctx) {
  App* app = (App*)ctx;
  g_mouse_world = to_world_coord_camera_2d(&app->camera, vec2((f32)x, (f32)y), app->tela);

  if (button == SDL_BUTTON_LEFT) {
    g_mouse_left_down = true;
  }
  if (button == SDL_BUTTON_RIGHT) {
    g_right_mouse_down = true;
  }
}

static void on_mouse_up(Window* w, i32 x, i32 y, u32 button, void* ctx) {
  App* app = (App*)ctx;

  if (button == SDL_BUTTON_LEFT) {
    g_mouse_left_down = false;
    // Finalize drawn path into a physics body
    if (app->draft_points.length > 2) {
      Array cleaned = clean_path(&app->draft_points);
      if (cleaned.length >= 3) {
        PhysicsBody body = create_body(&cleaned);
        push_array(&app->bodies, &body);
      }
      free_array(&cleaned);
    }
    clear_array(&app->draft_points);
  }
  if (button == SDL_BUTTON_RIGHT) {
    g_right_mouse_down = false;
  }
}

static void on_mouse_move(Window* w, i32 x, i32 y, void* ctx) {
  (void)w;
  App* app = (App*)ctx;
  Vec2 new_mouse = to_world_coord_camera_2d(&app->camera, vec2((f32)x, (f32)y), app->tela);

  if (g_mouse_left_down && !equals_vec2(new_mouse, g_mouse_world)) {
    // Add both endpoints of the segment to build the path
    push_array(&app->draft_points, &g_mouse_world);
    push_array(&app->draft_points, &new_mouse);
  }

  g_mouse_world = new_mouse;
}

static void on_key_down(Window* w, u32 keycode, void* ctx) {
  App* app = (App*)ctx;

  if (keycode == SDLK_r) {
    // Reset: free all bodies
    for (u32 i = 0; i < app->bodies.length; i++) {
      PhysicsBody* body = (PhysicsBody*)get_array_element(&app->bodies, i);
      free_body(body);
    }
    clear_array(&app->bodies);
    clear_array(&app->draft_points);
  }
  if (keycode == SDLK_t) {
    app->is_filled = !app->is_filled;
  }
}

/* =============================================================================
 * Main
 * ========================================================================== */

int main(void) {
  Tela* tela = new_tela(WIDTH, HEIGHT);
  Window* window = new_window(WIDTH, HEIGHT, "2D Physics");

  App app = {
    .tela = tela,
    .window = window,
    .camera = build_camera_2d(vec2(0, 0), vec2(1, 1)),
    .bodies = new_array(16, sizeof(PhysicsBody)),
    .draft_points = new_array(256, sizeof(Vec2)),
    .debug_lines = new_array(64, sizeof(DebugLine)),
    .is_filled = false,
  };

  Loop* animation = loop(on_frame, &app);
  on_close_window(window, on_close, animation);
  on_mouse_down_window(window, on_mouse_down, &app);
  on_mouse_up_window(window, on_mouse_up, &app);
  on_mouse_move_window(window, on_mouse_move, &app);
  on_key_down_window(window, on_key_down, &app);

  play_loop(animation);
  free_window(window);
  return 0;
}
