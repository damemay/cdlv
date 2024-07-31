# cdlv
Simple SDL2-based library and scripting system for ADV/VN style games made in pure C.

~~The goal was to create as lowest resource usage engine as possible. This is why the scripting system divides every data-heavy behavior into scenes with separately loaded resources.~~

Easy to implement into existing SDL2-based application.

## Features
- **Simple C-inspired scripting language**
- **Compiles to static library**

## Example
Refer to [main.c](main.c) and [res/sample/sample.cdlv](res/sample/sample.cdlv).

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