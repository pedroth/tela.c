# tela.c

Experimental graphic library from scratch(software-only), with reference implementation of computer graphics algorithms.

![](/tela.png)

## Purpose

The purpose of this graphic engine is to be able to generate images in a computational way, with minimal dependencies, such that the readable graphical algorithms shine instead of opaque graphical APIs. The engine should also be capable to create videos and interactive demos or games. 

It is a port of [tela.js](https://github.com/pedroth/tela.js), made with the help of AI(Claude opus, gemini, gpt) and some minor conceptual improvements from original.

# Table of Contents

- [Quick start](#quick-start)
- [Dependencies](#dependencies)
- [Acknowledgements](#acknowledgements)
- [TODOs](#todos)

# Quick start

## In the browser (webAssembly)



## On the desktop

Install sdl2 in your system, download and include `src/index.c` in your file, then compile it using _clang_ or any C compiler, linking the SDL2 library and math library. 

// minor c example to create a window and render something on it using tela.c:

```c
// my_app.c
#include "index.c"


```

Then compile it using:
```bash
clang -o my_app my_app.c -lSDL2 -lm
```

For optimized compilation:
```bash
clang -O3 -o my_app my_app.c -lSDL2 -lm
```
or 
```bash
clang -O3 -march=native -ffast-math -o my_app my_app.c -lSDL2 -lm
```


## Generate images and videos

### Note on generating videos

To generate videos and images `tela.c` needs [ffmpeg][ffmpeg] in your system, in a way that it is possible to write on the console:
```bash
ffmpeg -version 

# it should output something like: fmpeg version 4.4.2-0ubuntu0.22.04.1...
# maybe with a different OS...

```

# Dependencies

- [`clang`][clang] or any C compiler
- [`ffmpeg`][ffmpeg]
- [`SDL2`][sdl]
- [xMake _not really necessary_](https://xmake.io/#/)


# Acknowledgements

- [Keenan's 3D Model Repository](https://www.cs.cmu.edu/~kmcrane/Projects/ModelRepository/)
- [The models resource](https://www.models-resource.com/)
- [otaviogood fonts](https://github.com/otaviogood/shader_fontgen)
- [tela.js](https://github.com/pedroth/tela.js)



[ffmpeg]: https://ffmpeg.org/
[clang]: https://clang.llvm.org/
[sdl]: https://www.libsdl.org/

