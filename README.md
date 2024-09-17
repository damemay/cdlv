# cdlv
Simple SDL2-based library and scripting system for ADV/VN style games made in pure C.

Easy to implement into existing SDL2-based application.

## Features
- **Simple C-inspired scripting language**
- **Compiles to static library**
- **Supports various image and video formats with SDL2_image and ffmpeg**

## In progress
- scene/script behaviorism tests
- mdlv reimplementation

## Things to come
- choices and goto reimplementation
- cdlv-menu reimplementation app

## Example
```
!resources_path "images/"
!resources {
    "black.png"
    "cloudy_sky.png"
    "sky_clearing.mp4"
    "sunny_sky.png"
}
!scene countdown {
    @bg cloudy_sky
    "I don't like rainy days. I hope it will get sunnier soon..."
    @bg black
    Later that day...
    @bg sky_clearing once
    @bg cloudy_sky
    "Ah, there's the sun! I'm already feeling better!"
}
```
Refer to [sample.c](sample.c) and [res/sample/sample.cdlv](res/sample/sample.cdlv) for a full sample documented app.

## Building
Building requires SDL2, SDL2_image and SDL2_ttf and ffmpeg libraries installed on system.

```
git clone https://github.com/damemay/cdlv.git
cd cdlv
mkdir build && cd build
cmake .. && make
```

Above will build `cdlv-sample` and static library `libcdlv.a`
