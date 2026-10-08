/**
 * Mario soft-body physics demo. Made with AI using tela.c engine for usage reference.
 *
 * Left drag grabs the clicked triangle and stretches the mesh, which springs
 * back on release.
 * Right drag orbits the camera and the mouse wheel zooms.
 *
 * Build: gcc -O3 -o app test/mario_physics.c -lSDL2 -lm && ./app
 * 
 * gcc -O3 -fopenmp -ffast-math -o app test/mario_physics.c -lSDL2 -lm -march=native && ./app
 */

#include "../src/index.c"

static const u32 WIDTH = 640;
static const u32 HEIGHT = 480;
#define NO_EDGE UINT32_MAX
static const f32 SPRING_STIFFNESS = 150.0f;
static const f32 FRICTION = 3.0f;
static const f32 REST_STIFFNESS = 25.0f;
static const f32 GRAB_STIFFNESS = 400.0f;

typedef struct {
  Vec3 position;
  Vec3 rest_position;
  Vec3 velocity;
  Vec3 force;
  u32 first_edge;
} GraphNode;

typedef struct {
  u32 a;
  u32 b;
  u32 next_a;
  u32 next_b;
  f32 rest_length;
} GraphEdge;

typedef struct {
  GraphNode* nodes;
  u32 node_count;
  Array edges; /* GraphEdge */
} MeshGraph;

typedef struct {
  Tela* tela;
  Window* window;
  Camera camera;
  Scene scene;
  Mesh mesh;
  MeshGraph graph;
  bool left_mouse_down;
  bool right_mouse_down;
  bool has_mouse_target;
  Vec2 mouse;
  Vec3 mouse_target;
  Vec3 mouse_plane_normal;
  u32 grab_nodes[3];
  Vec3 grab_offsets[3];
} App;

typedef struct {
  Vec3 center;
  f32 scale_inv;
} FirstTransformContext;

static Vec3 first_transform(Vec3 v, void* ctx) {
  FirstTransformContext* c = (FirstTransformContext*)ctx;
  return scale_vec3(sub_vec3(v, c->center), c->scale_inv);
}

static Vec3 second_transform(Vec3 v, void* ctx) {
  (void)ctx;
  return vec3(-v.y, v.x, v.z);
}

static Vec3 third_transform(Vec3 v, void* ctx) {
  (void)ctx;
  return vec3(v.z, v.y, -v.x);
}

static u32 graph_other_endpoint(const GraphEdge* edge, u32 node) {
  return edge->a == node ? edge->b : edge->a;
}

/* Edges of a node form a linked list; NULL marks the end. */
static GraphEdge* graph_edge_at(MeshGraph* graph, u32 edge_index) {
  if (edge_index == NO_EDGE)
    return NULL;
  return (GraphEdge*)get_array_element(&graph->edges, edge_index);
}

static GraphEdge* graph_first_edge(MeshGraph* graph, u32 node) {
  return graph_edge_at(graph, graph->nodes[node].first_edge);
}

static GraphEdge* graph_next_edge(MeshGraph* graph, GraphEdge* edge, u32 node) {
  return graph_edge_at(graph, edge->a == node ? edge->next_a : edge->next_b);
}

static Vec3 graph_tension_spring_force(
    const GraphNode* node, const GraphNode* neighbor, f32 rest_length
) {
  Vec3 displacement = sub_vec3(neighbor->position, node->position);
  f32 length = length_vec3(displacement);
  if (length > rest_length && length > 1e-6f) {
    return scale_vec3(
        displacement, SPRING_STIFFNESS * (length - rest_length) / length
    );
  }
  return vec3(0.0f, 0.0f, 0.0f);
}

static bool graph_add_edge(MeshGraph* graph, u32 a, u32 b) {
  if (a >= graph->node_count || b >= graph->node_count)
    return false;
  if (a == b)
    return true;

  for (GraphEdge* edge = graph_first_edge(graph, a); edge;
       edge = graph_next_edge(graph, edge, a)) {
    if (graph_other_endpoint(edge, a) == b)
      return true;
  }

  Vec3 displacement =
      sub_vec3(graph->nodes[b].position, graph->nodes[a].position);
  GraphEdge edge = {
    .a = a,
    .b = b,
    .next_a = graph->nodes[a].first_edge,
    .next_b = graph->nodes[b].first_edge,
    .rest_length = length_vec3(displacement),
  };
  if (graph->edges.length >= graph->edges.capacity)
    return false;
  u32 edge_index = graph->edges.length;
  push_array(&graph->edges, &edge);
  if (graph->edges.length != edge_index + 1)
    return false;
  graph->nodes[a].first_edge = edge_index;
  graph->nodes[b].first_edge = edge_index;
  return true;
}

