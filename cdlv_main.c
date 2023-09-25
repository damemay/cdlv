#include "cdlv.h"

size_t iter = 0;
size_t count = 10000;

bool changing = false;

void cdlv_canvas_create(cdlv_base* base, const size_t w, const size_t h, const size_t fps, SDL_Renderer** r) {
    base->canvas = malloc(sizeof(cdlv_canvas));
    if(!base->canvas)
        cdlv_die("Could not allocate memory for cdlv_canvas!");
    base->canvas->tex = NULL;
    base->canvas->tex = SDL_CreateTexture(*r,
            SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, w, h);
    if(!base->canvas->tex)
        cdlv_diev("Could not create streaming texture: %s", SDL_GetError());
    base->canvas->w = w;
    base->canvas->h = h;
    base->canvas->raw_pitch = 0;
    base->canvas->raw_pixels = NULL;
    base->canvas->framerate = fps;
}

static inline void cdlv_load_images(cdlv_canvas* canvas, cdlv_scene* scene) {
    cdlv_alloc_ptr_arr(&scene->images, scene->image_count, SDL_Surface*);
    for(size_t i=0; i<scene->image_count; ++i) {
        scene->images[i] = malloc(sizeof(SDL_Surface*));
        if(!scene->images[i])
            cdlv_diev("Could not allocate memory "
            "for image with path: %s", scene->image_paths[i]);

        SDL_Surface* temp = NULL;
        temp = IMG_Load(scene->image_paths[i]);
        if(!temp)
            cdlv_diev("Could not load image "
            "with path \"%s\": %s", scene->image_paths[i], SDL_GetError());

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

        scene->images[i] = NULL;
        scene->images[i] = SDL_ConvertSurfaceFormat(temp, SDL_PIXELFORMAT_RGBA32, 0);
        if(!scene->images[i])
            cdlv_diev("Could not convert surfaces "
            "when handling image with path "
            "\"%s\": %s", scene->image_paths[i], SDL_GetError());
        if(SDL_SetSurfaceBlendMode(scene->images[i], SDL_BLENDMODE_BLEND) < 0)
            cdlv_diev("Could not set alpha blending on image with path \"%s\": %s",
                scene->image_paths[i], SDL_GetError());
        free(temp);
    }
}

static inline void cdlv_scene_clean(cdlv_scene* scene) {
    for(size_t i=0; i<scene->image_count; ++i)
        SDL_FreeSurface(scene->images[i]);
    free(scene->images);
    scene->images = NULL;
}

