#include "cdlv.h"
#include <SDL_error.h>
#include <SDL_render.h>
#include <SDL_surface.h>
#include <SDL_ttf.h>

void cdlv_canvas_create(cdlv_base* base, const size_t w, const size_t h, const size_t fps) {
    #define canvas base->canvas
    canvas = malloc(sizeof(cdlv_canvas));
    if(!canvas)
        die("Could not allocate memory for cdlv_canvas!");
    canvas->tex = NULL;
    canvas->tex = SDL_CreateTexture(base->renderer,
            SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, w, h);
    if(!canvas->tex)
        diev("Could not create streaming texture: %s", SDL_GetError());
    canvas->w = w;
    canvas->h = h;
    canvas->raw_pitch = 0;
    canvas->raw_pixels = NULL;
    canvas->framerate = fps;
    #undef canvas
}

void cdlv_text_create(cdlv_base* base, const char* path,
        const size_t size, const uint32_t wrap,
        const size_t x, const size_t y,
        const uint8_t r, const uint8_t g, const uint8_t b,
        const uint8_t a) {
    #define text base->text
    text = malloc(sizeof(cdlv_text));
    if(!text)
        die("Could not allocate memory for cdlv_text!");
    text->font = NULL;
    text->font = TTF_OpenFont(path, size);
    if(!text->font)
        diev("Could not create font from file at path: "
        "\"%s\": %s", path, SDL_GetError());
    text->tex = NULL;
    text->color.r = r;
    text->color.g = g;
    text->color.b = b;
    text->color.a = a;
    text->bg.r = 0;
    text->bg.g = 0;
    text->bg.b = 0;
    text->bg.a = 128;
    text->size = size;
    text->x = x;
    text->y = y;
    text->w = 0;
    text->h = 0;
    text->wrap = wrap;
    #undef text
}

static inline void cdlv_text_update(cdlv_base* base, const char* content) {
    #define text base->text
    if(strlen(content)) {
        SDL_Surface* temp = NULL;
        temp = TTF_RenderUTF8_Shaded_Wrapped(text->font, content, text->color, text->bg, text->wrap);
        if(!temp) diev("Could not render text to surface: %s", TTF_GetError());
        // Free current texture
        if(text->tex) SDL_DestroyTexture(text->tex);
        text->tex = NULL;
        text->tex = SDL_CreateTextureFromSurface(base->renderer, temp);
        if(!text->tex) diev("Could not convert surface to texture: %s", SDL_GetError());
        text->w = temp->w;
        text->h = temp->h;
        SDL_FreeSurface(temp);
    }
    #undef text
}

static inline void cdlv_text_render(cdlv_base* base) {
    #define text base->text
    SDL_Rect quad = {text->x, text->y, text->w, text->h};
    SDL_RenderCopy(base->renderer, text->tex, NULL, &quad);
    #undef text
}

static inline void cdlv_load_images(cdlv_scene* scene) {
    alloc_ptr_arr(&scene->images, scene->image_count, SDL_Surface*);
    #define imgs    scene->images
    #define paths   scene->image_paths
    for(size_t i=0; i<scene->image_count; ++i) {
        imgs[i] = malloc(sizeof(SDL_Surface*));
        if(!imgs[i])
            diev("Could not allocate memory "
            "for image with path: %s", paths[i]);

        SDL_Surface* temp = NULL;
        temp = IMG_Load(paths[i]);
        if(!temp)
            diev("Could not load image "
            "with path \"%s\": %s", paths[i], SDL_GetError());

        imgs[i] = NULL;
        imgs[i] = SDL_ConvertSurfaceFormat(temp, SDL_PIXELFORMAT_RGBA32, 0);
        if(!imgs[i])
            diev("Could not convert surfaces "
            "when handling image with path "
            "\"%s\": %s", paths[i], SDL_GetError());
        free(temp);
    }
    #undef paths
    #undef imgs
}

static inline void cdlv_scene_clean(cdlv_scene* scene) {
    for(size_t i=0; i<scene->image_count; ++i) {
        SDL_FreeSurface(scene->images[i]);
        free(scene->image_paths[i]);
    }
    free(scene->images);
    free(scene->image_paths);

    for(size_t i=0; i<scene->line_count; ++i)
        free(scene->script[i]);
    free(scene->script);

    free(scene);
}

static inline void cdlv_scene_load(cdlv_base* base, const size_t index) {
    if(index < base->scene_count) {
        base->c_scene = index;
        base->c_image = 0;
        base->c_line  = 0;
        cdlv_load_images(base->scenes[index]);
        if(base->scenes[index]->type != cdlv_anim_once_scene) {
            cdlv_text_update(base, base->scenes[index]->script[base->c_line]);
            base->can_interact = true;
        } else base->can_interact = false;
        if(base->c_scene) {
            cdlv_scene_clean(base->scenes[index-1]);
            base->scenes[index-1] = NULL;
        }
    }
}

