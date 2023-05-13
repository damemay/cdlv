#ifndef CDLV_H
#define CDLV_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include "cdlv_macros.h"

typedef enum {
    cdlv_default_scene,
    cdlv_anim_scene,
    cdlv_anim_once_scene,
    cdlv_script,
    cdlv_none,
} cdlv_type;

typedef enum {
    cdlv_main_menu,
    cdlv_main_run,
} cdlv_state;

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
    SDL_Color color;
    SDL_Color bg;
    size_t size;
    size_t x, y;
    size_t w, h;
    uint32_t wrap;
} cdlv_text;

typedef struct cdlv_menu {
    char* path;
    char** files;
    char* text;
    size_t file_count;
    size_t current_choice;
    bool path_exists;
} cdlv_menu;

typedef struct cdlv_base {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_GameController** gamepads;
    SDL_Event event;
    bool run;
    uint64_t c_tick, l_tick;
    float e_ticks;
    cdlv_state state;
    size_t w, h;
    const char* title;

    cdlv_canvas* canvas;
    cdlv_text* text;
    cdlv_scene** scenes;
    size_t scene_count;
    size_t c_line, c_scene, c_image;
    float accum;
    bool can_interact;
} cdlv_base;

cdlv_base* cdlv_create(const char* title, const size_t w, const size_t h);

void cdlv_clean_all(cdlv_base* base);
void cdlv_loop_start(cdlv_base* base);
void cdlv_render(cdlv_base* base);
void cdlv_loop_end(cdlv_base* base);
void cdlv_handle_keys(cdlv_base* base, SDL_Event* e);

void cdlv_canvas_create(cdlv_base* base, const size_t w, const size_t h, const size_t fps);

void cdlv_read_file(cdlv_base* base, const char* file);
void cdlv_scene_info(cdlv_base* base, const size_t index);
void cdlv_start(cdlv_base* base);

void cdlv_text_create(cdlv_base* base, const char* path, const size_t size, const uint32_t wrap, const size_t x, const size_t y, const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a);
void cdlv_text_render(cdlv_base* base);
void cdlv_text_update(cdlv_base* base, const char* content);

cdlv_menu* cdlv_menu_create(cdlv_base* base, const char* path);
void cdlv_menu_render(cdlv_base* base);
void cdlv_menu_handle_keys(cdlv_base** base, cdlv_menu** menu, SDL_Event* e);
void cdlv_menu_clean(cdlv_menu* menu);

#endif
