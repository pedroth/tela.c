set_languages("c99")
set_warnings("all")
if is_mode("debug") then
    set_optimize("none")
    set_symbols("debug")
else
    set_optimize("fastest")
end
set_toolchains("clang")


target("simple_animation")
    set_kind("binary")
    add_files("test/simple_animation.c")
    add_packages("sdl2")
    add_links("SDL2")

target("amazing_shader")
    set_kind("binary")
    add_files("test/amazing_shader.c")
    add_packages("sdl2")
    add_links("SDL2")

target("wave")
    set_kind("binary")
    add_files("test/wave.c")
    add_packages("sdl2")
    add_links("SDL2")

target("line_test")
    set_kind("binary")
    add_files("test/line_test.c")
    add_packages("sdl2")
    add_links("SDL2")

target("sdf_test")
    set_kind("binary")
    add_files("test/sdf_test.c")
    add_packages("sdl2")
    add_links("SDL2")

target("image_gen")
    set_kind("binary")
    set_rundir("$(projectdir)")
    add_files("test/image_gen.c")
    add_packages("sdl2")
    add_links("SDL2")
