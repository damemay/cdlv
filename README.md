# cdlv
Simple SDL2-based engine and scripting system for ADV/VN style games being made in pure C.

The goal is to create as lowest resource usage engine as possible. This is why the scripting system divides every data-heavy behavior into scenes with separately loaded resources.

## Current state
- **Simple scripting, scene-based framework**
- **Can be used as a separate part of another program**
- **Scenes can:**
  - have static backgrounds that are changed based on script file;
  - have looped animation;
  - have single loop animation;
  - prompt to jump to another scene based on player choice or scripted behavior.

## Example
  
##### example.adv
```
640 480 3 res/esteban.ttf 32

!scene
  !bg
    res/cloudy_sky.png
    res/black.png
  !script
    "I don't like rainy days. I hope it will get sunnier soon."
    @image 1
    Later that day...

!scene
  !anim_once
    res/anim/frame0.jpg
    res/anim/frame1.jpg
    res/anim/frame2.jpg
    res/anim/frame3.jpg
    res/anim/frame4.jpg
    res/anim/frame5.jpg
    res/anim/frame6.jpg
    res/anim/frame7.jpg
    res/anim/frame8.jpg
    res/anim/frame9.jpg

!scene
  !bg
    res/sunny_sky.png
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
        .text_x             = 50,
        .text_y             = 400,
        .text_wrap          = 1200,
        .text_color         = {255, 255, 255, 255},
        .text_render_bg     = true,
        .text_speed         = 20,

        .dissolve           = true,
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
  [images width] [images height] [framerate for animations] [font .ttf path] [font size]
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
    - `@end` ends parsing lines as choices and displays them on screen.
- **All indexes start from 0.**
- Default scene image starts from the 0 index, so there's no need to set it at the start of script.
- Paths can be either full or relative from the executable file.
- Images that are smaller than declared size are scaled to match it.

## To do
- [ ] rename scene definition tags.
- [x] dissolve effect between images.
- [x] typewriter effect.
- [ ] make a consistent gui.
- [x] refactor parsing code.
- [x] anim_once option --> automatically continue or wait for player input
  - also allow for text to be rendered if there is script added.
- [x] app for playtesting .adv files, similiar to renpy.
  - currently as a proof of concept in menu.c (to be improved upon later on)
- [x] ~~4-button~~ choice system + ~~flowscripts for~~ jumping between scenes.
