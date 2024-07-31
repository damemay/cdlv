#ifndef CDLV_H
#define CDLV_H

#include "common.h"
#include "array.h"
#include "hashdict.c/hashdict.h"

typedef enum {
    cdlv_ok,
    cdlv_memory_error,
    cdlv_file_error,
    cdlv_read_error,
    cdlv_parse_error,
} cdlv_error;

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
    uint16_t wrap;
    char rendered[cdlv_max_string_size];
    uint16_t current_char;
    float accum;
} cdlv_text;

typedef struct cdlv_config {
    char text_font[cdlv_max_string_size];
    uint8_t text_size;
    cdlv_vec2 text_xy;
    uint16_t text_wrap;
    cdlv_color text_color;
    int text_render_bg;
    uint8_t text_speed;
    uint8_t dissolve_speed;
} cdlv_config;

typedef struct cdlv {
    bool is_playing;
    bool can_interact;

    cdlv_error error;
    char log[cdlv_max_string_size];

    uint64_t c_tick, l_tick;
    float e_ticks;
    float accum;

    uint16_t width, height;

    cdlv_config config;
    cdlv_text* text;
    cdlv_dict* scenes;
    char* resources_path;
    cdlv_dict* resources;
} cdlv;

void cdlv_init(cdlv* base, uint16_t width, uint16_t height);
void cdlv_set_config(cdlv* base, const cdlv_config config);
cdlv_error cdlv_add_script(cdlv* base, const char* path);

void cdlv_free(cdlv* base);

#endif
