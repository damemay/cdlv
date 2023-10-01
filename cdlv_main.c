#include "cdlv.h"
#include "cdlv_util.h"

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
    base->canvas->iter = 0;
    if(base->config->dissolve_speed) base->canvas->changing = true;
    else base->canvas->changing = false;
}

static inline void cdlv_load_images(cdlv_canvas* canvas, cdlv_scene* scene) {
    cdlv_alloc_ptr_arr(&scene->images, scene->image_count, SDL_Surface*);
    for(size_t i=0; i<scene->image_count; ++i) {
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

static inline void cdlv_scene_clean_leave_last(cdlv_scene* scene) {
    for(size_t i=0; i<scene->image_count-1; ++i)
        SDL_FreeSurface(scene->images[i]);
}

void cdlv_scene_load(cdlv_base* base, const size_t prev, const size_t index) {
    if(index < base->scene_count) {
        base->canvas->iter = 0;
        if(base->config->dissolve_speed) base->canvas->changing = true;
        else base->canvas->changing = false;
        base->c_scene = index;
        base->p_scene = prev;
        base->c_image = 0;
        base->c_line  = 0;
        cdlv_load_images(base->canvas, base->scenes[index]);
        if(base->scenes[index]->type != cdlv_anim_once_scene && base->scenes[index]->type != cdlv_anim_wait_scene) {
            cdlv_text_update(base, base->scenes[index]->script[base->c_line]);
            base->can_interact = true;
        } else base->can_interact = false;
        if(base->c_scene) {
            if(base->config->dissolve_speed) 
                cdlv_scene_clean_leave_last(base->scenes[prev]);
            else cdlv_scene_clean(base->scenes[prev]);
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

void cdlv_update(cdlv_base* base) {
    if(base->text->current_char != base->text->content_size) {
        strcpy(base->text->rendered, base->text->content);
        base->text->current_char = base->text->content_size;
        return;
    }

    cdlv_scene* cdlv_temp_scene = base->scenes[base->c_scene];
    char* cdlv_temp_line = base->scenes[base->c_scene]->script[base->c_line];
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
            cdlv_temp_line = base->scenes[base->c_scene]->script[base->c_line];
            if(cdlv_temp_line[0] == '@') {
                // @ is now a function determinator, we need to check what it actually does
                if(strstr(cdlv_temp_line, cdlv_tag_func_image)) {
                    if(cdlv_change_image(base, cdlv_temp_scene, cdlv_temp_line)) {
                        base->canvas->iter = 0;
                        if(base->config->dissolve_speed) base->canvas->changing = true;
                        else base->canvas->changing = false;
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