static bool build_mesh_graph(MeshGraph* graph, Mesh* mesh) {
  if (mesh->vertices.length == 0 || mesh->faces.length == 0) {
    fprintf(stderr, "Mario OBJ has no vertices or faces.\n");
    return false;
  }

  graph->node_count = mesh->vertices.length;
  graph->nodes = (GraphNode*)calloc(graph->node_count, sizeof(GraphNode));
  graph->edges = new_array(mesh->faces.length * 3, sizeof(GraphEdge));
  if (!graph->nodes || !graph->edges.data) {
    fprintf(stderr, "Unable to allocate Mario's mesh graph.\n");
    free(graph->nodes);
    graph->nodes = NULL;
    free_array(&graph->edges);
    return false;
  }

  AABB bounds = get_bounding_box_mesh(mesh);
  FirstTransformContext first = {
    .center = bounds.center,
    .scale_inv = 2.0f / max_comp_vec3(bounds.diagonal),
  };
  map_vertices_mesh(mesh, first_transform, &first);
  map_vertices_mesh(mesh, second_transform, NULL);
  map_vertices_mesh(mesh, third_transform, NULL);
  for (u32 i = 0; i < graph->node_count; i++) {
    Vec3 position = *(Vec3*)get_array_element(&mesh->vertices, i);
    graph->nodes[i] = (GraphNode){
      .position = position,
      .rest_position = position,
      .first_edge = NO_EDGE,
    };
  }

  for (u32 face_index = 0; face_index < mesh->faces.length; face_index++) {
    Face* face = (Face*)get_array_element(&mesh->faces, face_index);
    for (u32 side = 0; side < 3; side++) {
      if (!graph_add_edge(graph, face->vertex_indices[side],
                          face->vertex_indices[(side + 1) % 3])) {
        fprintf(stderr, "Unable to build Mario's mesh graph edges.\n");
        free(graph->nodes);
        graph->nodes = NULL;
        free_array(&graph->edges);
        return false;
      }
    }
  }

  return true;
}

static bool ray_triangle_hit(Ray ray, Triangle triangle, f32* hit_distance) {
  const f32 epsilon = 1e-7f;
  Vec3 edge1 = sub_vec3(triangle.positions[1], triangle.positions[0]);
  Vec3 edge2 = sub_vec3(triangle.positions[2], triangle.positions[0]);
  Vec3 p = cross_vec3(ray.dir, edge2);
  f32 determinant = dot_vec3(edge1, p);
  if (fabsf(determinant) < epsilon)
    return false;

  f32 inverse_determinant = 1.0f / determinant;
  Vec3 from_vertex = sub_vec3(ray.init, triangle.positions[0]);
  f32 u = dot_vec3(from_vertex, p) * inverse_determinant;
  if (u < 0.0f || u > 1.0f)
    return false;

  Vec3 q = cross_vec3(from_vertex, edge1);
  f32 v = dot_vec3(ray.dir, q) * inverse_determinant;
  if (v < 0.0f || u + v > 1.0f)
    return false;

  f32 distance = dot_vec3(edge2, q) * inverse_determinant;
  if (distance <= epsilon)
    return false;
  *hit_distance = distance;
  return true;
}

static bool pick_mesh(App* app, Ray ray, Vec3* hit_point, u32* hit_face) {
  f32 closest = INFINITY;
  bool found = false;
  for (u32 i = 0; i < app->mesh.faces.length; i++) {
    Face* face = (Face*)get_array_element(&app->mesh.faces, i);
    Triangle triangle = { 0 };
    for (u32 j = 0; j < 3; j++) {
      triangle.positions[j] =
          app->graph.nodes[face->vertex_indices[j]].position;
    }
    f32 distance;
    if (ray_triangle_hit(ray, triangle, &distance) && distance < closest) {
      closest = distance;
      *hit_face = i;
      found = true;
    }
  }
  if (found)
    *hit_point = trace_ray(ray, closest);
  return found;
}

static bool mouse_point_on_plane(App* app, i32 x, i32 y, Vec3* point) {
  if (x < 0 || y < 0 || (u32)x >= app->tela->width ||
      (u32)y >= app->tela->height) {
    return false;
  }
  Ray ray = ray_from_tela_camera(&app->camera, app->tela, (u32)x, (u32)y);
  f32 denominator = dot_vec3(app->mouse_plane_normal, ray.dir);
  if (fabsf(denominator) < 1e-6f)
    return false;
  f32 distance = dot_vec3(
      sub_vec3(app->mouse_target, ray.init), app->mouse_plane_normal
  ) / denominator;
  if (distance < 0.0f)
    return false;
  *point = trace_ray(ray, distance);
  return true;
}

