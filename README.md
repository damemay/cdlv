# cdlv
Simple SDL2-based library and scripting system for ADV/VN style games made in pure C.

~~The goal was to create as lowest resource usage engine as possible. This is why the scripting system divides every data-heavy behavior into scenes with separately loaded resources.~~

Easy to implement into existing SDL2-based application.

## Features
- **Simple C-inspired scripting language**
- **Compiles to static library**

## Example
  
##### example.cdlv
```
# This is a comment. It's not parsed by CDLV.

# Lines starting with `!` are definitions.
# `!resources_path` defines a path for loading resources.
# Without this, CDLV will only look for resources in scripts directory.
!resources_path = "res/";

# Remember to end definition with a semicolon.

# Let's define resources. Resources are images or animations used just like variables.
# Because they will be loaded as soon as CDLV is called to play the script and live for it's whole life, try not to declare too much and leave this usage for reserving custom names to be used by a whole script/game.
# Let's reserve a name `empty` when we don't want to display anything and we want it to make it obvious inside our script:
!resources = {
  empty = "black_screen.png",
};

# Note: writing `!resources` again will delete previously defined resources (free memory and clear names) and overwrite them with new set.
# Same goes for `!resources_path`.

# For a script to run and display, there needs to exist a scene and, preferably, some text.
# Scene is a special in many ways. For example, it's definition requires a name, e.g. `sun_rising` below:
!scene sun_rising = {
  # Inside a scene, you can define new `!resources_path` and `!resources` that will be valid only inside this scene's scope.
  # Resources defined here will be loaded as soon as the scene is loaded by CDLV.
  # This is the only place where calling `!resources_path` and `!resources` again won't overwrite the "global" variables.
  # But calling them again in the same scene will cause this scene resources to be overwritten, so beware!
  
  # `!resources_path` inside a scene is appended to the global `!resources_path`, so the line below will make the CDLV look for resources declared here inside `res/sun_rising/`:
  !resources_path = "sun_rising/";
  !resources = {
    # There are some more options to loading resources, that work in both scopes, that weren't mentioned so far.
    # You can add a resource just as a filename. It's name will get taken from the filename, e.g. `cloudy_sky` for below lines:
    "cloudy_sky.png",
    # When adding a image-based animation, add a list with it's name defined:
    sky_clearing = {
      "anim/frame0.jpg",
      "anim/frame1.jpg",
      "anim/frame2.jpg",
      "anim/frame3.jpg",
      "anim/frame4.jpg",
      "anim/frame5.jpg",
      "anim/frame6.jpg",
      "anim/frame7.jpg",
      "anim/frame8.jpg",
      "anim/frame9.jpg",
    },
  };

  # Lines starting with `@` are prompts or functions.
  # Each character separated by a whitespace in the same line is it's argument.
  # Line below sets the current background image to file loaded from `res/cloudy_sky.png`:
  @bg cloudy_sky,

  # Notice that when calling a prompt (or later inserting text) inside a scene, the statement is finished with `,` and not `;`.
  # `;` is reserved only for ending definitions. Statements inside a scene must end with `,`.

  # Lines starting and ending with `"` are script lines that will be printed on screen.
  # Escape `"` with backslash if you want to include a character `"` in the text:
  "\"I don't like rainy days. I hope it will get sunnier soon.\"";

  @bg empty,
  "Later that day...",

  # Calls to `@bg` with one argument (a resource) like until now display resource as a static image.
  # If you want to play a loaded animation, you have to call `@bg resource time`, e.g.:
  #   `@bg resource once` for animation that plays once,
  #   `@bg resource loop` for looped animation.
  @bg sky_clearing once,

  # `.` is not allowed in resource name.
  # If there's a dot in resource name when calling `@bg`, CDLV will try to load a file with that name.
  # The paths it searches are the same as declared for the whole script.
  @bg sunny_sky.png,
  "\"Ah, it's the sun!\"",

  # You can also load a .mp4 video to display instead of a animation based on separate images.
  # Animation set as background at an end of scene will either loop till the program dies or end scene after finishing single loop.
  @bg rickroll.mp4 once,

  # On scene's end, all it's resources are freed.
};
```
##### main.c
```c
#include "cdlv.h"

int main() {
    uint32 screen_width = 640;
    uint32 screen_height = 480;
    // Initialize SDL2, SDL2_image and SDL2_ttf.
    // ...

    // Initialize CDLV's "base".
    // You can leave this in-memory whenever using CDLV functionality.
    cdlv* base = cdlv_new(screen_width, screen_height, SDL_Renderer*);

    // Define config for a script we will be loading.
    cdlv_config config = {
        .text_font = "res/esteban.ttf", // TTF file path (char*)
        .text_size = 32, // text size in points (uint8)
        .text_xy = {50, 400}, // text position vector2{x,y}
        .text_wrap = 1200, // text wrap position
        .text_color = {255, 255, 255, 255}, // color in RGBA format (uint8)
        .text_render_bg = true, // render semi-transparent background behind text (bool)
        .text_speed = 0, // text typewriting effect speed. 0 turns it off (uint8)
        .dissolve_speed = 4, // image dissolving effect speed. 0 turns it off (uint8)
    };
    
    // You can also load config from file.
    cdlv_load_config("res/config", &config);

    cdlv_set_config(config);
    cdlv_add_script("res/example.cdlv");

    while(sdl->run) {
        cdlv_loop_start(base, SDL_Event);
        cdlv_render(base, SDL_Renderer*);
        cdlv_loop_end(base, SDL_Renderer*);
    }

    cdlv_free(base);
}
``` 

## Building
Building requires SDL2, SDL2_image and SDL2_ttf installed on system.

```
git clone https://github.com/damemay/cdlv.git
cd cdlv
mkdir build && cd build
# Linux & MacOS:
cmake .. && make
```

Above will build `cdlv-menu` and static library `libcdlv.a`