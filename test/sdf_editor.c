#include "../src/index.c"

static const u32 WIDTH = 640;
static const u32 HEIGHT = 480;
static const f32 BALL_RADIUS = 0.25f;
static const f32 MIN_PLACEMENT_CLEARANCE = 0.0001f;

typedef struct {
	Tela* tela;
	Window* window;
	Scene scene;
	Camera camera;
	bool left_mouse_down;
	bool right_mouse_down;
	Vec2 mouse;
	u32 ball_id;
	bool show_debug;
	bool use_parallel;
} App;

typedef struct {
	Scene* scene;
} RenderCtx;

static void add_ball_from_screen(App* app, i32 x, i32 y) {
	const i32 clamped_x = max_i32(0, min_i32((i32)app->tela->width - 1, x));
	const i32 clamped_y = max_i32(0, min_i32((i32)app->tela->height - 1, y));
	Ray ray = ray_from_tela_camera(&app->camera, app->tela, (u32)clamped_x, (u32)clamped_y);
	// SceneHit hit = intersect_scene(&app->scene, ray);
	// if (hit.hit) {
	// 	return;
	// }

	Ray center_ray = ray_from_tela_camera(&app->camera, app->tela, app->tela->width / 2, app->tela->height / 2);

	const Vec3 normal = center_ray.dir;
	const f32 denom = dot_vec3(normal, ray.dir);
	if (fabsf(denom) > 1e-8f) {
		const f32 t = -dot_vec3(normal, ray.init) / denom;
		const Vec3 p = trace_ray(ray, t);
		// const f32 occupied_distance = distance_to_point_scene(&app->scene, p, fminf);
		// if (occupied_distance < MIN_PLACEMENT_CLEARANCE) {
		// 	return;
		// }
		Sphere sphere = build_sphere(p, BALL_RADIUS);
		SceneElem elem = build_scene_elem_sphere(sphere);
		add_scene_elem_scene(&app->scene, elem);
		app->ball_id += 1;
	}
}

static Color render_ray(Ray ray, void* ctx) {
	RenderCtx* render_ctx = (RenderCtx*)ctx;
	if (!render_ctx || !render_ctx->scene) return COLOR_BLACK;

	const u32 max_iterations = 100;
	const f32 epsilon = 1e-6f;

	Vec3 p = ray.init;
	f32 t = distance_on_ray_scene(render_ctx->scene, ray, smooth_min_scene_default);
	if (!isfinite(t)) return COLOR_BLACK;

	for (u32 i = 0; i < max_iterations; i++) {
		p = trace_ray(ray, t);
		const f32 d = distance_on_ray_scene(render_ctx->scene, build_ray(p, ray.dir), smooth_min_scene_default);
		if (!isfinite(d)) {
			return COLOR_BLACK;
		}

		if (d < epsilon) {
			const Vec3 normal = normal_to_point_scene(render_ctx->scene, p, NULL);
			return (Color) {
				.red = (normal.x + 1.0f) * 0.5f,
				.green = (normal.y + 1.0f) * 0.5f,
				.blue = (normal.z + 1.0f) * 0.5f,
				.alpha = 1.0f,
			};
		}

		t += d;

		if (d > 10.0f) {
			return (Color) {
				.red = 0.0f,
				.green = 0.0f,
				.blue = 10.0f * ((f32)i / (f32)max_iterations),
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
	};

	if (app->use_parallel) {
		ray_map_camera_parallel(&app->camera, app->tela, render_ray, &render_ctx);
	} else {
		ray_map_camera(&app->camera, app->tela, render_ray, &render_ctx);
	}

	if (app->show_debug) {
		debug_scene(&app->scene, &(SceneDebugProps) { .camera = &app->camera, .tela = app->tela });
	}

	set_window_title(
		app->window,
		format_string("SDF Editor | FPS: %.2f | Render: %s (P) | Debug: %s (D)",
			1.0f / dt,
			app->use_parallel ? "Parallel" : "Serial",
			app->show_debug ? "ON" : "OFF")
	);
	paint_window(app->window, app->tela);

	if (fmodf(time, 10.0f) < 0.5f) {
		app->scene.vtable->rebuild_scene(&app->scene);
	}
}

static void on_close(Window* window, void* ctx) {
	Loop* animation = (Loop*)ctx;
	stop_loop(animation);
}

static void on_mouse_down(Window* window, i32 x, i32 y, u32 button, void* ctx) {
	App* app = (App*)ctx;

	if (button == 1) {
		app->left_mouse_down = true;
	} else {
		app->right_mouse_down = true;
	}
	app->mouse = vec2((f32)x, (f32)y);
}

static void on_mouse_up(Window* window, i32 x, i32 y, u32 button, void* ctx) {
	App* app = (App*)ctx;
	app->left_mouse_down = false;
	app->right_mouse_down = false;
	app->mouse = vec2(0, 0);
}

static void on_mouse_move(Window* window, i32 x, i32 y, void* ctx) {
	App* app = (App*)ctx;

	Vec2 new_mouse = vec2((f32)x, (f32)y);
	Vec2 delta = sub_vec2(new_mouse, app->mouse);

	if (app->left_mouse_down) {
		add_ball_from_screen(app, x, y);
	}

	if (app->right_mouse_down) {
		Vec3 orbit = get_camera_orbit(&app->camera);
		set_orbit_camera(
			&app->camera,
			orbit.x,
			orbit.y - 2.0f * PI * (delta.x / (f32)app->tela->width),
			orbit.z - 2.0f * PI * (delta.y / (f32)app->tela->height)
		);
	}

	app->mouse = new_mouse;
}

static void on_mouse_scroll(Window* window, i32 delta_y, void* ctx) {
	App* app = (App*)ctx;
	Vec3 orbit = get_camera_orbit(&app->camera);
	set_orbit_camera(&app->camera, orbit.x + (f32)delta_y * 0.1f, orbit.y, orbit.z);
}

static void on_key_down(Window* window, u32 keycode, void* ctx) {
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
	Window* window = new_window(WIDTH, HEIGHT, "SDF Editor");

	Scene scene = new_kscene(20);
	Camera camera = create_camera(vec3(5.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 1.0f);

	App app = {
		.tela = tela,
		.window = window,
		.scene = scene,
		.camera = camera,
		.left_mouse_down = false,
		.right_mouse_down = false,
		.mouse = vec2(0, 0),
		.ball_id = 0,
		.show_debug = false,
		.use_parallel = true,
	};

	Loop* animation = loop(on_frame, &app);
	on_close_window(window, on_close, animation);
	on_mouse_down_window(window, on_mouse_down, &app);
	on_mouse_up_window(window, on_mouse_up, &app);
	on_mouse_move_window(window, on_mouse_move, &app);
	on_mouse_scroll_window(window, on_mouse_scroll, &app);
	on_key_down_window(window, on_key_down, &app);

	play_loop(animation);
	return 0;
}
