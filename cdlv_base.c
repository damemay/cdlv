#include "cdlv.h"
#include "cdlv_util.h"

cdlv_base* cdlv_create(cdlv_config* config) {
    cdlv_base* base = NULL;
    base = malloc(sizeof(cdlv_base));
    if(!base)
        cdlv_die("Could not allocate memory for cdlv_base!");

    base->config        = config;
    base->canvas        = NULL;
    base->text          = NULL;
    base->choice        = NULL;
    base->scenes        = NULL;
    base->scene_count   = 0;
    base->c_tick        = SDL_GetTicks64();
    base->l_tick        = SDL_GetTicks64();
    base->e_ticks       = 0.0f;
    base->c_line        = 0;
    base->c_scene       = 0;
    base->p_scene       = 0;
    base->c_image       = 0;
    base->accum         = 0.0f;
    base->can_interact  = true;
    base->state         = cdlv_main_menu;


    return base;
};

cdlv_base* cdlv_init_from_script(cdlv_config* config, const char* path, SDL_Renderer** r) {
    cdlv_base* base = NULL;
    base = cdlv_create(config);
    base->state = cdlv_main_run;
    cdlv_read_file(base, path, r);
    cdlv_start(base);
    return base;
}

void cdlv_clean_scenes(cdlv_base* base) {
    if(base->scenes) {
        for(size_t i=0; i<base->scene_count; ++i)
            if(base->scenes[i]) {
                if(base->scenes[i]->image_paths) {
                    for(size_t j=0; j<base->scenes[i]->image_count; ++j) {
                        if(base->scenes[i]->image_paths[j])
                            free(base->scenes[i]->image_paths[j]);
                    }
                    free(base->scenes[i]->image_paths);
                    if(base->scenes[i]->images)
                        free(base->scenes[i]->images);
                }
                if(base->scenes[i]->script) {
                    for(size_t j=0; j<base->scenes[i]->line_count; ++j)
                        free(base->scenes[i]->script[j]);
                    free(base->scenes[i]->script);
                }
                free(base->scenes[i]);
            }
        free(base->scenes);
    }
}

void cdlv_clean_canvas(cdlv_base* base) {
    if(base->canvas) {
        SDL_DestroyTexture(base->canvas->tex);
        if(base->canvas->raw_pixels) free(base->canvas->raw_pixels);
        free(base->canvas);
    }
}

void cdlv_clean_text(cdlv_base* base) {
    if(base->text) {
        TTF_CloseFont(base->text->font);
        if(base->text->tex) SDL_DestroyTexture(base->text->tex);
        if(base->text->glyphs) free(base->text->glyphs);
        free(base->text);
    }
}

void cdlv_clean_all(cdlv_base* base) {
    cdlv_clean_scenes(base);
    cdlv_clean_canvas(base);
    cdlv_clean_text(base);
    free(base);
}
