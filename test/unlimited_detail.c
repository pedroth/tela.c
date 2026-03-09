#include "../src/index.c"

static const u32 WIDTH = 640;
static const u32 HEIGHT = 480;

typedef struct {
	Tela* tela;
	Window* window;
	Scene scene;
	Camera camera;
	bool mouse_down;
	Vec2 mouse_pos;
	bool show_debug;
	bool use_parallel;
} App;

static Vec3 map_v1(Vec3 v, void* ctx) {
	return vec3(-v.y, v.x, v.z);
}

static Vec3 map_v2(Vec3 v, void* ctx) {
	return vec3(v.z, v.y, -v.x);
}

static Vec3 map_v3(Vec3 v, void* ctx) {
	return vec3(-v.x, v.y, v.z);
}

static Color map_color_from_vertex(Vec3 v, void* ctx) {
	(void)ctx;
	return (Color) {
		.red = clamp(0.5f * (v.x + 1.0f), 0.0f, 1.0f),
		.green = clamp(0.5f * (v.y + 1.0f), 0.0f, 1.0f),
		.blue = clamp(0.5f * (v.z + 1.0f), 0.0f, 1.0f),
		.alpha = 1.0f,
	};
}

static Mesh translated_mesh_copy(const Mesh* mesh, Vec3 offset) {
	Mesh copy = *mesh;
	copy.vertices = new_array(mesh->vertices.length > 0 ? mesh->vertices.length : 4, sizeof(Vec3));
	for (u32 i = 0; i < mesh->vertices.length; i++) {
		Vec3 v = *(Vec3*)get_array_element((Array*)&mesh->vertices, i);
		v = add_vec3(v, offset);
		push_array(&copy.vertices, &v);
	}
	return copy;
}

static AABB compute_scene_bbox(Scene* scene) {
	Array* elems = get_scene_elems_scene(scene);
	AABB box = EMPTY_AABB;
	for (u32 i = 0; i < elems->length; i++) {
		SceneElem* elem = (SceneElem*)get_array_element(elems, i);
		AABB elem_box = get_bounding_box_scene_elem(elem);
		box = union_aabb(&box, &elem_box);
	}
	return box;
}

static Color render_ray(Ray ray, void* ctx) {
	App* app = (App*)ctx;
	SceneHit hit = intersect_scene(&app->scene, ray);
	if (hit.hit && hit.scene_elem) {
		Vec3 normal = normal_to_point_scene_elem(hit.scene_elem, hit.position);
		return (Color) {
			.red = (normal.x + 1.0f) * 0.5f,
			.green = (normal.y + 1.0f) * 0.5f,
			.blue = (normal.z + 1.0f) * 0.5f,
			.alpha = 1.0f,
		};
	}
	return COLOR_BLACK;
}

static void on_frame(f32 dt, f32 time, void* ctx) {
	(void)time;
	App* app = (App*)ctx;
	if (app->use_parallel) {
		ray_map_camera_parallel(&app->camera, app->tela, render_ray, app);
	} else {
		ray_map_camera(&app->camera, app->tela, render_ray, app);
	}
	if (app->show_debug) {
		debug_scene(&app->scene, &(SceneDebugProps) { .camera = &app->camera, .tela = app->tela });
	}
	set_window_title(app->window, format_string("UD, FPS: %.2f | Render: %s (P) | Debug: %s (D)", 1.0f / dt,
		app->use_parallel ? "Parallel" : "Serial", app->show_debug ? "ON" : "OFF"));
	paint_window(app->window, app->tela);
}

static void on_close(Window* window, void* ctx) {
	Loop* animation = (Loop*)ctx;
	stop_loop(animation);
}

static void on_mouse_down(Window* w, i32 x, i32 y, u32 button, void* ctx) {
	App* app = (App*)ctx;
	app->mouse_down = true;
	app->mouse_pos = vec2((f32)x, (f32)y);
}

