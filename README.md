# tela.c

Experimental graphic library from scratch(software-only), with reference implementation of computer graphics algorithms.

![](/tela.png)

## Purpose

The purpose of this graphic engine is to be able to generate images in a computational way, with minimal dependencies, such that the readable graphical algorithms shine instead of opaque graphical APIs. The engine should also be capable to create videos and interactive demos or games. 

It is a port of [tela.js](https://github.com/pedroth/tela.js).

# Table of Contents

- [Quick start](#quick-start)
- [Dependencies](#dependencies)
- [Acknowledgements](#acknowledgements)
- [TODOs](#todos)

# Quick start

## In the browser (webAssembly)



## On the desktop
Git clone this repository and `cd` into it. Then create a main.c file that uses `tela.c`.


## Generate images and videos

### Note on generating videos

To generate videos and images `tela.c` needs [ffmpeg][ffmpeg] in your system, in a way that it is possible to write on the console:
```bash
ffmpeg -version 

# it should output something like: fmpeg version 4.4.2-0ubuntu0.22.04.1...
# maybe with a different OS...

```

# Dependencies

- [`clang`][clang]
- [`ffmpeg`][ffmpeg]
- [`SDL2`][sdl]
- [xMake](https://xmake.io/#/)


# Acknowledgements

- [Keenan's 3D Model Repository](https://www.cs.cmu.edu/~kmcrane/Projects/ModelRepository/)
- [The models resource](https://www.models-resource.com/)
- [otaviogood fonts](https://github.com/otaviogood/shader_fontgen)



[ffmpeg]: https://ffmpeg.org/
[clang]: https://clang.llvm.org/
[sdl]: https://www.libsdl.org/