void cdlv_start(cdlv_base* base) {
    cdlv_scene_load(base, 0);
}

static inline void cdlv_update(cdlv_base* base) {
    #define scene base->scenes[base->c_scene]
    #define line base->scenes[base->c_scene]->script[base->c_line]
    if(scene->type == cdlv_anim_once_scene) {
        cdlv_scene_load(base, base->c_scene+1);
    } else {
        if(base->c_line == scene->line_count-1)
            cdlv_scene_load(base, base->c_scene+1);
        else if(base->c_line < scene->line_count) {
            ++base->c_line;
            if(line[0] == '@') {
                size_t p = strspn(line, "@");
                size_t b = 0;
                if((b = strtoul(line+p, NULL, 0)) < scene->image_count) {
                    base->c_image = b;
                    cdlv_update(base);
                }
            } cdlv_text_update(base, line);
        }
    }
    #undef line
    #undef scene
}

static inline void cdlv_handle_keys(cdlv_base* base, SDL_Event* e) {
    #define scene base->scenes[base->c_scene]
    if(base->can_interact) {
        if(e->type == SDL_KEYUP && e->key.repeat == 0) {
            switch(e->key.keysym.sym) {
                case SDLK_RETURN: cdlv_update(base); break;
            }
        }
        if(e->type == SDL_CONTROLLERBUTTONUP) {
            switch(e->cbutton.button) {
                case SDL_CONTROLLER_BUTTON_A: cdlv_update(base); break;
            }
        }
    }
    #undef scene
}

static inline void* cdlv_scene_pixels(cdlv_scene* scene, const size_t index) {
    return scene->images[index]->pixels;
}

static inline void cdlv_canvas_lock(cdlv_canvas* canvas) {
    if(!canvas->raw_pixels)
        if(SDL_LockTexture(canvas->tex,
            NULL, &canvas->raw_pixels,
            &canvas->raw_pitch) != 0)
                diev("Could not lock texture: %s", SDL_GetError());
}

static inline void cdlv_canvas_unlock(cdlv_canvas* canvas) {
    if(canvas->raw_pixels) {
        SDL_UnlockTexture(canvas->tex);
        canvas->raw_pitch = 0;
        canvas->raw_pixels = NULL;
    }
}

static inline void cdlv_canvas_copy_pixels(cdlv_canvas* canvas, void* pixels) {
    if(canvas->raw_pixels)
        memcpy(canvas->raw_pixels, pixels, canvas->raw_pitch*canvas->h);
}

static inline void cdlv_canvas_copy_buffer(cdlv_canvas* canvas, void* buff) {
    cdlv_canvas_lock(canvas);
    cdlv_canvas_copy_pixels(canvas, buff);
    cdlv_canvas_unlock(canvas);
}

void cdlv_render(cdlv_base* base) {
    #define scene base->scenes[base->c_scene]
    #define canvas base->canvas
    #define anim()                                          \
    base->accum += base->e_ticks * canvas->framerate;       \
    if(base->accum > 1) {                                   \
        size_t n_frame = base->c_image + 1;                 \
        n_frame %= scene->image_count;                      \
        if(n_frame < scene->image_count) {                  \
            base->c_image = n_frame;                        \
            base->accum = 0;                                \
            cdlv_canvas_copy_buffer(canvas,                 \
                    scene->images[base->c_image]->pixels);  \
        }                                                   \
    }
    SDL_RenderClear(base->renderer);
    if(scene->type == cdlv_default_scene) {
        cdlv_canvas_copy_buffer(canvas,
                cdlv_scene_pixels(scene, base->c_image));
    } else if(scene->type == cdlv_anim_scene) {
        anim();
    } else if(scene->type == cdlv_anim_once_scene) {
        if(base->c_image != scene->image_count-1) {
            anim();
        } else {
            base->can_interact = true;
            cdlv_text_update(base, ">>");
        }
    }
    SDL_RenderCopy(base->renderer, canvas->tex, NULL, NULL);
    if(base->can_interact) cdlv_text_render(base);
    #undef anim
    #undef canvas
    #undef scene
}

void cdlv_loop_start(cdlv_base* base) {
    while(SDL_PollEvent(&base->event) != 0) {
        if(base->event.type == SDL_QUIT) base->run = false;
        cdlv_handle_keys(base, &base->event);
    }
    base->c_tick = SDL_GetTicks64();
    base->e_ticks = (base->c_tick - base->l_tick) / 1000.0f;
}

void cdlv_loop_end(cdlv_base* base) {
    SDL_RenderPresent(base->renderer);
    base->l_tick = base->c_tick;
}
