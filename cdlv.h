#ifndef CDLV_H
#define CDLV_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#define cdlv_max_string_size    UINT16_MAX
#define cdlv_small_string       UINT8_MAX
#define cdlv_max_choice_count   UINT8_MAX
#define cdlv_max_menu_entries   UINT8_MAX

#define cdlv_ascii_count        128
#define cdlv_font_atlas_size    1024

#define cdlv_arrow              "> "
#define cdlv_continue           ". . ."

typedef enum {
    cdlv_no_err,
    cdlv_config_err,
    cdlv_no_mem_err,
    cdlv_file_err,
    cdlv_script_err,
    cdlv_sdl_err,
} cdlv_error;

typedef struct cdlv_scene {
    SDL_Surface** images;
    char** image_paths;
    uint16_t image_count;
    char** script;
    uint16_t line_count;
} cdlv_scene;

typedef struct cdlv_canvas {
    SDL_Texture* tex;
    uint16_t w, h;
    int raw_pitch;
    void* raw_pixels;
    uint8_t framerate;

    uint16_t iter;
    bool changing;
} cdlv_canvas;

typedef struct cdlv_text {
    SDL_Texture* tex;
    TTF_Font* font;
    SDL_Rect* glyphs;
    SDL_Color color;
    SDL_Color bg;
    char content[cdlv_max_string_size];
    uint16_t content_size;
    uint8_t font_size;
    uint16_t x, y;
    uint16_t w, h;
    uint32_t wrap;
    char rendered[cdlv_max_string_size];
    uint16_t current_char;
    float accum;
} cdlv_text;

typedef struct cdlv_color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} cdlv_color;

typedef struct cdlv_vec2 {
    uint16_t x;
    uint16_t y;
} cdlv_vec2;

typedef struct cdlv_config {
    char text_font[cdlv_small_string];
    uint8_t text_size;
    cdlv_vec2 text_xy;
    uint16_t text_wrap;
    cdlv_color text_color;
    int text_render_bg;
    uint8_t text_speed;
    uint8_t dissolve_speed;
} cdlv_config;

typedef struct cdlv_base {
    cdlv_config* config;
    cdlv_canvas* canvas;
    cdlv_text* text;
    cdlv_scene** scenes;
    uint64_t c_tick, l_tick;
    float e_ticks;
    uint8_t scene_count;
    uint8_t c_line, c_scene, p_scene, c_image;
    float accum;
    bool can_interact;
    bool finished;
    cdlv_error error;
    char log[cdlv_max_string_size];
} cdlv_base;

#define cdlv_logv(msg, ...) fprintf(stderr, "cdlv: " msg "\n", __VA_ARGS__)
#define cdlv_log(msg) fprintf(stderr, "cdlv: " msg "\n")

#define cdlv_strdup(dest, src, size)            \
{                                               \
    *dest = calloc(size, sizeof(char));         \
    if(!dest)                                   \
        cdlv_log("Could not "                   \
        "allocate destination for "             \
        "string duplication!");                 \
    strcpy(*dest, src);                         \
}


#endif
