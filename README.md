![build](https://github.com/mattiasgustavsson/dos-like/workflows/build/badge.svg)
---
![yarnspin](images/yarnspin_logo.png)

**Yarnspin** is a simple story-telling engine, with its own built-in scripting language and graphics processing, turning hi-res photos into palettized low-res pixels.

There is a custom script language, which allows you to define dialogs with options, and set up locations with descriptions and images. It comes with a thousand portrait images included, and you can of course add your own.

Yarnspin games runs on Windows, Mac, Linux, and in browsers using web assembly.

The documentation is pretty thin at the moment - there's just this readme, which is very much work-in-progress and likely incomplete - but it comes with an example "game" which is also a tutorial explaining the key concepts. There is also a small game made with Yarnspin, called No Sunshine, and it is open source and available here https://github.com/mattiasgustavsson/no_sunshine


## Usage

When you run `yarnspin.exe` it will compile all the scripts and assets into a single compressed `yarnspin.dat` file. You can then distribute `yarnspin.exe` and `yarnspin.dat`, and that is the complete game ready for distribution. If there is no `scripts` folder in the same location as `yarnspin.exe`, it won't attempt compilation.

When compiling a yarn, it will load all files, regardless of extension, in the `scripts` folder and try to compile them. It doesn't matter what you put in different files, all files will be loaded and processed in one go. A script file can contain many `sections`, where a section is declared by putting three equal signs before and after its name - and names must be unique across all files. Like this:
```
=== my_section ===
```
Everything that comes before the first section in a file is read as a `global`, see below.

Sections comes in three flavours: location, dialog and character, but they are all declared that same way.

Note: as you read this documentation, make sure you have played through the tutorial game, which illustrates the concepts described here. It probably also helps to revisit it and look at its scripts as you read through the docs.

The tutorial game can be played here:

    https://mattiasgustavsson/wasm/yarnspin
    

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

A section is determined to be a location section if it contains any of these declarations listed, and no other types of declarations.


### Dialog sections

A dialog section can not use the `img`, `txt` or `chr` declarations, but it can use the `act` and `use` declarations as described for the location sections, including conditions. 

The main part of the dialog are phrase declarations, which consists of a character name followed by some text:
```
some_character: Hello!
some_character: Good to see you again
```
Note that `some_character` must have been declared somewhere as a character section (see below). The character section defines the displayed name of the character, and what image is used as its portrait.

You can also use the pre-defined character `player` for lines that are spoken by the player
```
player: Hey, maybe you can help me?
some_character: I certainly hope so!
player: You are most kind.
```
Lines spoken by `player` will be displayed at the bottom of the screen and can be in a different font/color.

There can be as many phrase declarations as you like, and they will be run through in sequence. After all have been displayed, you can have `use` declarations, working just like for location sections, and `say` declarations, which allows the player to choose what to say next. They work just like the `opt` declarations for location sections, but for dialogs.
```
some_character: How can I help you?

say: I don't think you can...
act: some_section

say: I need a million dollars, right now!
act: other_section
```
Just as for location sections, these can have conditions, and the `act` statement is a section (dialog or location) to jump to when the option is selected.

A section is determined to be a dialog section if it contains any of the declarations listed for dialog sections, and no other types of declarations.

### Character sections

Character sections are much simpler, and you can not jump to a character section with an `act` statement. A character section defines the name and appearance for a character only.

```
=== some_character ===
name: The Abominable Snowman    
short: Frosty
face: cool_portrait.png
```
The section name `some_character` is used in location sections, in the `chr` statements, and in dialog sections in phrase declarations. The section name is not displayed anywhere, it is just for referring to the character.

When a character is added to a location using the `chr` statement, the `short` name is displayed in the list to the left on the screen.

When a dialog is playing, the longer `name` is displayed above the portrait picture defined as `face`. All portrait images must be in the `faces` folder. There are 1000 auto generated portrait images included, but you can of course make your own as well.


### Globals

Any declarations that appear before the first section definition in a file is a global declaration. These control the overall appearance and behaviour of the game. Each global can be declared only once throughout all files, but it doesn't matter which globals are declared in what file.

The list of globals are:


```
title
```
The name of this yarn - will be displayed in the title bar of the window of native builds.


```
author
```
Your name, as the author of the yarn


```
start
```
Specifies which section the game starts in. Must be a defined dialog or location section.


```
items
```
Items, as used with `get`/`drop`/`use` declarations, doesn't have to be declared ahead of referring to them. But sometimes you might want to, as to avoid spelling mistakes and hard to find bugs. If you specify the `items` global, it must contain a comma separated list of ALL items referred to in any `get`/`drop`/`use` statements in your scripts, or you will get a compile error. Specifying `items` is optional, but if you do specify it, all items must be listed.


```
flags
```
Just as for items, you might want to explicitly pre-define all flags before using them in `set`/`clear`/`toggle` statements or conditions. The `flags` global is optional, but if present it must list all flags.


```
palette
```
This points to an image file in the `palettes` folder, which will be processed and used as the palette for the game. An image used as a palette must have at most 256 distinct colors, but may have less. To process images for the game, a look up table has to be generated the first time a palette is used, and this can be a slow operation, especially for palettes with many colors. But it only needs to be done once per palette, and only while you are writing your yarn - the final distribution contains just pre-processed, run-time ready data.


```
display_filters
```
Yarnspin has two different CRT emulation filters, emulating the look of either an old TV or an old PC monitor. This global allows you to specify which one to use, like `display_filters: tv` or `display_filters: pc`. You can turn the filter off as well, for crisp pixels: `display_filters: none`. It is also possible to specify a list of filters, in which case the player will be able to cycle through them, in the order specified, by pressing F9 in the game. Declaring multiple filters looks like this: `display_filters: tv, pc, none`.


```
logo
```
Specifies one or more image files, as a comma separated list, to display at the start of the game, before jumpin to the first section. Images needs to be in the `images` folder, and will be displayed in order, waiting for player input to dismiss each one.


```
alone_text
```
On the left of the screen is a list of characters present at the current location. If there are no characters in a location, the default is to display a text there instead that says `You are alone.`. Using this global, you can change what the text says, or disable it altogether by simply specifying `alone_text:`


```
font_description
font_options
font_characters
font_items
font_name
```
These globals specify font files to use for the various text areas in the game. Font files must all be stored in the `fonts` folder, and must be .ttf files containing pixel fonts. A selection of fonts have been included. If these globals are not specified, the default fonts will be used.



```
background_location
```
Specifies an image file (present in the `images` folder) to use as a background when the game is displaying a location section. Check out the `images/yarnspin_location_template.gif` for a template file indicating the layout of the locations screen.


```
background_dialog
```
Specifies an image file (present in the `images` folder) to use as a background when the game is displaying a dialog section. Check out the `images/yarnspin_dialog_template.gif` for a template file indicating the layout of the dialog screen.



```
color_background
color_disabled
color_txt
color_opt
color_chr
color_use
color_name
color_facebg
```
These globals controls the text display color for the various text areas in the game. They specify the index in the palette (0 to 255) of the color to use for each text. If not specified, defaults will be calculated and used.


## Building the code

No build system is used, simply call the compiler from the commandline.


### Windows

From a Visual Studio Developer Command Prompt, do:
```
  cl source\yarnspin.c
```  

For building the final release version, you probably want all optimizations enabled. There's a helper script (a windows bat file) in the `build` folder of the repo, which will build with full optimizations, and also include an application icon. It will also call the compiled exe to generate the `yarnspin.dat` data file, and then append the file to the end of the executable, giving you a single exe you can distribute which contains both code and data. No need to include the yarnspin.dat file. See the `build\build_win.bat` file for details.


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

For a final release, you probably want the web page it is embedded on to look a bit nicer - there is a helper build script `build\build_web.bat` which does this, specifying a template html file.
