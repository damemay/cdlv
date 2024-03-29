#include "cdlv.h"
#include "cdlv_util.h"

static inline int text_font_create(cdlv_text* text, bool bg, const char* path, SDL_Renderer* renderer) {
    text->font = TTF_OpenFont(path, text->font_size);
    if(!text->font) {
        cdlv_logv("Could not create font from file at path: "
            "\"%s\": %s", path, SDL_GetError());
        return -1;
    }

    SDL_Surface* s = NULL;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    s = SDL_CreateRGBSurface(0, cdlv_font_atlas_size, cdlv_font_atlas_size,
            32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
#else
    s = SDL_CreateRGBSurface(0, cdlv_font_atlas_size, cdlv_font_atlas_size,
            32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
#endif
    if(!s) {
        cdlv_logv("Could not create surface for font atlas: %s", SDL_GetError());
        return -1;
    }

    cdlv_alloc_ptr_arr(&text->glyphs, cdlv_ascii_count, SDL_Rect);

    SDL_Rect dest = {0, 0, 0, 0};
    for(uint32_t i=' '; i<='~'; ++i) {
        SDL_Surface* g = NULL;
        if(bg)
            g = TTF_RenderGlyph32_Shaded(text->font, i, text->color, text->bg);
        else
            g = TTF_RenderGlyph32_Blended(text->font, i, text->color);
        if(!g) {
            cdlv_logv("Could not render glyph: %s", TTF_GetError());
            return -1;
        }

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
            if(dest.y + dest.h >= cdlv_font_atlas_size) {
                cdlv_log("Out of font atlas space");
                return -1;
            }
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
    if(!text->tex) {
        cdlv_logv("Could not convert surface to texture: %s", SDL_GetError());
        return -1;
    }

    text->w = s->w;
    text->h = s->h;
    SDL_FreeSurface(s);
    return 0;
}

void cdlv_text_create(cdlv_base* base, const char* path,
        const size_t size, const uint32_t wrap,
        const size_t x, const size_t y,
        const uint8_t r, const uint8_t g, const uint8_t b,
        const uint8_t a, SDL_Renderer* renderer) {
    base->text = malloc(sizeof(cdlv_text));
    if(!base->text) {
        sprintf(base->log, "Could not allocate memory for cdlv_text!");
        base->error = cdlv_no_mem_err;
        return;
    }

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
    base->text->font_size = size;
    base->text->x = x;
    base->text->y = y;
    base->text->w = 0;
    base->text->h = 0;
    base->text->wrap = wrap;
    base->text->current_char = 0;
    memset(base->text->rendered, 0, cdlv_max_string_size);
    base->text->content_size = 0;
    base->text->accum = 0.0f;
    text_font_create(base->text, base->config->text_render_bg, path, renderer);
}

static inline void cdlv_text_type(cdlv_base* base) {
    base->text->accum += base->e_ticks * base->config->text_speed;
    if(base->text->accum > 1) {
        if(base->text->current_char < base->text->content_size) {
            base->text->accum = 0;
            base->text->rendered[base->text->current_char] = base->text->content[base->text->current_char];
            base->text->current_char++;
        }
    }
}

static inline void cdlv_text_retype(cdlv_text* text) {
    memset(text->rendered, 0, cdlv_max_string_size);
    text->current_char = 0;
}

void cdlv_text_update(cdlv_base* base, const char* content) {
    base->text->content_size = strlen(content) + 1;
    if(base->text->content_size) {
        if(base->text->content_size > cdlv_max_string_size)
            strncpy(base->text->content, content, cdlv_max_string_size-1);
        else {
            memset(base->text->content, 0, cdlv_max_string_size);
            strcpy(base->text->content, content);
        }
    }
    cdlv_text_retype(base->text);
    if(!base->config->text_speed) {
        strcpy(base->text->rendered, base->text->content);
        base->text->current_char = base->text->content_size;
    }
}

static inline void text_draw_line(cdlv_text* text, size_t x, size_t y, const char* line, SDL_Renderer* renderer) {
    size_t length = strlen(line);
    SDL_Rect dest = {0, 0, 0, 0};
    for(size_t i=0; i<length; i++) {
        unsigned char c = line[i];
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
    size_t length = strlen(text->rendered);
    for(size_t i=0; i<length; i++) {
        char c = text->content[i];
        word_w += text->glyphs[c].w;
        if(c != ' ' && (c != '\n' || c != '\r')) word[n++] = c;
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
    cdlv_text_type(base);
    text_draw_wrap(base->text, r);
}
