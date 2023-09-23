#ifndef CDLV_TYPES_H
#define CDLV_TYPES_H

#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "cdlv_macros.h"

typedef enum {
    cdlv_static_scene,
    cdlv_anim_scene,
    cdlv_anim_once_scene,
    cdlv_anim_wait_scene,
    cdlv_anim_text_scene,
    cdlv_script,
    cdlv_scene_decl,
    cdlv_none,
} cdlv_type;

typedef enum {
    cdlv_main_menu,
    cdlv_main_run,
} cdlv_state;

typedef enum {
    cdlv_parsing,
    cdlv_parsed,
} cdlv_parse_state;

typedef struct cdlv_choice {
    size_t count;
    size_t current;
    cdlv_parse_state state;
    size_t* destinations;
    char** options;
    char* prompt;
} cdlv_choice;

typedef struct cdlv_scene {
    SDL_Surface** images;
    char** image_paths;
    size_t image_count;
    char** script;
    size_t line_count;
    cdlv_type type;
} cdlv_scene;

typedef struct cdlv_canvas {
    SDL_Texture* tex;
    size_t w, h;
    int raw_pitch;
    void* raw_pixels;
    size_t framerate;
} cdlv_canvas;

typedef struct cdlv_text {
    SDL_Texture* tex;
    TTF_Font* font;
    SDL_Rect* glyphs;
    SDL_Color color;
    SDL_Color bg;
    char content[cdlv_max_string_size];
    size_t size;
    size_t x, y;
    size_t w, h;
    uint32_t wrap;
} cdlv_text;

typedef struct cdlv_config {
    uint32_t text_wrap;
    size_t text_x;
    size_t text_y;
    size_t text_color_r;
    size_t text_color_g;
    size_t text_color_b;
    size_t text_color_a;
} cdlv_config;

typedef struct cdlv_base {
    cdlv_config* config;
    cdlv_canvas* canvas;
    cdlv_text* text;
    cdlv_choice* choice;
    cdlv_scene** scenes;
    cdlv_state state;
    uint64_t c_tick, l_tick;
    float e_ticks;
    size_t scene_count;
    size_t c_line, c_scene, c_image;
    float accum;
    bool can_interact;
} cdlv_base;

typedef struct sdl_base {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_GameController** gamepads;
    SDL_Event event;
    int run;
    size_t w, h;
    const char* title;
} sdl_base;

#endif
