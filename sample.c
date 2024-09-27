#include "cdlv.h"

#include <SDL2/SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>

#define _width 960
#define _height 544
#define title "cdlv2 sample"
#define cdlv_ascii_count 128
#define cdlv_font_atlas_size 1024

struct config {
    char text_font[cdlv_max_string_size];
    uint8_t text_size;
    cdlv_vec2 text_xy;
    uint16_t text_wrap;
    cdlv_color text_color;
    int text_render_bg;
    uint8_t text_speed;
    uint8_t dissolve_speed;
};

struct text {
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
};

struct config config = {
    .text_font = "../res/fonts/roboto.ttf",
    .text_size = 32,
    .text_xy = {.x = 50, .y = 400},
    .text_wrap = 900,
    .text_color = {.r = 0, .g = 0, .b = 0, .a = 255},
};

struct text* text;
uint64_t current_tick, last_tick;
float accum, elapsed_ticks;
SDL_Event event;
SDL_Window* window;
SDL_Renderer* renderer;

int create_text();
void text_type();
void text_retype();
void text_update(const char* content);
void text_draw_line(size_t x, size_t y, const char* line);
void text_draw_wrap();
void text_render();
void text_free();

static inline void log_cb(char* buf) {
    printf("cdlv: %s\n", buf);
}

static inline void error_cb(cdlv_error error, void* user_data) {
    if(error == cdlv_ok) return;
    switch(error) {
        case cdlv_ok: printf("CDLV OK\n"); break;
        case cdlv_memory_error:	    puts("CDLV Memory error"); break;
        case cdlv_file_error:       puts("CDLV File error"); break;
        case cdlv_read_error:       puts("CDLV Read error"); break;
        case cdlv_parse_error:      puts("CDLV Parse error"); break;
        case cdlv_video_error:      puts("CDLV Video error"); break;
        case cdlv_fatal_error:      puts("CDLV Fatal error"); break;
        case cdlv_callback_error:   puts("Callback provided by user returned with error"); break;
    }
    exit(1);
}

static inline void* image_load_cb(const char* path, void* user_data) {
    SDL_Renderer* renderer = (SDL_Renderer*)user_data;
    SDL_Texture* texture = IMG_LoadTexture(renderer, path);
    if(!texture) return NULL;
    return texture;
}

static inline void image_render_cb(void* image, void* user_data) {
    SDL_Renderer* renderer = (SDL_Renderer*)user_data;
    SDL_Texture* texture = (SDL_Texture*)image;
    SDL_RenderCopy(renderer, texture, NULL, NULL);
}

static inline void image_free_cb(void* image) {
    SDL_Texture* texture = (SDL_Texture*)image;
    SDL_DestroyTexture(texture);
}

static inline void video_free_cb(void* texture) {
    SDL_Texture* _texture = (SDL_Texture*)texture;
    SDL_DestroyTexture(_texture);
}

static inline void* video_load_cb(const uint64_t width, const uint64_t height, void* user_data) {
    SDL_Renderer* renderer = (SDL_Renderer*)user_data;
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, width, height);
    if(!texture) return NULL;
    return texture;
}

static inline void video_update_cb(cdlv_yuv_plane plane, cdlv_yuv_pitch pitch, void* texture, void* user_data) {
    SDL_Texture* _texture = (SDL_Texture*)texture;
    SDL_UpdateYUVTexture(_texture, NULL, plane.y, pitch.y, plane.u, pitch.u, plane.v, pitch.v);
}

static inline int user_update_cb(void* user_data) {
    struct text* t = (struct text*)user_data;
    if(t->current_char != t->content_size) {
        strcpy(t->rendered, t->content);
        t->current_char = t->content_size;
	return 0;
    }
    return 1;
}

static inline void line_cb(const char* line, void* user_data) {
    text_update(line);
}

