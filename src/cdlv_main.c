#include "cdlv.h"

void cdlv_canvas_create(cdlv_base* base, const size_t w, const size_t h, const size_t fps, SDL_Renderer** r) {
    #define cdlv_temp_canvas base->canvas
    cdlv_temp_canvas = malloc(sizeof(cdlv_canvas));
    if(!cdlv_temp_canvas)
        cdlv_die("Could not allocate memory for cdlv_canvas!");
    cdlv_temp_canvas->tex = NULL;
    cdlv_temp_canvas->tex = SDL_CreateTexture(*r,
            SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, w, h);
    if(!cdlv_temp_canvas->tex)
        cdlv_diev("Could not create streaming texture: %s", SDL_GetError());
    cdlv_temp_canvas->w = w;
    cdlv_temp_canvas->h = h;
    cdlv_temp_canvas->raw_pitch = 0;
    cdlv_temp_canvas->raw_pixels = NULL;
    cdlv_temp_canvas->framerate = fps;
    #undef cdlv_temp_canvas
}

static inline void cdlv_load_images(cdlv_canvas* canvas, cdlv_scene* scene) {
    cdlv_alloc_ptr_arr(&scene->images, scene->image_count, SDL_Surface*);
    #define cdlv_imgs    scene->images
    #define cdlv_paths   scene->image_paths
    for(size_t i=0; i<scene->image_count; ++i) {
        cdlv_imgs[i] = malloc(sizeof(SDL_Surface*));
        if(!cdlv_imgs[i])
            cdlv_diev("Could not allocate memory "
            "for image with path: %s", cdlv_paths[i]);

        SDL_Surface* temp = NULL;
        temp = IMG_Load(cdlv_paths[i]);
        if(!temp)
            cdlv_diev("Could not load image "
            "with path \"%s\": %s", cdlv_paths[i], SDL_GetError());

        if(temp->w < canvas->w && temp->h < canvas->h) {
            SDL_Surface* scaled = NULL;
            scaled = SDL_CreateRGBSurfaceWithFormat(temp->flags, canvas->w, canvas->h,
                    temp->format->BitsPerPixel, temp->format->format);
            if(!scaled)
                cdlv_diev("Could not scale surface: %s", SDL_GetError());
            if(SDL_BlitScaled(temp, NULL, scaled, NULL) < 0)
                cdlv_diev("Could not blit scaled surface: %s", SDL_GetError());
            SDL_FreeSurface(temp);
            temp = scaled;
        }

        cdlv_imgs[i] = NULL;
        cdlv_imgs[i] = SDL_ConvertSurfaceFormat(temp, SDL_PIXELFORMAT_RGBA32, 0);
        if(!cdlv_imgs[i])
            cdlv_diev("Could not convert surfaces "
            "when handling image with path "
            "\"%s\": %s", cdlv_paths[i], SDL_GetError());
        if(SDL_SetSurfaceBlendMode(cdlv_imgs[i], SDL_BLENDMODE_BLEND) < 0)
            cdlv_diev("Could not set alpha blending on image with path \"%s\": %s",
                cdlv_paths[i], SDL_GetError());
        free(temp);
    }
    #undef cdlv_temp_paths
    #undef cdlv_temp_imgs
}

static inline void cdlv_scene_clean(cdlv_scene* scene) {
    for(size_t i=0; i<scene->image_count; ++i)
        SDL_FreeSurface(scene->images[i]);
    free(scene->images);
    scene->images = NULL;
}

static inline void cdlv_scene_load(cdlv_base* base, const size_t prev, const size_t index) {
    if(index < base->scene_count) {
        base->c_scene = index;
        base->c_image = 0;
        base->c_line  = 0;
        cdlv_load_images(base->canvas, base->scenes[index]);
        if(base->scenes[index]->type != cdlv_anim_once_scene && base->scenes[index]->type != cdlv_anim_wait_scene) {
            cdlv_text_update(base, base->scenes[index]->script[base->c_line]);
            base->can_interact = true;
        } else base->can_interact = false;
        if(base->c_scene) {
            cdlv_scene_clean(base->scenes[prev]);
            // base->scenes[prev] = NULL;
        }
    }
}

static inline bool cdlv_change_image(cdlv_base* base, cdlv_scene* scene, const char* line) {
    size_t i = 0;
    if((sscanf(line, cdlv_tag_func_image " %lu", &i)) != 0)
        if(i < scene->image_count) {
            base->c_image = i;
            return true;
        }
    return false;
}