static Vec3 node_spring_force(App* app, u32 node_index) {
  GraphNode* node = &app->graph.nodes[node_index];
  Vec3 force = vec3(0.0f, 0.0f, 0.0f);
  for (GraphEdge* edge = graph_first_edge(&app->graph, node_index); edge;
       edge = graph_next_edge(&app->graph, edge, node_index)) {
    GraphNode* neighbor =
        &app->graph.nodes[graph_other_endpoint(edge, node_index)];
    force = add_vec3(
        force, graph_tension_spring_force(node, neighbor, edge->rest_length)
    );
  }
  return force;
}

static Vec3 node_rest_force(GraphNode* node) {
  Vec3 offset = sub_vec3(node->rest_position, node->position);
  return scale_vec3(offset, REST_STIFFNESS);
}

static Vec3 node_mouse_force(App* app, u32 node_index) {
  if (!app->has_mouse_target)
    return vec3(0.0f, 0.0f, 0.0f);
  for (u32 k = 0; k < 3; k++) {
    if (app->grab_nodes[k] == node_index) {
      Vec3 goal = add_vec3(app->mouse_target, app->grab_offsets[k]);
      Vec3 offset = sub_vec3(goal, app->graph.nodes[node_index].position);
      return scale_vec3(offset, GRAB_STIFFNESS);
    }
  }
  return vec3(0.0f, 0.0f, 0.0f);
}

static void integrate_node(App* app, u32 node_index, f32 dt) {
  GraphNode* node = &app->graph.nodes[node_index];
  Vec3 force = add_vec3(
      node_spring_force(app, node_index), node_rest_force(node)
  );
  force = add_vec3(force, node_mouse_force(app, node_index));
  force = sub_vec3(force, scale_vec3(node->velocity, FRICTION));

  node->force = force;
  node->velocity = add_vec3(node->velocity, scale_vec3(force, dt));
  node->position = add_vec3(node->position, scale_vec3(node->velocity, dt));
}

static void sync_scene_with_graph(App* app) {
  Array* elements = get_scene_elems_scene(&app->scene);
  for (u32 i = 0; i < app->mesh.faces.length; i++) {
    Face* face = (Face*)get_array_element(&app->mesh.faces, i);
    SceneElem* element = (SceneElem*)get_array_element(elements, i);
    if (!element)
      continue;
    for (u32 j = 0; j < 3; j++) {
      element->as.triangle.positions[j] =
          app->graph.nodes[face->vertex_indices[j]].position;
    }
  }
}

static void update_graph_physics(App* app, f32 frame_dt) {
  const f32 max_step = 1.0f / 240.0f;
  i32 steps = (i32)ceilf(frame_dt / max_step);
  if (steps < 1)
    steps = 1;
  f32 dt = frame_dt / (f32)steps;

  for (i32 step = 0; step < steps; step++) {
    for (u32 i = 0; i < app->graph.node_count; i++) {
      integrate_node(app, i, dt);
    }
  }
  sync_scene_with_graph(app);
}

static void on_frame(f32 dt, f32 time, void* context) {
  (void)time;
  App* app = (App*)context;

  raster_scene(
      &app->scene,
      (RasterParams){
          .camera = &app->camera,
          .tela = app->tela,
          .cull_backfaces = false,
          .bilinear_texture = false,
          .clip_camera_plane = true,
          .clear_screen = true,
          .background_color = (Color){ 0.08f, 0.08f, 0.1f, 1.0f },
          .perspective_correct = true,
      }
  );

  update_graph_physics(app, dt);

  u32 fps = dt > 0.0f ? (u32)(1.0f / dt) : 0;
  set_window_title(
      app->window,
      format_string(
          "Mario Graph Physics | Left drag: deform | Right drag: orbit | Wheel: zoom | FPS: %u",
          fps
      )
  );
  paint_window(app->window, app->tela);
}

static void on_close(Window* window, void* context) {
  (void)window;
  stop_loop((Loop*)context);
}

