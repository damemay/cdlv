#include "cdlv.h"

static inline void text_font_create(cdlv_text* text, const char* path, SDL_Renderer* renderer) {
    text->font = TTF_OpenFont(path, text->size);
    if(!text->font)
        cdlv_diev("Could not create font from file at path: "
        "\"%s\": %s", path, SDL_GetError());

    SDL_Surface* s = NULL;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    s = SDL_CreateRGBSurface(0, cdlv_font_atlas_size, cdlv_font_atlas_size,
            32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
#else
    s = SDL_CreateRGBSurface(0, cdlv_font_atlas_size, cdlv_font_atlas_size,
            32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
#endif
    if(!s) cdlv_diev("Could not create surface for font atlas: %s", SDL_GetError());
    //SDL_SetSurfaceBlendMode(s, SDL_BLENDMODE_BLEND);

    cdlv_alloc_ptr_arr(&text->glyphs, cdlv_ascii_count, SDL_Rect);

    SDL_Rect dest = {0, 0, 0, 0};
    for(uint32_t i=' '; i<='~'; ++i) {
        SDL_Surface* g = NULL;
        g = TTF_RenderGlyph32_Shaded(text->font, i, text->color, text->bg);
        if(!g) cdlv_diev("Could not render glyph: %s", TTF_GetError());
        SDL_SetSurfaceBlendMode(g, SDL_BLENDMODE_NONE);

        int minx = 0;
        int maxx = 0;
        int miny = 0;
        int maxy = 0;
        TTF_GlyphMetrics32(text->font, i, &minx, &maxx, &miny, &maxy, NULL);
        dest.w = maxx - minx;
        dest.h = maxy - miny;

        if(dest.x + dest.w >= cdlv_font_atlas_size) {
            dest.x = 0;
            dest.y = dest.h + TTF_FontLineSkip(text->font);
            if(dest.y + dest.h >= cdlv_font_atlas_size)
                cdlv_die("Out of font atlas space");
        }

        SDL_BlitSurface(g, NULL, s, &dest);
        text->glyphs[i].x = dest.x;
        text->glyphs[i].y = dest.y;
        text->glyphs[i].w = dest.w;
        text->glyphs[i].h = dest.h;
        SDL_FreeSurface(g);
        dest.x += dest.w;
    }
    text->tex = SDL_CreateTextureFromSurface(renderer, s);
    if(!text->tex) cdlv_diev("Could not convert surface to texture: %s", SDL_GetError());
    text->w = s->w;
    text->h = s->h;
    SDL_FreeSurface(s);
}

void cdlv_text_create(cdlv_base* base, const char* path,
        const size_t size, const uint32_t wrap,
        const size_t x, const size_t y,
        const uint8_t r, const uint8_t g, const uint8_t b,
        const uint8_t a, SDL_Renderer* renderer) {
    base->text = malloc(sizeof(cdlv_text));
    if(!base->text)
        cdlv_die("Could not allocate memory for cdlv_text!");
    base->text->font = NULL;
    base->text->glyphs = NULL;
    base->text->tex = NULL;
    base->text->color.r = r;
    base->text->color.g = g;
    base->text->color.b = b;
    base->text->color.a = a;
    base->text->bg.r = 0;
    base->text->bg.g = 0;
    base->text->bg.b = 0;
    base->text->bg.a = 128;
    base->text->size = size;
    base->text->x = x;
    base->text->y = y;
    base->text->w = 0;
    base->text->h = 0;
    base->text->wrap = wrap;
    text_font_create(base->text, path, renderer);
}

void cdlv_text_update(cdlv_base* base, const char* content) {
    size_t len = strlen(content);
    if(len) {
        if(len>cdlv_max_string_size)
            strncpy(base->text->content, content, cdlv_max_string_size);
        else {
            memset(base->text->content, 0, cdlv_max_string_size);
            strcpy(base->text->content, content);
        }
    }
}

static inline void text_draw_line(cdlv_text* text, size_t x, size_t y, const char* line, SDL_Renderer* renderer) {
    size_t length = strlen(line);
    SDL_Rect dest = {0, 0, 0, 0};
    for(size_t i=0; i<length; i++) {
        char c = line[i];
        dest.x = x;
        dest.y = y;
        dest.w = text->glyphs[c].w;
        dest.h = text->glyphs[c].h;
        SDL_RenderCopy(renderer, text->tex, &text->glyphs[c], &dest);
        x += text->glyphs[c].w;
    }
}

static inline void text_draw_wrap(cdlv_text* text, SDL_Renderer* renderer) {
    char word[cdlv_ascii_count];
    memset(word, 0, cdlv_ascii_count);
    char line[text->wrap];
    memset(line, 0, text->wrap);
    size_t line_w = 0;
    size_t word_w = 0;
    size_t n = 0;
    size_t x = text->x;
    size_t y = text->y;
    size_t length = strlen(text->content);
    for(size_t i=0; i<length; i++) {
        char c = text->content[i];
        word_w += text->glyphs[c].w;
        if(c != ' ' && c != '\n') word[n++] = c;
        if(c == ' ' || i == length-1) {
            if(line_w + word_w >= text->wrap) {
                text_draw_line(text, x, y, line, renderer);
                memset(line, 0, text->wrap);
                y += TTF_FontLineSkip(text->font);
                line_w = 0;
            } else if(line_w != 0) strcat(line, " ");
            strcat(line, word);
            line_w += word_w;
            memset(word, 0, cdlv_ascii_count);
            word_w = 0;
            n = 0;
        } else if(c == '\n' || c == '\r') {
            strcat(line, word);
            line_w += word_w;
            text_draw_line(text, x, y, line, renderer);
            memset(line, 0, text->wrap);
            y += TTF_FontLineSkip(text->font);
            line_w = 0;
            memset(word, 0, cdlv_ascii_count);
            word_w = 0;
            n = 0;
        }
    }
    text_draw_line(text, x, y, line, renderer);
}

void cdlv_text_render(cdlv_base* base, SDL_Renderer* r) {
    text_draw_wrap(base->text, r);
}