static inline void cdlv_choice_create(cdlv_base* base) {
    #define cdlv_temp_choice base->choice
    cdlv_temp_choice = malloc(sizeof(cdlv_choice));
    if(!cdlv_temp_choice)
        cdlv_die("Could not allocate memory for a new scripted choice!");

    cdlv_temp_choice->count               = 0;
    cdlv_temp_choice->current             = 0;
    cdlv_temp_choice->state               = cdlv_parsing;
    cdlv_temp_choice->destinations        = NULL;
    cdlv_temp_choice->options             = NULL;
    cdlv_temp_choice->prompt              = NULL;
    cdlv_alloc_ptr_arr(&cdlv_temp_choice->destinations, cdlv_max_choice_count, size_t);
    cdlv_alloc_ptr_arr(&cdlv_temp_choice->options, cdlv_max_choice_count, char*);
    cdlv_alloc_ptr_arr(&cdlv_temp_choice->prompt, cdlv_max_string_size, char);
    #undef cdlv_temp_choice
}

static inline void cdlv_choice_clean(cdlv_base* base) {
    #define cdlv_temp_choice base->choice
    for(size_t i=0;i<cdlv_max_choice_count; ++i)
        if(cdlv_temp_choice->options[i]) free(cdlv_temp_choice->options[i]);
    free(cdlv_temp_choice->options);
    free(cdlv_temp_choice->prompt);
    free(cdlv_temp_choice->destinations);
    free(cdlv_temp_choice);
    cdlv_temp_choice = NULL;
    #undef cdlv_temp_choice
}

static inline void cdlv_choice_add(cdlv_base* base, const char* line) {
    #define cdlv_temp_choice base->choice
    const char chars[12] = "1234567890 ";
    size_t d = 0;
    size_t s = 0;
    if((sscanf(line, "%lu", &d)) != 0)
        if(d < base->scene_count)
            s = strspn(line, chars);
    if(s>0 && cdlv_temp_choice->count < cdlv_max_choice_count) {
        cdlv_temp_choice->destinations[cdlv_temp_choice->count] = d;
        cdlv_duplicate_string(&cdlv_temp_choice->options[cdlv_temp_choice->count], line+s, strlen(line+s)+1);
        ++cdlv_temp_choice->count;
    }
    #undef cdlv_temp_choice
}

static inline void choice_update_prompt(cdlv_base* base) {
    #define cdlv_temp_choice base->choice
    if(cdlv_temp_choice->current < cdlv_temp_choice->count) {
        sprintf(cdlv_temp_choice->prompt, cdlv_arrow);
        for(size_t i=cdlv_temp_choice->current; i<cdlv_temp_choice->count; ++i) {
            strcat(cdlv_temp_choice->prompt, cdlv_temp_choice->options[i]);
            strcat(cdlv_temp_choice->prompt, "\n");
        }
        //strcat(choice->prompt, " ");
    }
    cdlv_text_update(base, cdlv_temp_choice->prompt);
    #undef cdlv_temp_choice
}

static inline void cdlv_goto(cdlv_base* base, const char* line) {
    size_t i = 0;
    if((sscanf(line, cdlv_tag_func_goto " %lu", &i)) != 0)
        if(i < base->scene_count)
            cdlv_scene_load(base, base->c_scene, i);
}


static inline void cdlv_update(cdlv_base* base) {
    #define cdlv_temp_scene base->scenes[base->c_scene]
    #define cdlv_temp_line base->scenes[base->c_scene]->script[base->c_line]
    if(cdlv_temp_scene->type == cdlv_anim_once_scene
        || cdlv_temp_scene->type == cdlv_anim_wait_scene) {
        cdlv_scene_load(base, base->c_scene, base->c_scene+1);
    } else {
        if(base->c_line == cdlv_temp_scene->line_count-1) {
            if(cdlv_temp_scene->type == cdlv_anim_text_scene && base->c_image == cdlv_temp_scene->image_count-1)
                cdlv_scene_load(base, base->c_scene, base->c_scene+1);
            else if(cdlv_temp_scene->type != cdlv_anim_text_scene)
                cdlv_scene_load(base, base->c_scene, base->c_scene+1);
        } else if(base->c_line < cdlv_temp_scene->line_count) {
            // update current line
            ++base->c_line;
            if(cdlv_temp_line[0] == '@') {
                // @ is now a function determinator, we need to check what it actually does
                if(strstr(cdlv_temp_line, cdlv_tag_func_image)) {
                    if(cdlv_change_image(base, cdlv_temp_scene, cdlv_temp_line))
                        cdlv_update(base);
                } else if(strstr(cdlv_temp_line, cdlv_tag_func_choice)) {
                    cdlv_choice_create(base);
                    cdlv_update(base);
                } else if(strstr(cdlv_temp_line, cdlv_tag_func_end)) {
                    base->choice->state = cdlv_parsed;
                    choice_update_prompt(base);
                } else if(strstr(cdlv_temp_line, cdlv_tag_func_goto)) {
                    cdlv_goto(base, cdlv_temp_line);
                }
            } else if(base->choice) {
                if(base->choice->state == cdlv_parsing) {
                    cdlv_choice_add(base, cdlv_temp_line);
                    cdlv_update(base);
                }
            } else cdlv_text_update(base, cdlv_temp_line);
        }
    }
    #undef cdlv_temp_line
    #undef cdlv_temp_scene
}

