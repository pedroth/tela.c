# tela.c

`tela.c` is a `C` port of _pedroth's `tela.js`_, a software-only graphic library from scratch, with reference implementation of computer graphics algorithms.

![](/tela.png)

## Purpose

The purpose of this graphic engine is to be able to generate images in a computational way, with minimal dependencies, such that the readable graphical algorithms shine instead of opaque graphical APIs. The engine should also be capable to create videos and interactive demos or games. 

## Port details

It tries to be as close as possible to the original `tela.js`, but with some minor differences regarding the differences between C and JS. There are also some minor improvements over the original. 

> Many parts of the port were done with the help of _claude opus 4.6_. Without it, this port would have taken much longer to be done.

# Table of Contents

- [Quick start](#quick-start)
- [Acknowledgements](#acknowledgements)
- [TODOs](#todos)

# Quick start

## Installing Dependencies

- Install [SDL2][sdl] and [FFMPEG][ffmpeg] in your system
  - SDL2(UBUNTU): `sudo apt-get install libsdl2-dev`
  - ffmpeg(UBUNTU): `sudo apt-get install ffmpeg`
- Have a C compiler (like [clang][clang] or [gcc][gcc]) installed

## Trying the examples

Just git clone the repo, then compile any of the examples in the `test` as follows:

```bash
gcc -O3 -fopenmp -o <executable name> test/<demo name>.c -lSDL2 -lm && ./<executable name>
```

Example:
```bash
gcc -O3 -fopenmp -o app test/meshes_sky.c -lSDL2 -lm && ./app
```

For even more performance, you could do:
```bash
gcc -O3 -fopenmp -ffast-math -o app test/meshes_sky.c -lSDL2 -lm -march=native && ./app
```

# Acknowledgements

- [Keenan's 3D Model Repository](https://www.cs.cmu.edu/~kmcrane/Projects/ModelRepository/)
- [The models resource](https://www.models-resource.com/)
- [Alec Jacobson's common 3D test models](https://github.com/alecjacobson/common-3d-test-models/)
- [tela.js](https://github.com/pedroth/tela.js)


[ffmpeg]: https://ffmpeg.org/
[gcc]: https://gcc.gnu.org/
[clang]: https://clang.llvm.org/
[sdl]: https://www.libsdl.org/

