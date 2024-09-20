# cdlv
Library and scripting system for ADV/VN style games made in pure C.

Easy to implement basically anywhere.

## Features
- **Simple C-inspired scripting language**
- **Play anywhere thanks to HTTP REST API server capabilities**
- **Rendering and windowing backend independent - use it with anything you like**
- **Simple to setup with callbacks API**
- **Supports decoding various video formats thanks to ffmpeg**

## Planned features
- choices and goto
- more human callback config api

## In progress
- extensively test script behaviour
- fix/extensively test ffmpeg decoding

## Documentation

Documentation can be found in [this repository's wiki](https://github.com/damemay/cdlv/wiki)

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
Refer to [sample.c](sample.c) and [res/sample/sample.cdlv](res/sample/sample.cdlv) for a documented sample desktop app with SDL2.

# mdlv

CDLV parsing REST API HTTP server implemented with Mongoose.

Using the server from code is as simple as this:
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

# Building
Building binaries requires linking with `ffmpeg` (and `SDL2`, `SDL2_image`, `SDL2_ttf` for `cdlv-sample`) libraries installed on system.

```
git clone --recurse-submodules https://github.com/damemay/cdlv.git
cd cdlv
mkdir build && cd build
cmake .. && make
```

## CMake options:
- `CDLV_FFMPEG=ON` - Include FFmpeg as a library dependency for video decoding when linking
- `CDLV_MONGOOSE=ON` - Include `mongoose.c` and `mdlv.c` inside library
- `CDLV_SAMPLE=ON` - Build `cdlv-sample`
- `CDLV_MDLV_SERVER=ON` - Build `mdlv-server`
