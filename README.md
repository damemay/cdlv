# cdlv
Simple SDL2-based library/engine and scripting system for ADV/VN style games made in pure C.

The goal was to create as lowest resource usage engine as possible. This is why the scripting system divides every data-heavy behavior into scenes with separately loaded resources.

## Features
- **Simple scripting, scene-based framework**
- **Compiles to static library, so that it can be used as a separate part of another program**
- **Scenes can:**
  - have static backgrounds that are changed based on script file;
  - have looped animation;
  - have single loop animation;
  - prompt to jump to another scene based on player choice or scripted behavior.

## Example
  
##### example.adv
```
640 480 3 res/

!scene
  !bg
    cloudy_sky.png
    black.png
  !script
    "I don't like rainy days. I hope it will get sunnier soon."
    @image 1
    Later that day...

!scene
  !anim_once
    anim/frame0.jpg
    anim/frame1.jpg
    anim/frame2.jpg
    anim/frame3.jpg
    anim/frame4.jpg
    anim/frame5.jpg
    anim/frame6.jpg
    anim/frame7.jpg
    anim/frame8.jpg
    anim/frame9.jpg

!scene
  !bg
    sunny_sky.png
  !script
    "Ah, it's the sun!"
```
##### main.c
```c
#include "cdlv.h"

int main() {
    // SDL initialization can be done with provided wrapper or preferred way.
    sdl_base* sdl = sdl_create("sample", 640, 480);

    cdlv_config config = {
        .text_font          = "res/esteban.ttf",
        .text_size          = 32,

        .text_xy            = {50, 400},
        .text_wrap          = 1200,
        .text_color         = {255, 255, 255, 255},
        .text_render_bg     = true,
        .text_speed         = 0,

        .dissolve_speed     = 4,
    };

    // cdlv_init_from_script easily sets up cdlv structures for instant use.
    cdlv_base* base = cdlv_init_from_script(&config, "res/example.adv", &sdl->renderer);

    while(sdl->run) {
        cdlv_loop_start(base, &sdl->event, &sdl->run);
        cdlv_render(base, &sdl->renderer);
        cdlv_loop_end(base, &sdl->renderer);
    }

    cdlv_clean_all(base);
    sdl_clean(sdl);
    return EXIT_SUCCESS;
}
``` 

## Script syntax
  
- First line **must** contain data for the whole .adv file:

  ```
  [images width] [images height] [framerate for animations] [path to images]
  ```
- Each scene **must** be declared with tag: `!scene`
- Right below it should be one of the tags defining scene type:
  - `!bg` - static backgrounds scene,
  - `!anim` - animated loop scene,
  - `!anim_once` - single loop animation scene that skips when animation ends.
  - `!anim_wait` - single loop animation scene that waits for user input when animation ends.
  - `!anim_text` - single loop animation scene that renders script. to continue, animation has to end and the script has to be read.
    - Each consequent line below the tag is path to a single image/frame in .jpg/.png
- `!script` tag declares that each consequent line below is a text to be parsed:
  - blank lines are not parsed,
  - lines starting with `@` are prompts to call a function. Currently there are three:
    - `@image [index]` changes background image in static scenes;
    - `@goto [index]` jump to scene of index;
    - `@choice` starts parsing each consequent line as a option to be chosen by the player;
      - each choice is written as: `[index] [text]`
    - `@end` ends parsing lines as choices and displays them on screen.
- **All indexes start from 0.**
- First scene image always starts from 0 when it's loaded.
- Paths can be either full or relative from the executable file.
- Images that are smaller than declared size are scaled to match it.

## Building
First, install SDL2, SDL2_image and SDL2_ttf.

If not found within expected paths, you'll need to pass `CMAKE_PREFIX_PATH` or `SDL2_DIR` to cmake.

For building with `VITA`, you'll need Vita SDK.
```
git clone https://github.com/damemay/cdlv.git
cd cdlv
mkdir build && cd build
# Linux & MacOS:
cmake .. && make
# Windows (MinGW64):
cmake -DMINGW=ON .. && cmake --build .
# PS Vita:
cmake -DVITA=ON .. && make
```

Above will build `cdlv-menu` and leave static lib named `libcdlv.a`

# cdlv-menu
Simple application for launching and testing scripts.

## Usage

```
$ ./cdlv-menu 
cdlv-menu [config path] [scripts folder path]
```

## Config file

It can be any text file, where each line consists of option name followed by whitespace and it's value.

#### Options:

- `text_font` - string - path to .ttf file
- `text_size` - number up to 255 - text size in points
- `text_x` - number - text position on X-axis
- `text_y` - number - text position on Y-axis
- `text_wrap` - number - text wrap position
- `text_r` - number up to 255 - text color RED
- `text_g` - number up to 255 - text color GREEN
- `text_b` - number up to 255 - text color BLUE
- `text_a` - number up to 255 - text color ALPHA
- `text_render_bg` - 0 or 1 - whether to render semi-transparent background behind text
- `text_speed` - number up to 255 - text typewriting effect speed. 0 prints text instantly
- `dissolve_speed` - number up to 255 - dissolve effect speed. 0 shows image instantly
