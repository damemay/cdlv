#include "cdlv.h"
#include "cdlv_util.h"

static inline void sdl_init(const char* title,
        const size_t w, const size_t h,
        SDL_Window** sdl_w, SDL_Renderer** sdl_r,
        SDL_GameController** sdl_g) {
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
        cdlv_diev("Could not init SDL: %s", SDL_GetError());
    atexit(SDL_Quit);

    if(!(*sdl_w = SDL_CreateWindow(title,
                    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                    w, h, SDL_WINDOW_SHOWN)))
        cdlv_diev("Could not create SDL_Window: %s", SDL_GetError());

    if(!(*sdl_r = SDL_CreateRenderer(*sdl_w, -1,
                    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)))
        cdlv_diev("Could not create SDL_Renderer: %s", SDL_GetError());

    if(!(IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG) & IMG_INIT_JPG | IMG_INIT_PNG))
        cdlv_diev("Could not init SDL_image: %s", IMG_GetError());
    atexit(IMG_Quit);

    if(TTF_Init() == -1)
        cdlv_diev("Could not init SDL_ttf: %s", TTF_GetError());
    atexit(TTF_Quit);
    
    int joysticks;
    if((joysticks = SDL_NumJoysticks()) > 0) {
        sdl_g = calloc(joysticks, sizeof(SDL_GameController*));
        for(int i=0; i<joysticks; i++)
            sdl_g[i] = SDL_GameControllerOpen(i);
    } else sdl_g = NULL;
}

static inline void sdl_clean_(SDL_Window** sdl_w, SDL_Renderer** sdl_r, SDL_GameController** sdl_g) {
    if(sdl_g) {
        for(int i=0; i<sizeof(sdl_g); i++) SDL_GameControllerClose(sdl_g[i]);
        free(sdl_g);
    }
    if(*sdl_r) SDL_DestroyRenderer(*sdl_r);
    if(*sdl_w) SDL_DestroyWindow(*sdl_w);
}

sdl_base* sdl_create(const char* title, const size_t w, const size_t h) {
    sdl_base* base = NULL;
    base = malloc(sizeof(sdl_base));
    if(!base)
        cdlv_die("Could not allocate memory for cdlv_base!");

    base->window        = NULL;
    base->renderer      = NULL;
    base->gamepads      = NULL;
    base->run           = true;
    base->title         = title;
    base->w             = w;
    base->h             = h;

    sdl_init(title, w, h, &base->window, &base->renderer, base->gamepads);
    SDL_SetRenderDrawColor(base->renderer, 0, 0, 0, 255);

    return base;
}

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

void sdl_clean(sdl_base* base) {
    if(base->window || base->renderer)
        sdl_clean_(&base->window, &base->renderer, base->gamepads);
    free(base);
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