static inline bool cdlv_choice_switch(cdlv_base* base, size_t ch) {
    #define cdlv_temp_choice base->choice
    if(cdlv_temp_choice->destinations[ch]) {
        cdlv_scene_load(base, base->c_scene, cdlv_temp_choice->destinations[ch]);
        return true;
    }
    return false;
    #undef cdlv_temp_choice
}

static inline void cdlv_choice_handler(cdlv_base* base, size_t ch) {
    if(cdlv_choice_switch(base, ch)) cdlv_choice_clean(base);
}

void cdlv_handle_keys(cdlv_base* base, SDL_Event* e) {
    if(base->can_interact && !base->choice) {
        if(e->type == SDL_KEYUP && e->key.repeat == 0) switch(e->key.keysym.sym) {
                case SDLK_RETURN: cdlv_update(base); break;
        }
        if(e->type == SDL_CONTROLLERBUTTONUP) switch(e->cbutton.button) {
                case SDL_CONTROLLER_BUTTON_A: cdlv_update(base); break;
        }
    } else if(base->can_interact && base->choice) {
        if(e->type == SDL_KEYUP && e->key.repeat == 0) switch(e->key.keysym.sym) {
                case SDLK_UP:
                    if(base->choice->current > 0) {
                        --base->choice->current;
                        choice_update_prompt(base);
                    } break;
                case SDLK_DOWN:
                    if(base->choice->current < base->choice->count) {
                        ++base->choice->current;
                        choice_update_prompt(base);
                    } break;
                case SDLK_RETURN:
                    cdlv_choice_handler(base, base->choice->current);
                    break;
        }
        if(e->type == SDL_CONTROLLERBUTTONUP) switch(e->cbutton.button) {
                case SDL_CONTROLLER_BUTTON_DPAD_UP:
                    if(base->choice->current > 0) {
                        --base->choice->current;
                        choice_update_prompt(base);
                    } break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                    if(base->choice->current < base->choice->count) {
                        ++base->choice->current;
                        choice_update_prompt(base);
                    } break;
                case SDL_CONTROLLER_BUTTON_A:
                    cdlv_choice_handler(base, base->choice->current);
                    break;
        }
    }
}

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

void cdlv_render(cdlv_base* base, SDL_Renderer** r) {
    #define cdlv_temp_scene base->scenes[base->c_scene]
    #define cdlv_temp_canvas base->canvas
    #define cdlv_anim()                                     \
    base->accum += base->e_ticks * cdlv_temp_canvas->framerate;       \
    if(base->accum > 1) {                                   \
        size_t n_frame = base->c_image + 1;                 \
        n_frame %= cdlv_temp_scene->image_count;                      \
        if(n_frame < cdlv_temp_scene->image_count) {                  \
            base->c_image = n_frame;                        \
            base->accum = 0;                                \
            cdlv_canvas_copy_buffer(cdlv_temp_canvas,                 \
                    cdlv_temp_scene->images[base->c_image]->pixels);  \
        }                                                   \
    }
    SDL_RenderClear(*r);
    if(cdlv_temp_scene->type == cdlv_static_scene) {
        cdlv_canvas_copy_buffer(cdlv_temp_canvas,
                cdlv_scene_pixels(cdlv_temp_scene, base->c_image));
    } else if(cdlv_temp_scene->type == cdlv_anim_scene) {
        cdlv_anim();
    } else if(cdlv_temp_scene->type == cdlv_anim_text_scene) {
        if(base->c_image != cdlv_temp_scene->image_count-1) cdlv_anim();
    } else if(cdlv_temp_scene->type == cdlv_anim_once_scene
            || cdlv_temp_scene->type == cdlv_anim_wait_scene) {
        if(base->c_image != cdlv_temp_scene->image_count-1) {
            cdlv_anim();
        } else {
            base->can_interact = true;
            if(cdlv_temp_scene->type == cdlv_anim_once_scene)
                cdlv_update(base);
            else if(cdlv_temp_scene->type == cdlv_anim_wait_scene)
                cdlv_text_update(base, cdlv_continue);
        }
    }
    SDL_RenderCopy(*r, cdlv_temp_canvas->tex, NULL, NULL);
    if(base->can_interact) cdlv_text_render(base, *r);
    #undef cdlv_anim
    #undef cdlv_temp_canvas
    #undef cdlv_temp_scene
}

void cdlv_loop_start(cdlv_base* base, SDL_Event* e, int* run) {
    while(SDL_PollEvent(e) != 0) {
        if(e->type == SDL_QUIT) *run = false;
        cdlv_handle_keys(base, e);
    }
    base->c_tick = SDL_GetTicks64();
    base->e_ticks = (base->c_tick - base->l_tick) / 1000.0f;
}

void cdlv_loop_end(cdlv_base* base, SDL_Renderer** r) {
    SDL_RenderPresent(*r);
    base->l_tick = base->c_tick;
}

void cdlv_start(cdlv_base* base) {
    cdlv_scene_load(base, 0, 0);
}
