# cdlv
Simple SDL2-based engine and scripting system for ADV/VN style games being made in pure C.

The goal is to create as lowest resource usage engine as possible. This is why the scripting system divides every type of behavior into scenes with separately loading resources.

## Current state
- **Simple scripting, scene-based framework:**
  - **Scenes can:**
    - have static backgrounds, changeable on script prompt;
    - have animated loops;
    - be animated only once (without text).

## Documentation
 
<details>
  
  <summary><b>Script syntax</b></summary>
  
- First line **must** contain data for the whole .adv file:

  ```
  [scene count] [images width] [images height] [framerate for animations] [font .ttf path] [font size]
  ```
- Each scene **must** be declared with tag: `!scene`
  - Right below it should be one of the tags defining scene type:
    - `!bg` - static backgrounds scene,
    - `!anim` - animated loop scene,
    - `!anim_once` - single loop animation scene.
  - Each consequent line below the tag is path to a single image/frame in .jpg/.png
  - `!script` tag declares that each consequent line below is a text to display:
    - blank lines are not parsed,
    - lines starting with `@` are prompts to change image. `@` should be followed by image index.
- All indexes start from 0.
- Default scene image starts from the 0 index, so there's no need to set it at the start of script.
- Paths can be either full or relative from the executable file.
- Images width/height that is smaller than window width/height will be scaled to match window.

</details>

<details>
  
  <summary><b>Sample script and main.c</b></summary>
  
##### sample.adv
```
2 640 480 3 res/esteban.ttf 32

!scene
  !bg
    res/cloudy_sky.png
    res/black.png
  !scene
    "I don't like rainy days. I hope it will get sunnier soon."
    @1
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
  !scene
    "Ah, it's the sun!"
```
##### main.c
```c
#include "cdlv.h"

int main() {
    cdlv_base* base = cdlv_create("sample", 640, 480);
    cdlv_read_file(base, "res/sample.adv");
    cdlv_start(base);
    
    while(base->run) {
        cdlv_loop_start(base);
        cdlv_render(base);
        cdlv_loop_end(base);
    }

    cdlv_clean(base);
    return EXIT_SUCCESS;
}
```
  
</details>

## To do
- [ ] 4-button choice system + flowscripts for jumping between scenes.
- [ ] .adv files formatter:
  - auto scene,resources,script lines counting, so the main app doesn't have to do that;
  - format into something that would allow more direct parsing.
- [ ] app for playtesting .adv files, similiar to renpy.
- [ ] transition effects between images (dissolve, fade in/out, etc.).
- [ ] typewriter effect.
- [ ] make a consistent gui
