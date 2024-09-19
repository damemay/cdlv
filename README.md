# cdlv
Library and scripting system for ADV/VN style games made in pure C.

Easy to implement into existing SDL2 renderer-based application or anywhere thanks to Mongoose.

## Features
- **Simple C-inspired scripting language**
- **Play anywhere thanks to HTTP REST API server abilities and sample client implementation**
- **Supports various image and video formats with SDL2_image and ffmpeg**

## In progress
- scene/script behaviorism tests
- fix ffmpeg green frame

## Things to come
- choices and goto reimplementation
- cdlv-menu reimplementation app

## Scripting example
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
Refer to [sample.c](sample.c) and [res/sample/sample.cdlv](res/sample/sample.cdlv) for a documented sample desktop app.

## MDLV

CDLV parser HTTP server implemented with Mongoose.

Starting a server from code is as simple as below:
```c
mdlv mdlv_base = {
    .host = "http://localhost:8080",
    .path = "/var/www/scripts/",
    .web_root = "/var/www/web_root/",
};
if(mdlv_init(&mdlv_base) != cdlv_ok) exit(1);
for(;;) mg_mgr_poll(&mdlv_base.manager, 50);
mdlv_free(&mdlv_base);
```
Refer to [server.c](server.c) and [web_root/script.js](web_root/script.js) for working server and browser client implementation.

## Building
Building requires SDL2, SDL2_image and SDL2_ttf and ffmpeg libraries installed on system.

```
git clone https://github.com/damemay/cdlv.git
cd cdlv
mkdir build && cd build
cmake .. && make
```

### CMake options:
- `CDLV_SAMPLE=ON` - Build `cdlv-sample`
- `CDLV_MONGOOSE=ON` - Include mongoose and mdlv in library for MDLV support
- `CDLV_MDLV_SERVER=ON` - Build `mdlv-server`
- `CDLV_MENU=ON` - Build `cdlv-menu`
