#include "../src/index.c"

static const u32 WIDTH = 640/2;
static const u32 HEIGHT = 480/2;
static const f32 TORUS_MAJOR = 0.75f;
static const f32 TORUS_MINOR = 0.25f;
static const f32 MESH_SPHERE_RADIUS = 0.05f;
static const u32 MAX_RAYMARCH_ITERATIONS = 100;
static const f32 MAX_RAYMARCH_DISTANCE = 10.0f;
static const f32 RAYMARCH_EPSILON = 1e-2f;
static const f32 NORMAL_EPSILON = 1e-3f;

typedef struct {
	Tela* tela;
	Window* window;
	Scene scene;
	Camera camera;
	f32 time;
} App;

typedef struct {
	Scene* scene;
	f32 time;
} RenderCtx;

typedef struct {
	Vec3 center;
	f32 scale_inv;
} NormalizeCtx;

static bool g_mouse_down = false;
static Vec2 g_mouse_pos = { 0 };

static Vec3 normalize_mesh_vertex(Vec3 v, void* ctx) {
	NormalizeCtx* c = (NormalizeCtx*)ctx;
	return scale_vec3(sub_vec3(v, c->center), c->scale_inv);
}

static Vec3 rotate_xy(Vec3 v, void* ctx) {
	(void)ctx;
	return vec3(-v.y, v.x, v.z);
}

static Vec3 rotate_xz(Vec3 v, void* ctx) {
	(void)ctx;
	return vec3(v.z, v.y, -v.x);
}

static f32 torus_sdf(Vec3 p) {
	f32 q = length_vec2(vec2(p.x, p.y)) - TORUS_MAJOR;
	return length_vec2(vec2(q, p.z)) - TORUS_MINOR;
}

static f32 smooth_min_32(f32 a, f32 b) {
	const f32 k = 32.0f;
	const f32 m = fminf(a, b);
	const f32 ea = expf(-k * (a - m));
	const f32 eb = expf(-k * (b - m));
	return m - logf(ea + eb) / k;
	// return fminf(a, b);
}

static Vec3 torus_normal(Vec3 p) {
	const f32 epsilon = NORMAL_EPSILON;
	const f32 f = torus_sdf(p);
	Vec3 gradient = vec3(
		torus_sdf(add_vec3(p, vec3(epsilon, 0.0f, 0.0f))) - f,
		torus_sdf(add_vec3(p, vec3(0.0f, epsilon, 0.0f))) - f,
		torus_sdf(add_vec3(p, vec3(0.0f, 0.0f, epsilon))) - f
	);
	return normalize_vec3(gradient);
}

static Color ray_scene(Ray ray, void* ctx) {
	RenderCtx* render_ctx = (RenderCtx*)ctx;
	if (!render_ctx || !render_ctx->scene) {
		return COLOR_BLACK;
	}

	Vec3 p = ray.init;
	f32 t = 0.0f;
	for (u32 i = 0; i < MAX_RAYMARCH_ITERATIONS; i++) {
		p = trace_ray(ray, t);
		f32 tau = (sinf(2.0f * PI * 0.25f * (render_ctx->time - 1.0f)) + 1.0f) * 0.5f;
		f32 torus_dist = torus_sdf(p);
		f32 scene_dist = distance_on_ray_scene(
			render_ctx->scene,
			build_ray(p, ray.dir),
			smooth_min_32
		)-0.01f;
		f32 d = tau * torus_dist + (1.0f - tau) * scene_dist;
		t += d;

		if (d < RAYMARCH_EPSILON) {
			Vec3 torus_n = torus_normal(p);
			Vec3 scene_n = normal_to_point_scene(render_ctx->scene, p, smooth_min_32);
			Vec3 normal = normalize_vec3(
				add_vec3(scale_vec3(torus_n, tau), scale_vec3(scene_n, 1.0f - tau))
			);
			return (Color) {
				.red = (normal.x + 1.0f) * 0.5f,
				.green = (normal.y + 1.0f) * 0.5f,
				.blue = (normal.z + 1.0f) * 0.5f,
				.alpha = 1.0f,
			};
		}

		if (t > MAX_RAYMARCH_DISTANCE) {
			return (Color) {
				.red = 0.0f,
				.green = 0.0f,
				.blue = 10.0f * ((f32)i / (f32)MAX_RAYMARCH_ITERATIONS),
				.alpha = 1.0f,
			};
		}
	}

	return COLOR_BLACK;
}