static void on_mouse_down(Window* window, i32 x, i32 y, u32 button,
                          void* context) {
  (void)window;
  App* app = (App*)context;
  app->mouse = vec2((f32)x, (f32)y);
  if (button == SDL_BUTTON_LEFT) {
    app->left_mouse_down = true;
    Ray ray = ray_from_tela_camera(
        &app->camera, app->tela, (u32)x, (u32)y
    );
    Vec3 point;
    u32 face_index;
    if (pick_mesh(app, ray, &point, &face_index)) {
      Face* face = (Face*)get_array_element(&app->mesh.faces, face_index);
      for (u32 k = 0; k < 3; k++) {
        app->grab_nodes[k] = face->vertex_indices[k];
        app->grab_offsets[k] =
            sub_vec3(app->graph.nodes[face->vertex_indices[k]].position, point);
      }
      app->mouse_target = point;
      app->mouse_plane_normal = ray.dir;
      app->has_mouse_target = true;
    }
  } else if (button == SDL_BUTTON_RIGHT) {
    app->right_mouse_down = true;
  }
}

static void on_mouse_up(Window* window, i32 x, i32 y, u32 button,
                        void* context) {
  (void)window;
  (void)x;
  (void)y;
  App* app = (App*)context;
  if (button == SDL_BUTTON_LEFT) {
    app->left_mouse_down = false;
    app->has_mouse_target = false;
  } else if (button == SDL_BUTTON_RIGHT) {
    app->right_mouse_down = false;
  }
}

static void on_mouse_move(Window* window, i32 x, i32 y, void* context) {
  (void)window;
  App* app = (App*)context;
  Vec2 next_mouse = vec2((f32)x, (f32)y);
  Vec2 delta = sub_vec2(next_mouse, app->mouse);

  if (app->left_mouse_down && app->has_mouse_target) {
    Vec3 point;
    if (mouse_point_on_plane(app, x, y, &point))
      app->mouse_target = point;
  }
  if (app->right_mouse_down) {
    Vec3 orbit = get_camera_orbit(&app->camera);
    f32 theta = orbit.y - 2.0f * PI * delta.x / (f32)WIDTH;
    f32 phi = clamp(
        orbit.z - 2.0f * PI * delta.y / (f32)HEIGHT, -1.45f, 1.45f
    );
    set_orbit_camera(&app->camera, orbit.x, theta, phi);
  }
  app->mouse = next_mouse;
}

static void on_mouse_scroll(Window* window, i32 delta_y, void* context) {
  (void)window;
  App* app = (App*)context;
  Vec3 orbit = get_camera_orbit(&app->camera);
  set_orbit_camera(
      &app->camera,
      clamp(orbit.x + (f32)delta_y * 0.12f, 1.4f, 8.0f),
      orbit.y,
      orbit.z
  );
}

static void register_input_handlers(Window* window, App* app) {
  on_mouse_down_window(window, on_mouse_down, app);
  on_mouse_up_window(window, on_mouse_up, app);
  on_mouse_move_window(window, on_mouse_move, app);
  on_mouse_scroll_window(window, on_mouse_scroll, app);
}

int main(void) {
  String obj = io_read_file("./assets/mario.obj");
  if (!obj.data || obj.length == 0) {
    fprintf(stderr, "Unable to read ./assets/mario.obj.\n");
    return 1;
  }

  Mesh mesh = read_obj_mesh(obj, "mario");
  Tela* texture = io_read_image("./assets/mario.png");
  if (!texture) {
    fprintf(stderr, "Unable to read ./assets/mario.png.\n");
    return 1;
  }
  add_texture_mesh(&mesh, texture);

  MeshGraph graph = { 0 };
  if (!build_mesh_graph(&graph, &mesh))
    return 1;

  Array triangles = get_triangles_mesh(&mesh);
  Array elements = triangles_to_scene_elems(triangles);
  Scene scene = new_naive_scene();
  add_scene_elems_scene(&scene, elements);
  free_array(&elements);
  free_array(&triangles);

  Tela* tela = new_tela(WIDTH, HEIGHT);
  Window* window = new_window(WIDTH, HEIGHT, "Mario Graph Physics");
  if (!tela || !window) {
    fprintf(stderr, "Unable to create the Mario physics window.\n");
    return 1;
  }

  App app = {
    .tela = tela,
    .window = window,
    .camera = create_camera(vec3(3.4f, 0.0f, 0.0f), vec3(0, 0, 0), 1.0f),
    .scene = scene,
    .mesh = mesh,
    .graph = graph,
  };
  set_orbit_camera(&app.camera, 3.4f, 0.0f, 0.12f);

  Loop* animation = loop(on_frame, &app);
  on_close_window(window, on_close, animation);
  register_input_handlers(window, &app);
  play_loop(animation);

  free_scene(&app.scene);
  free(app.graph.nodes);
  free_array(&app.graph.edges);
  return 0;
}
