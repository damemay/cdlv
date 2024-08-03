#ifndef CDLV_TEXT_H
#define CDLV_TEXT_H

#include "common.h"
#include "cdlv.h"
#include "util.h"

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

cdlv_error cdlv_text_create(cdlv* base, const char* path,
        const size_t size, const uint32_t wrap,
        const cdlv_vec2 pos, const cdlv_color color, SDL_Renderer* renderer);
void cdlv_text_update(cdlv* base, const char* content);
void cdlv_text_render(cdlv* base, SDL_Renderer* renderer);
void cdlv_text_free(cdlv_text* text);

#endif