static void on_mouse_up(Window* w, i32 x, i32 y, u32 button, void* ctx) {
	App* app = (App*)ctx;
	app->mouse_down = false;
	app->mouse_pos = vec2(0, 0);
}

static void on_mouse_move(Window* w, i32 x, i32 y, void* ctx) {
	App* app = (App*)ctx;

	Vec2 new_mouse = vec2((f32)x, (f32)y);
	if (!app->mouse_down || equals_vec2(new_mouse, app->mouse_pos)) {
		return;
	}

	Vec2 delta = sub_vec2(new_mouse, app->mouse_pos);
	Vec3 orbit = get_camera_orbit(&app->camera);
	set_orbit_camera(
		&app->camera,
		orbit.x,
		orbit.y - 2.0f * PI * (delta.x / (f32)WIDTH),
		orbit.z - 2.0f * PI * (delta.y / (f32)HEIGHT)
	);

	app->mouse_pos = new_mouse;
}

static void on_mouse_scroll(Window* w, i32 delta_y, void* ctx) {
	App* app = (App*)ctx;
	Vec3 orbit = get_camera_orbit(&app->camera);
	f32 new_radius = orbit.x + (f32)delta_y * 1.0f;
	set_orbit_camera(&app->camera, new_radius, orbit.y, orbit.z);
}

static void on_key_down(Window* w, u32 keycode, void* ctx) {
	(void)w;
	App* app = (App*)ctx;
	if (keycode == SDLK_d) {
		app->show_debug = !app->show_debug;
	}
	if (keycode == SDLK_p) {
		app->use_parallel = !app->use_parallel;
	}
}

int main(void) {
	Tela* tela = new_tela(WIDTH, HEIGHT);
	Window* window = new_window(WIDTH, HEIGHT, "Unlimited Detail");

	Scene scene = new_kscene(10);
	Camera camera = create_camera(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 1.0f);

	App app = {
		.tela = tela,
		.window = window,
		.scene = scene,
		.camera = camera,
		.mouse_down = false,
		.mouse_pos = vec2(0, 0),
		.show_debug = false,
		.use_parallel = true,
	};

	String obj = io_read_file("assets/spot.obj");
	Mesh mesh = read_obj_mesh(obj, "spot");
	map_vertices_mesh(&mesh, map_v1, NULL);
	map_vertices_mesh(&mesh, map_v2, NULL);
	map_vertices_mesh(&mesh, map_v3, NULL);
	map_colors_mesh(&mesh, map_color_from_vertex, NULL);

	const i32 n = 10;
	for (i32 i = 0; i < n; i++) {
		for (i32 j = 0; j < n; j++) {
			Mesh instance = translated_mesh_copy(&mesh, vec3(-2.0f * i, 2.0f * j, 0.0f));
			Array triangles = get_triangles_mesh(&instance);
			Array elems = triangles_to_scene_elems(triangles);
			add_scene_elems_scene(&app.scene, elems);
			free_array(&triangles);
			free_array(&elems);
			free_array(&instance.vertices);
		}
	}
	app.scene.vtable->rebuild_scene(&app.scene);

	AABB scene_box = compute_scene_bbox(&app.scene);
	if (!scene_box.is_empty) {
		Vec3 orbit = get_camera_orbit(&app.camera);
		f32 radius = fmaxf(length_vec3(scene_box.diagonal), 1.0f);
		app.camera.look_at = scene_box.center;
		set_orbit_camera(&app.camera, radius, orbit.y, orbit.z);
	}


	Loop* animation_loop = loop(on_frame, &app);
	on_close_window(window, on_close, animation_loop);

	on_mouse_down_window(window, on_mouse_down, &app);
	on_mouse_up_window(window, on_mouse_up, &app);
	on_mouse_move_window(window, on_mouse_move, &app);
	on_mouse_scroll_window(window, on_mouse_scroll, &app);
	on_key_down_window(window, on_key_down, &app);

	play_loop(animation_loop);
	return 0;
}
