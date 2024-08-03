#include "text.h"
#include "util.h"

static inline cdlv_error text_font_create(cdlv* base, cdlv_text* text, bool bg, const char* path, SDL_Renderer* renderer) {
    text->font = TTF_OpenFont(path, text->font_size);
    if(!text->font) {
        cdlv_logv("Could not create font from file at path: "
            "\"%s\": %s", path, SDL_GetError());
        cdlv_err(cdlv_file_error);
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
        cdlv_err(cdlv_fatal_error);
    }

    text->glyphs = calloc(cdlv_ascii_count, sizeof(SDL_Rect));
    if(!text->glyphs) {
        cdlv_log("Could not allocate memory for text glyphs");
        cdlv_err(cdlv_memory_error);
    }

    SDL_Rect dest = {0, 0, 0, 0};
    for(uint32_t i=' '; i<='~'; ++i) {
        SDL_Surface* g = NULL;
        if(bg)
            g = TTF_RenderGlyph32_Shaded(text->font, i, text->color, text->bg);
        else
            g = TTF_RenderGlyph32_Blended(text->font, i, text->color);
        if(!g) {
            cdlv_logv("Could not render glyph: %s", TTF_GetError());
            cdlv_err(cdlv_fatal_error);
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
                cdlv_err(cdlv_fatal_error);
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
        cdlv_err(cdlv_fatal_error);
    }

    text->w = s->w;
    text->h = s->h;
    SDL_FreeSurface(s);
    cdlv_err(cdlv_ok);
}

cdlv_error cdlv_text_create(cdlv* base, const char* path,
        const size_t size, const uint32_t wrap,
        const cdlv_vec2 pos, const cdlv_color color, SDL_Renderer* renderer) {
    cdlv_text* text = malloc(sizeof(cdlv_text));
    if(!text) {
        cdlv_log("Could not allocate memory for cdlv_text!");
        cdlv_err(cdlv_memory_error);
    }

    text->font = NULL;
    text->glyphs = NULL;
    text->tex = NULL;
    text->color.r = color.r;
    text->color.g = color.g;
    text->color.b = color.b;
    text->color.a = color.a;
    text->bg.r = 0;
    text->bg.g = 0;
    text->bg.b = 0;
    text->bg.a = 128;
    text->font_size = size;
    text->x = pos.x;
    text->y = pos.y;
    text->w = 0;
    text->h = 0;
    text->wrap = wrap;
    text->current_char = 0;
    memset(text->rendered, 0, cdlv_max_string_size);
    text->content_size = 0;
    text->accum = 0.0f;

    cdlv_error res;
    if((res = text_font_create(base, text, base->config.text_render_bg, path, renderer)) != cdlv_ok) cdlv_err(res);
    base->text = text;
    cdlv_err(cdlv_ok);
}

static inline void text_type(cdlv* base) {
    cdlv_text* text = (cdlv_text*)base->text;
    text->accum += base->elapsed_ticks * base->config.text_speed;
    if(text->accum > 1) {
        if(text->current_char < text->content_size) {
            text->accum = 0;
            text->rendered[text->current_char] = text->content[text->current_char];
            text->current_char++;
        }
    }
}

static inline void text_retype(cdlv_text* text) {
    memset(text->rendered, 0, cdlv_max_string_size);
    text->current_char = 0;
}

void cdlv_text_update(cdlv* base, const char* content) {
    cdlv_text* text = (cdlv_text*)base->text;
    text->content_size = strlen(content) + 1;
    if(text->content_size) {
        if(text->content_size > cdlv_max_string_size)
            strncpy(text->content, content, cdlv_max_string_size-1);
        else {
            memset(text->content, 0, cdlv_max_string_size);
            strcpy(text->content, content);
        }
    }
    text_retype(text);
    if(!base->config.text_speed) {
        strcpy(text->rendered, text->content);
        text->current_char = text->content_size;
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
        word_w += text->glyphs[(int)c].w;
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

void cdlv_text_render(cdlv* base, SDL_Renderer* renderer) {
    text_type(base);
    text_draw_wrap(base->text, renderer);
}

void cdlv_text_free(cdlv_text* text) {
    if(!text) return;
    TTF_CloseFont(text->font);
    if(text->tex) SDL_DestroyTexture(text->tex);
    if(text->glyphs) free(text->glyphs);
    free(text);
}