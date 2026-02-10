# Tela

struct Tela {
    width: u32,
    height: u32,
    data: f32[width * height * 4], // RGBA format
}

struct Color {
    r: f32,
    g: f32,
    b: f32,
    a: f32,
}

struct Maybe<T> {
    value: T,
    has_value: bool,
}

struct Vec3 {
    x: f32,
    y: f32,
    z: f32,
}


Tela* map(Tela* tela, lambda: (x: u32, y: u32) => Maybe<Color>) {
    u32 channels = 4;
    u32 total_pixels = tela->width * tela->height;
    u32 size = total_pixels * channels;
    u32 width = tela->width;
    u32 height = tela->height;

    for k in 0..size {
        u32 x = (k / channels) % width;
        u32 y = (k / channels) / width;
        Maybe<Color> color = lambda(x, y);
        color.map((color) => {
            tela->data[k + 0] = color.r;
            tela->data[k + 1] = color.g;
            tela->data[k + 2] = color.b;
            tela->data[k + 3] = color.a;
        });
    }
}

Tela* create_tela(u32 width, u32 height) {
    Tela* tela = allocate<Tela>();
    tela->width = width;
    tela->height = height;
    tela->data = allocate_array<f32>(width * height * 4);
    return tela;
}