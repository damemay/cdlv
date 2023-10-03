#include "cdlv.h"
#include "cdlv_util.h"

static inline void config_init(cdlv_config* config) {
    if(!config->text_font) strcpy(config->text_font, "../res/fonts/roboto.ttf");
    if(!config->text_size) config->text_size = 32;
    if(!config->text_wrap) config->text_wrap = 900;
    if(!config->text_xy.x) config->text_xy.x = 50;
    if(!config->text_xy.y) config->text_xy.y = 400;
    if(!config->text_color.a) config->text_color.a = 255;
}

void cdlv_config_from_file(cdlv_config* c, const char* path) {
    size_t lines;
    char** file = cdlv_read_file_in_lines(path, &lines);

    char name[cdlv_small_string];
    char value[cdlv_small_string];

    for(size_t i=0; i<lines; ++i) {
        if(!sscanf(file[i], "%s %s", name, value))
            cdlv_diev("Config error on line: %s", file[i]);

        if(!strcmp(name, "text_font"))          strncpy(c->text_font, value, cdlv_small_string-1);
        else if(!strcmp(name, "text_size"))     c->text_size = atoi(value);
        else if(!strcmp(name, "text_x"))        c->text_xy.x = atoi(value);
        else if(!strcmp(name, "text_y"))        c->text_xy.y = atoi(value);
        else if(!strcmp(name, "text_wrap"))     c->text_wrap = atoi(value);
        else if(!strcmp(name, "text_r"))        c->text_color.r = atoi(value);
        else if(!strcmp(name, "text_g"))        c->text_color.g = atoi(value);
        else if(!strcmp(name, "text_b"))        c->text_color.b = atoi(value);
        else if(!strcmp(name, "text_a"))        c->text_color.a = atoi(value);
        else if(!strcmp(name, "text_render_bg"))c->text_render_bg = atoi(value);
        else if(!strcmp(name, "text_speed"))    c->text_speed = atoi(value);
        else if(!strcmp(name, "dissolve_speed"))c->dissolve_speed = atoi(value);
    }

    cdlv_free_file_in_lines(file, lines);
}

cdlv_base* cdlv_create(cdlv_config* config) {
    cdlv_base* base = NULL;
    base = malloc(sizeof(cdlv_base));
    if(!base)
        cdlv_die("Could not allocate memory for cdlv_base!");

    config_init(config);

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
