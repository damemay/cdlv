#include "cdlv.h"
#include <SDL_render.h>

static inline void sdl_init(const char* title,
        const size_t w, const size_t h,
        SDL_Window** sdl_w, SDL_Renderer** sdl_r,
        SDL_GameController** sdl_g) {
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
        diev("Could not init SDL: %s", SDL_GetError());
    atexit(SDL_Quit);

    if(!(*sdl_w = SDL_CreateWindow(title,
                    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                    w, h, SDL_WINDOW_SHOWN)))
        diev("Could not create SDL_Window: %s", SDL_GetError());

    if(!(*sdl_r = SDL_CreateRenderer(*sdl_w, -1,
                    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)))
        diev("Could not create SDL_Renderer: %s", SDL_GetError());

    if(!(IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG) & IMG_INIT_JPG | IMG_INIT_PNG))
        diev("Could not init SDL_image: %s", IMG_GetError());
    atexit(IMG_Quit);

    if(TTF_Init() == -1)
        diev("Could not init SDL_ttf: %s", TTF_GetError());
    atexit(TTF_Quit);
    
    int joysticks;
    if((joysticks = SDL_NumJoysticks()) > 0) {
        sdl_g = calloc(joysticks, sizeof(SDL_GameController*));
        for(int i=0; i<joysticks; i++)
            sdl_g[i] = SDL_GameControllerOpen(i);
    } else sdl_g = NULL;
}

static inline void sdl_clean(SDL_Window** sdl_w, SDL_Renderer** sdl_r, SDL_GameController** sdl_g) {
    if(sdl_g) {
        for(int i=0; i<sizeof(sdl_g); i++) SDL_GameControllerClose(sdl_g[i]);
        free(sdl_g);
    }
    if(*sdl_r) SDL_DestroyRenderer(*sdl_r);
    if(*sdl_w) SDL_DestroyWindow(*sdl_w);
}

cdlv_base* cdlv_create(const char* title, const size_t w, const size_t h) {
    cdlv_base* base = NULL;
    base = malloc(sizeof(cdlv_base));
    if(!base)
        die("Could not allocate memory for cdlv_base!");

    base->window        = NULL;
    base->renderer      = NULL;
    base->gamepads      = NULL;
    base->run           = true;
    base->c_tick        = SDL_GetTicks64();
    base->l_tick        = SDL_GetTicks64();
    base->e_ticks       = 0.0f;

    base->canvas        = NULL;
    base->text          = NULL;
    base->scenes        = NULL;
    base->scene_count   = 0;
    base->c_line        = 0;
    base->c_scene       = 0;
    base->c_image       = 0;
    base->accum         = 0.0f;
    base->can_interact  = true;

    sdl_init(title, w, h, &base->window, &base->renderer, base->gamepads);
    SDL_SetRenderDrawColor(base->renderer, 0, 0, 0, 255);

    return base;
};

void cdlv_clean(cdlv_base* base) {
    if(base->scenes) {
        for(size_t i=0; i<base->scene_count; ++i)
            if(base->scenes[i]) free(base->scenes[i]);
        free(base->scenes);
    }
    if(base->canvas) {
        SDL_DestroyTexture(base->canvas->tex);
        if(base->canvas->raw_pixels) free(base->canvas->raw_pixels);
        free(base->canvas);
    }
    if(base->text) {
        TTF_CloseFont(base->text->font);
        if(base->text->tex) SDL_DestroyTexture(base->text->tex);
        free(base->text);
    }
    sdl_clean(&base->window, &base->renderer, base->gamepads);
    free(base);
}
