#include "cdlv.h"

static inline void text_font_create(cdlv_text* text, const char* path, SDL_Renderer* renderer) {
    text->font = TTF_OpenFont(path, text->size);
    if(!text->font)
        cdlv_diev("Could not create font from file at path: "
        "\"%s\": %s", path, SDL_GetError());

    SDL_Surface* s = NULL;
    s = SDL_CreateRGBSurface(0, cdlv_font_atlas_size, cdlv_font_atlas_size,
            32, 0, 0, 0, 255);
    if(!s) cdlv_diev("Could not create surface for font atlas: %s", SDL_GetError());
    SDL_SetColorKey(s, SDL_TRUE, SDL_MapRGBA(s->format, 0, 0, 0, 0));
    // SDL_SetSurfaceBlendMode(s, SDL_BLENDMODE_BLEND);

    cdlv_alloc_ptr_arr(&text->glyphs, cdlv_ascii_count, SDL_Rect);

    SDL_Rect dest = {0, 0, 0, 0};
    for(uint32_t i=' '; i<='~'; ++i) {
        SDL_Surface* g = NULL;
        g = TTF_RenderGlyph32_Blended(text->font, i, text->color);
        if(!g) cdlv_diev("Could not render glyph: %s", TTF_GetError());

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
        const uint8_t a) {
    #define cdlv_temp_text base->text
    cdlv_temp_text = malloc(sizeof(cdlv_text));
    if(!cdlv_temp_text)
        cdlv_die("Could not allocate memory for cdlv_text!");
    cdlv_temp_text->font = NULL;
    cdlv_temp_text->glyphs = NULL;
    cdlv_temp_text->tex = NULL;
    cdlv_temp_text->color.r = r;
    cdlv_temp_text->color.g = g;
    cdlv_temp_text->color.b = b;
    cdlv_temp_text->color.a = a;
    cdlv_temp_text->bg.r = 0;
    cdlv_temp_text->bg.g = 0;
    cdlv_temp_text->bg.b = 0;
    cdlv_temp_text->bg.a = 128;
    cdlv_temp_text->size = size;
    cdlv_temp_text->x = x;
    cdlv_temp_text->y = y;
    cdlv_temp_text->w = 0;
    cdlv_temp_text->h = 0;
    cdlv_temp_text->wrap = wrap;
    text_font_create(cdlv_temp_text, path, base->renderer);
    #undef cdlv_temp_text
}

void cdlv_text_update(cdlv_base* base, const char* content) {
    #define cdlv_temp_text base->text
    size_t len = strlen(content);
    if(len) {
        if(len>cdlv_max_string_size)
            strncpy(cdlv_temp_text->content, content, cdlv_max_string_size);
        else {
            memset(cdlv_temp_text->content, 0, cdlv_max_string_size);
            strcpy(cdlv_temp_text->content, content);
        }
    }
    #undef cdlv_temp_text
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

void cdlv_text_render(cdlv_base* base) {
    #define cdlv_temp_text base->text
    text_draw_wrap(cdlv_temp_text, base->renderer);
    #undef cdlv_temp_text
}