static void on_frame(f32 dt, f32 time, void* ctx) {
	App* app = (App*)ctx;
	RenderCtx render_ctx = {
		.scene = &app->scene,
		.time = app->time + 0.01f,
	};

	app->time += 0.01f;

	ray_map_camera_parallel(&app->camera, app->tela, ray_scene, &render_ctx);

	set_window_title(
		app->window,
		format_string("Bunny Ray march | FPS: %.2f", 1.0f / dt)
	);
	paint_window(app->window, app->tela);
}

static void on_close(Window* window, void* ctx) {
	Loop* animation = (Loop*)ctx;
	stop_loop(animation);
}

static void on_mouse_down(Window* window, i32 x, i32 y, u32 button, void* ctx) {
	g_mouse_down = true;
	g_mouse_pos = vec2((f32)x, (f32)y);
}

static void on_mouse_up(Window* window, i32 x, i32 y, u32 button, void* ctx) {
	g_mouse_down = false;
	g_mouse_pos = vec2(0.0f, 0.0f);
}

static void on_mouse_move(Window* window, i32 x, i32 y, void* ctx) {
	App* app = (App*)ctx;
	Vec2 new_mouse = vec2((f32)x, (f32)y);
	if (!g_mouse_down || equals_vec2(new_mouse, g_mouse_pos)) {
		return;
	}

	Vec2 delta = sub_vec2(new_mouse, g_mouse_pos);
	Vec3 orbit = get_camera_orbit(&app->camera);
	set_orbit_camera(
		&app->camera,
		orbit.x,
		orbit.y - 2.0f * PI * (delta.x / (f32)app->tela->width),
		orbit.z - 2.0f * PI * (delta.y / (f32)app->tela->height)
	);
	g_mouse_pos = new_mouse;
}

static void on_mouse_scroll(Window* window, i32 delta_y, void* ctx) {
	App* app = (App*)ctx;
	Vec3 orbit = get_camera_orbit(&app->camera);
	set_orbit_camera(&app->camera, orbit.x + (f32)delta_y * 0.1f, orbit.y, orbit.z);
}

static void build_bunny_scene(Scene* scene) {
	String obj = io_read_file("./assets/bunny_orig.obj");
	Mesh mesh = read_obj_mesh(obj, "mesh");

	AABB box = get_bounding_box_mesh(&mesh);
	f32 scale_inv = 2.0f / fmaxf(max_comp_vec3(box.diagonal), 1e-6f);
	NormalizeCtx normalize_ctx = {
		.center = box.center,
		.scale_inv = scale_inv,
	};

	map_vertices_mesh(&mesh, normalize_mesh_vertex, &normalize_ctx);
	map_vertices_mesh(&mesh, rotate_xy, NULL);
	map_vertices_mesh(&mesh, rotate_xz, NULL);

	Array spheres = get_spheres_mesh(&mesh, MESH_SPHERE_RADIUS);
	Array elems = spheres_to_scene_elems(spheres);
	add_scene_elems_scene(scene, elems);
	free_array(&spheres);
	free_array(&elems);
	scene->vtable->rebuild_scene(scene);
}

int main(void) {
	Tela* tela = new_tela(WIDTH, HEIGHT);
	Window* window = new_window(WIDTH * 2, HEIGHT * 2, "Bunny Ray march");
	Scene scene = new_kscene(10);
	Camera camera = create_camera(vec3(5.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 1.0f);

	build_bunny_scene(&scene);

	App app = {
		.tela = tela,
		.window = window,
		.scene = scene,
		.camera = camera,
		.time = 0.0f,
	};

	Loop* animation = loop(on_frame, &app);
	on_close_window(window, on_close, animation);
	on_mouse_down_window(window, on_mouse_down, &app);
	on_mouse_up_window(window, on_mouse_up, &app);
	on_mouse_move_window(window, on_mouse_move, &app);
	on_mouse_scroll_window(window, on_mouse_scroll, &app);

	play_loop(animation);
	return 0;
}