int main(int argc, char* argv[]) {
    char script_path[cdlv_max_string_size];
    if(argc < 2) {
	strcpy(script_path, "../res/sample/sample.cdlv");
    } else {
	if(strstr(argv[1], ".cdlv")) strncpy(script_path, argv[1], cdlv_max_string_size-1);
	else {
	    fprintf(stderr, "Usage: %s [script]\r\n [script]\tpath to .cdlv file. defaults to ../res/sample/sample.cdlv\r\n", argv[0]);
	    exit(1);
	}
    }
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG);
    TTF_Init();
    window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, _width, _height, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_TARGETTEXTURE);
    create_text();

    // Setup base cdlv instance with configured callbacks
    char buffer[cdlv_max_string_size];
    cdlv base = {
	.log_config = {
	    .callback = log_cb,
	    .buffer = buffer,
	    .buffer_size = cdlv_max_string_size,
	},
	.error_config = {
	    .callback = error_cb,
	},
	.image_config = {
	    .load_callback = image_load_cb,
	    .render_callback = image_render_cb,
	    .free_callback = image_free_cb,
	    .user_data = renderer,
	},
	.video_config = {
	    .load_callback = video_load_cb,
	    .free_callback = video_free_cb,
	    .update_callback = video_update_cb,
	    .user_data = renderer,
	},
	.user_config = {
	    .update_callback = user_update_cb,
	    .line_callback = line_cb,
	    .user_data = text,
	},
    };

    // Set script file and load it
    cdlv_set_script(&base, script_path);

    bool running = true;
    while(running) {
	current_tick = SDL_GetTicks64();
	elapsed_ticks = (current_tick - last_tick) / 1000.0f;
        while(SDL_PollEvent(&event) != 0) {
            if(event.type == SDL_QUIT) {
                running = false;
                break;
            } else if(event.type == SDL_KEYUP && event.key.repeat == 0) {
		if(event.key.keysym.sym == SDLK_RETURN) {
		    // Add user update function where you want to update
		    // and parse next line
		    cdlv_user_update(&base);
		    break;
		}
	    }
        }
        SDL_RenderClear(renderer);

	// Add cdlv render between clearing the screen and updating it
	cdlv_render(&base);

	text_render();
        SDL_RenderPresent(renderer);
	last_tick = current_tick;
    }

    // Unset script and unload resources
    cdlv_unset_script(&base);

    text_free();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

int create_text() {
    text = malloc(sizeof(struct text));
    if(!text) return 0;
    text->font = NULL;
    text->glyphs = NULL;
    text->tex = NULL;
    text->color.r = config.text_color.r;
    text->color.g = config.text_color.g;
    text->color.b = config.text_color.b;
    text->color.a = config.text_color.a;
    text->bg.r = 0;
    text->bg.g = 0;
    text->bg.b = 0;
    text->bg.a = 128;
    text->font_size = config.text_size;
    text->x = config.text_xy.x;
    text->y = config.text_xy.y;
    text->w = 0;
    text->h = 0;
    text->wrap = config.text_wrap;
    text->current_char = 0;
    memset(text->rendered, 0, cdlv_max_string_size);
    text->content_size = 0;
    text->accum = 0.0f;
    text->font = TTF_OpenFont(config.text_font, text->font_size);
    if(!text->font) return 0;
    SDL_Surface* s = NULL;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    s = SDL_CreateRGBSurface(0, cdlv_font_atlas_size, cdlv_font_atlas_size,
            32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
#else
    s = SDL_CreateRGBSurface(0, cdlv_font_atlas_size, cdlv_font_atlas_size,
            32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
#endif
    if(!s) return 0;
    text->glyphs = calloc(cdlv_ascii_count, sizeof(SDL_Rect));
    if(!text->glyphs) return 0;
    SDL_Rect dest = {0, 0, 0, 0};
    for(uint32_t i=' '; i<='~'; ++i) {
        SDL_Surface* g = NULL;
        if(config.text_render_bg)
            g = TTF_RenderGlyph32_Shaded(text->font, i, text->color, text->bg);
        else
            g = TTF_RenderGlyph32_Blended(text->font, i, text->color);
        if(!g) return 0;
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
            if(dest.y + dest.h >= cdlv_font_atlas_size) return 0;
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
    if(!text->tex) return 0;
    text->w = s->w;
    text->h = s->h;
    SDL_FreeSurface(s);
    return 1;
}

void text_type() {
    text->accum += elapsed_ticks * config.text_speed;
    if(text->accum > 1) {
        if(text->current_char < text->content_size) {
            text->accum = 0;
            text->rendered[text->current_char] = text->content[text->current_char];
            text->current_char++;
        }
    }
}

void text_retype() {
    memset(text->rendered, 0, cdlv_max_string_size);
    text->current_char = 0;
}

void text_update(const char* content) {
    text->content_size = strlen(content) + 1;
    if(text->content_size) {
        if(text->content_size > cdlv_max_string_size)
            strncpy(text->content, content, cdlv_max_string_size-1);
        else {
            memset(text->content, 0, cdlv_max_string_size);
            strcpy(text->content, content);
        }
    }
    text_retype();
    if(!config.text_speed) {
        strcpy(text->rendered, text->content);
        text->current_char = text->content_size;
    }
}

void text_draw_line(size_t x, size_t y, const char* line) {
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

void text_draw_wrap() {
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
                text_draw_line(x, y, line);
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
            text_draw_line(x, y, line);
            memset(line, 0, text->wrap);
            y += TTF_FontLineSkip(text->font);
            line_w = 0;
            memset(word, 0, cdlv_ascii_count);
            word_w = 0;
            n = 0;
        }
    }
    text_draw_line(x, y, line);
}

void text_render() {
    text_type();
    text_draw_wrap();
}

void text_free() {
    if(!text) return;
    TTF_CloseFont(text->font);
    if(text->tex) SDL_DestroyTexture(text->tex);
    if(text->glyphs) free(text->glyphs);
    free(text);
}
