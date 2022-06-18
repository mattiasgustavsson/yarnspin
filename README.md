![build](https://github.com/mattiasgustavsson/dos-like/workflows/build/badge.svg)
---
![yarnspin](images/yarnspin_logo.png)

**Yarnspin** is a simple story-telling engine, with its own built-in scripting language and graphics processing, turning hi-res photos into palettized low-res pixels.

There is a custom script language, which allows you to define dialogs with options, and set up locations with descriptions and images. It comes with a thousand portrait images included, and you can of course add your own.

Yarnspin games runs on Windows, Mac, Linux, and in browsers using web assembly.

The documentation is pretty non-existing at the moment - there's just this readme - but it comes with an example "game" which is also a tutorial explaining the key concepts. There is also a small game made with Yarnspin, called No Sunshine, and it is open source and available here https://github.com/mattiasgustavsson/no_sunshine


## Usage

When you run `yarnspin.exe` it will compile all the scripts and assets into a single compressed `yarnspin.dat` file. You can then distribute `yarnspin.exe` and `yarnspin.dat`, and that is the complete game ready for distribution. If there is no `scripts` folder in the same location as `yarnspin.exe`, it won't attempt compilation.

When compiling a yarn, it will load all files in the `scripts` folder and try to compile them. A script file can contain many `sections`, where a section is declared by putting three equal signs before and after its name - and names must be unique across all files. Like this:
```
=== my_section ===
```
Everything that comes before the first section in a file is read as a `global`, see below.

Sections comes in three flavours: location, dialog and character, but they are all declared as described above.


### Location sections

A location section can contain one or more image and text declarations, as well as options. Each declaration can optionally have a condition before it, and the declaration will only be included if the condition evaluates to true. Conditions can only test flags, and are written with the flag name before a question mark like this:
```
my_flag ? txt: This text will only display if my_flag has been set
```
You can check if a flag is not set by placing the word `not` before it
```
not my_flag ? txt: This text will only display if my_flag has NOT been set
```
You can check if any out of a list of flags are set, by listing multiple flags
```
my_flag other_flag third_flag ? txt: This text will only display if ANY of the three flags have been set
```
You can check if several flags are set by writing them as multiple condition statements
```
my_flag ? other_flag ? third_flag ? txt: This text will only display if all three flags have been set
```
Please note that when using `not` for multiple flags, the `not` is only apply for the single flag following it, not to the whole list of flags.

A section can use `img` to display an image
```
img: picture_of_a_room.jpg
```
Note that all images must be in the `images` folder. If multiple images are specified, only one will actually be displayed - use condition statements to control which one.

A section can use `txt` to display text, and it can have multiple txt statements to display multiple texts
```
txt: This text will be displayed.
txt: And so will this.
```
Note that you can use conditions to control which texts are displayed.

A section can use `act` to perform action, like setting, clearing or toggling a flag 
```
act: set my_flag
act: clear other_flag
act: toggle third_flag
```
or go to another section, after the player clicks the mouse or press a key to dismiss the current one
```
act: my_other_section
```

The `act` statement can also be used to get or drop items
```
act: get Some item
```
or
```
act: drop Some item
```

After the img/txt/act declarations, a location section can have `use`, `chr` and `opt` declarations

`chr` declarations adds a character to the character list, and if the player clicks on it, we go to the section specified in its corresponding `act` statement. A `chr` declaration is always followed by an `act` statement with a section name.
```
chr: some_character
act: talk_to_character
```
Note that `some_character` needs to be defined somewhere as a character section (see below) and that `talk_to_character` needs to be defined as either a location section or a dialog section

`use` works similarly, but for items
```
use: Some item
act: section_describing_what_happens
```
If the player does not currently have the item in his inventory, the use declaration is ignored.

`opt` adds an option at the bottom of the screen, and also has a corresponding `act` declaration specifying a section to go to
```
opt: I want to go to the other section
act: my_other_section
```

Note that `use`, `chr` and `opt` declarations can also have conditions specified before them, and the condition will control whether the following use/chr/opt declaration is active or not.


### Dialog sections


### Character sections


### Globals

## Building the code

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
