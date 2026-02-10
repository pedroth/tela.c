set_languages("c99")
set_warnings("all")
if is_mode("debug") then
    set_optimize("none")
    set_symbols("debug")
else
    set_optimize("fastest")
end
set_toolchains("clang")

target("vec_benchmark")
    set_kind("binary")
    add_files("src/Vector/Vec.c", "src/utils/time.c")
    add_includedirs("src")
    add_packages("sdl2")
    add_links("SDL2")

target("simple_animation_window")
    set_kind("binary")
    add_files("test/simple_animation_window.c")
    add_packages("sdl2")
    add_links("SDL2")

target("amazing_shader_window")
    set_kind("binary")
    add_files("test/amazing_shader_window.c")
    add_packages("sdl2")
    add_links("SDL2")

target("wave_window")
    set_kind("binary")
    add_files("test/wave_window.c")
    add_packages("sdl2")
    add_links("SDL2")

target("line_test_window")
    set_kind("binary")
    add_files("test/line_test_window.c")
    add_packages("sdl2")
    add_links("SDL2")

target("sdf_test_window")
    set_kind("binary")
    add_files("test/sdf_test_window.c")
    add_packages("sdl2")
    add_links("SDL2")

target("image_gen")
    set_kind("binary")
    set_rundir("$(projectdir)")
    add_files("test/image_gen.c")
    add_packages("sdl2")
    add_links("SDL2")
