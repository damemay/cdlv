#include "cdlv.h"
#include "cdlv_util.h"

static inline void* cdlv_scene_pixels(cdlv_scene* scene, const size_t index) {
    return scene->images[index]->pixels;
}

static inline void cdlv_canvas_lock(cdlv_canvas* canvas) {
    if(!canvas->raw_pixels)
        if(SDL_LockTexture(canvas->tex,
            NULL, &canvas->raw_pixels,
            &canvas->raw_pitch) != 0)
                cdlv_diev("Could not lock texture: %s", SDL_GetError());
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

static inline uint8_t lerp(uint8_t a, uint8_t b, size_t t) {
    int x = a*(256-t) + b*t;
    uint8_t res = x >> 8;
    return res;
}

static inline uint32_t* get_pixel_row(void* pixels, int pitch, size_t y) {
    return (uint32_t*)((uint8_t*)pixels + pitch * y);
}

static inline void dissolve_iter(cdlv_base* base) {
    base->accum += base->e_ticks * 60.0f;
    if(base->accum > 1) {
        if(base->canvas->iter<256)
            base->canvas->iter+=base->config->dissolve_speed;
        else base->canvas->changing=false;
    }
}

static inline void dissolve_images(cdlv_base* base) {
    if(base->canvas->raw_pixels) {
        for(size_t y=0; y < base->canvas->h; y++) {
            uint32_t* can = get_pixel_row(
                    base->canvas->raw_pixels, 
                    base->canvas->raw_pitch, y);
            uint32_t* cur = get_pixel_row(
                    base->scenes[base->c_scene]->images[base->c_image]->pixels, 
                    base->scenes[base->c_scene]->images[base->c_image]->pitch, y);
            uint32_t* prv = get_pixel_row(
                    base->scenes[base->c_scene]->images[base->c_image-1]->pixels, 
                    base->scenes[base->c_scene]->images[base->c_image-1]->pitch, y);
            for(size_t x=0; x < base->canvas->w; x++) {
                uint8_t r, g, b, a,
                        rc, gc, bc, ac;
                SDL_GetRGBA(*prv,
                        base->scenes[base->c_scene]->images[base->c_image]->format,
                        &r,&g,&b,&a);
                SDL_GetRGBA(*cur,
                        base->scenes[base->c_scene]->images[base->c_image]->format,
                        &rc,&gc,&bc,&ac);
                *can = SDL_MapRGBA(
                        base->scenes[base->c_scene]->images[base->c_image]->format,
                        lerp(r, rc, base->canvas->iter),
                        lerp(g, gc, base->canvas->iter),
                        lerp(b, bc, base->canvas->iter),
                        lerp(a, ac, base->canvas->iter));
                can++;
                cur++;
                prv++;
            }
        } dissolve_iter(base);
    }
}

static inline void dissolve_and_free_prev_scene(cdlv_base* base) {
    if(base->canvas->raw_pixels) {
        for(size_t y=0; y < base->canvas->h; y++) {
            uint32_t* can = get_pixel_row(
                    base->canvas->raw_pixels, 
                    base->canvas->raw_pitch, y);
            uint32_t* cur = get_pixel_row(
                    base->scenes[base->c_scene]->images[base->c_image]->pixels, 
                    base->scenes[base->c_scene]->images[base->c_image]->pitch, y);
            uint32_t* prv = get_pixel_row(
                    base->scenes[base->p_scene]->images[base->scenes[base->p_scene]->image_count-1]->pixels, 
                    base->scenes[base->p_scene]->images[base->scenes[base->p_scene]->image_count-1]->pitch, y);
            for(size_t x=0; x < base->canvas->w; x++) {
                uint8_t r, g, b, a,
                        rc, gc, bc, ac;
                SDL_GetRGBA(*prv,
                        base->scenes[base->c_scene]->images[base->c_image]->format,
                        &r,&g,&b,&a);
                SDL_GetRGBA(*cur,
                        base->scenes[base->c_scene]->images[base->c_image]->format,
                        &rc,&gc,&bc,&ac);
                *can = SDL_MapRGBA(
                        base->scenes[base->c_scene]->images[base->c_image]->format,
                        lerp(r, rc, base->canvas->iter),
                        lerp(g, gc, base->canvas->iter),
                        lerp(b, bc, base->canvas->iter),
                        lerp(a, ac, base->canvas->iter));
                can++;
                cur++;
                prv++;
            }
        } 
        base->accum += base->e_ticks * 60.0f;
        if(base->accum > 1) {
            if(base->canvas->iter<256)
                base->canvas->iter+=base->config->dissolve_speed;
            else {
                base->canvas->changing=false;
                SDL_FreeSurface(base->scenes[base->p_scene]->images[base->scenes[base->p_scene]->image_count-1]);
                free(base->scenes[base->p_scene]->images);
                base->scenes[base->p_scene]->images = NULL;
            }
        }

    }
}

static inline void cdlv_anim(cdlv_base* base, cdlv_scene* scene) {
    base->accum += base->e_ticks * base->canvas->framerate;
    if(base->accum > 1) {
        size_t n_frame = base->c_image + 1;
        n_frame %= scene->image_count;
        if(n_frame < scene->image_count) {
            base->c_image = n_frame;
            base->accum = 0;
            cdlv_canvas_copy_buffer(base->canvas, scene->images[base->c_image]->pixels);
        }
    }
}

void cdlv_render(cdlv_base* base, SDL_Renderer** r) {
    cdlv_scene* cdlv_temp_scene = base->scenes[base->c_scene];
    SDL_RenderClear(*r);
    if(cdlv_temp_scene->type == cdlv_static_scene) {
        if(base->canvas->changing) {
            cdlv_canvas_lock(base->canvas);
            if(base->c_image > 0) {
                dissolve_images(base);
            } else {
                if(base->c_scene > 0) dissolve_and_free_prev_scene(base);
                else base->canvas->changing = false;
            }
            cdlv_canvas_unlock(base->canvas);
        } else cdlv_canvas_copy_buffer(base->canvas,
              cdlv_scene_pixels(cdlv_temp_scene, base->c_image));
    } else if(cdlv_temp_scene->type == cdlv_anim_scene) {
        cdlv_anim(base, cdlv_temp_scene);
    } else if(cdlv_temp_scene->type == cdlv_anim_text_scene) {
        if(base->c_image != cdlv_temp_scene->image_count-1) cdlv_anim(base, cdlv_temp_scene);
    } else if(cdlv_temp_scene->type == cdlv_anim_once_scene || cdlv_temp_scene->type == cdlv_anim_wait_scene) {
        if(base->c_image != cdlv_temp_scene->image_count-1) {
            cdlv_anim(base, cdlv_temp_scene);
        } else {
            base->can_interact = true;
            if(cdlv_temp_scene->type == cdlv_anim_once_scene)
                cdlv_update(base);
            else if(cdlv_temp_scene->type == cdlv_anim_wait_scene)
                cdlv_text_update(base, cdlv_continue);
        }
    }
    SDL_RenderCopy(*r, base->canvas->tex, NULL, NULL);
    if(base->can_interact) cdlv_text_render(base, *r);
    #undef cdlv_anim
}
