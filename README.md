![build](https://github.com/mattiasgustavsson/dos-like/workflows/build/badge.svg)
---
![yarnspin](images/yarnspin_logo.png)

**Yarnspin** is a simple story-telling engine, with its own built-in scripting language and graphics processing, turning hi-res photos into palettized low-res pixels.

There is a custom script language, which allows you to define dialogs with options, and set up locations with descriptions and images. It comes with a thousand portrait images included, and you can of course add your own.

Yarnspin games runs on Windows, Mac, Linux, and in browsers using web assembly.

The documentation is pretty non-existing at the moment - there's just this readme - but it comes with an example "game" which is also a tutorial explaining the key concepts. There is also a small game made with Yarnspin, called No Sunshine, and it is open source and available here https://github.com/mattiasgustavsson/no_sunshine


## Building

No build system is used, simply call the compiler from the commandline.


### Windows

From a Visual Studio Developer Command Prompt, do:
```
  cl source\yarnspin.c
```  


### Mac

```
  clang source/yarnspin.c `sdl2-config --libs --cflags` -lGLEW -framework OpenGL -lpthread
```

SDL2 and GLEW are required - if you don't have them installed you can do so with Homebrew by running
```
  brew install sdl2 glew  
```


### Linux

```
  gcc source/yarnspin.c `sdl2-config --libs --cflags` -lGLEW -lGL -lm -lpthread
```

SDL2 and GLEW are required - if you don't have them installed you can do so on Ubuntu (or wherever `apt-get` is available) by running
```
  sudo apt-get install libsdl2-dev
  sudo apt-get install libglew-dev
```


### WebAssembly

Using WAjic:
```
  wasm\node wasm/wajicup.js -embed yarnspin.dat yarnspin.dat source/yarnspin.c yarnspin.html
```

Note that you must have generated the `yarnspin.dat` file (by running the yarnspin executable once) before running this build command.

A WebAssembly build environment is required. You can download it (for Windows) here: [wasm-env](https://github.com/mattiasgustavsson/dos-like/releases/tag/wasm-env).
Unzip it so that the `wasm` folder in the zip file is at your repository root.

The wasm build environment is a compact distribution of [node](https://nodejs.org/en/download/), [clang/wasm-ld](https://releases.llvm.org/download.html),
[WAjic](https://github.com/schellingb/wajic) and [wasm system libraries](https://github.com/emscripten-core/emscripten/tree/yarnspin/system).