static inline void cdlv_scene_load(cdlv_base* base, const size_t prev, const size_t index) {
    if(index < base->scene_count) {
        iter = 0;
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
    base->choice = malloc(sizeof(cdlv_choice));
    if(!base->choice)
        cdlv_die("Could not allocate memory for a new scripted choice!");

    base->choice->count               = 0;
    base->choice->current             = 0;
    base->choice->state               = cdlv_parsing;
    base->choice->destinations        = NULL;
    base->choice->options             = NULL;
    base->choice->prompt              = NULL;
    cdlv_alloc_ptr_arr(&base->choice->destinations, cdlv_max_choice_count, size_t);
    cdlv_alloc_ptr_arr(&base->choice->options, cdlv_max_choice_count, char*);
    cdlv_alloc_ptr_arr(&base->choice->prompt, cdlv_max_string_size, char);
}

static inline void cdlv_choice_clean(cdlv_base* base) {
    for(size_t i=0;i<cdlv_max_choice_count; ++i)
        if(base->choice->options[i]) free(base->choice->options[i]);
    free(base->choice->options);
    free(base->choice->prompt);
    free(base->choice->destinations);
    free(base->choice);
    base->choice = NULL;
}

static inline void cdlv_choice_add(cdlv_base* base, const char* line) {
    const char chars[12] = "1234567890 ";
    size_t d = 0;
    size_t s = 0;
    if((sscanf(line, "%lu", &d)) != 0)
        if(d < base->scene_count)
            s = strspn(line, chars);
    if(s>0 && base->choice->count < cdlv_max_choice_count) {
        base->choice->destinations[base->choice->count] = d;
        cdlv_duplicate_string(&base->choice->options[base->choice->count], line+s, strlen(line+s)+1);
        ++base->choice->count;
    }
}

static inline void choice_update_prompt(cdlv_base* base) {
    if(base->choice->current < base->choice->count) {
        sprintf(base->choice->prompt, cdlv_arrow);
        for(size_t i=base->choice->current; i<base->choice->count; ++i) {
            strcat(base->choice->prompt, base->choice->options[i]);
            strcat(base->choice->prompt, "\n");
        }
        //strcat(choice->prompt, " ");
    }
    cdlv_text_update(base, base->choice->prompt);
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
                    if(cdlv_change_image(base, cdlv_temp_scene, cdlv_temp_line)) {
                        changing = true;
                        cdlv_update(base);
                    }
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
    if(base->choice->destinations[ch]) {
        cdlv_scene_load(base, base->c_scene, base->choice->destinations[ch]);
        return true;
    }
    return false;
}

static inline void cdlv_choice_handler(cdlv_base* base, size_t ch) {
    if(cdlv_choice_switch(base, ch)) cdlv_choice_clean(base);
}

void cdlv_handle_keys(cdlv_base* base, SDL_Event* e) {
    if(base->can_interact && !base->choice) {
        if(e->type == SDL_KEYUP && e->key.repeat == 0) switch(e->key.keysym.sym) {
                case SDLK_RETURN: cdlv_update(base); iter = 0; break;
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

static inline uint8_t lerp(uint8_t a, uint8_t b, size_t t) {
    int x = a*(256-t) + b*t;
    uint8_t res = x >> 8;
    return res;
}

static inline void dissolve(cdlv_base* base) {
    if(base->canvas->raw_pixels) {
        for(size_t y=0; y < base->canvas->h; y++) {
            uint32_t* can = (uint32_t*)(
                    (uint8_t*)base->canvas->raw_pixels +
                    base->canvas->raw_pitch * y);
            uint32_t* cur = (uint32_t*)(
                    (uint8_t*)
                    base->scenes[base->c_scene]->images[base->c_image]->pixels + 
                    base->scenes[base->c_scene]->images[base->c_image]->pitch * y);
            uint32_t* prv = (uint32_t*)(
                    (uint8_t*)
                    base->scenes[base->c_scene]->images[base->c_image-1]->pixels +
                    base->scenes[base->c_scene]->images[base->c_image-1]->pitch * y);
            for(size_t x=0; x < base->canvas->w; x++) {
                uint8_t r, g, b, a,
                        rc, gc, bc, ac;
                SDL_GetRGBA(*prv,
                        base->scenes[base->c_scene]->images[base->c_image]->format,
                        &r,&g,&b,&a);
                SDL_GetRGBA(*cur,
                        base->scenes[base->c_scene]->images[base->c_image]->format,
                        &rc,&gc,&bc,&ac);
                uint8_t rl = lerp(r, rc, iter);
                uint8_t gl = lerp(g, gc, iter);
                uint8_t bl = lerp(b, bc, iter);
                uint8_t al = lerp(a, ac, iter);
                *can = SDL_MapRGBA(
                        base->scenes[base->c_scene]->images[base->c_image]->format,
                        rl, gl, bl, al);
                can++;
                cur++;
                prv++;
            }
        }
        base->accum += base->e_ticks * 60.0f;
        if(base->accum > 1) {
            if(iter<256) iter+=4;
            else changing=false;
        }
    }
}

void cdlv_render(cdlv_base* base, SDL_Renderer** r) {
    #define cdlv_temp_scene base->scenes[base->c_scene]
    #define cdlv_anim()                                     \
    base->accum += base->e_ticks * base->canvas->framerate;       \
    if(base->accum > 1) {                                   \
        size_t n_frame = base->c_image + 1;                 \
        n_frame %= cdlv_temp_scene->image_count;                      \
        if(n_frame < cdlv_temp_scene->image_count) {                  \
            base->c_image = n_frame;                        \
            base->accum = 0;                                \
            cdlv_canvas_copy_buffer(base->canvas,                 \
                    cdlv_temp_scene->images[base->c_image]->pixels);  \
        }                                                   \
    }
    SDL_RenderClear(*r);
    if(cdlv_temp_scene->type == cdlv_static_scene) {
        if(changing) {
            cdlv_canvas_lock(base->canvas);
            if(base->c_image > 0) {
                dissolve(base);
            } else {
                cdlv_canvas_copy_pixels(base->canvas, cdlv_scene_pixels(cdlv_temp_scene, base->c_image));
                changing = false;
            }
            cdlv_canvas_unlock(base->canvas);
        } else cdlv_canvas_copy_buffer(base->canvas,
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
    SDL_RenderCopy(*r, base->canvas->tex, NULL, NULL);
    if(base->can_interact) cdlv_text_render(base, *r);
    #undef cdlv_anim
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
