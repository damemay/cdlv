#include "cdlv.h"
#include "cdlv_util.h"

static inline void sdl_init(const char* title,
        const size_t w, const size_t h,
        SDL_Window** sdl_w, SDL_Renderer** sdl_r,
        SDL_GameController** sdl_g) {
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
        cdlv_diev("Could not init SDL: %s", SDL_GetError());

    if(!(*sdl_w = SDL_CreateWindow(title,
                    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                    w, h, SDL_WINDOW_SHOWN)))
        cdlv_diev("Could not create SDL_Window: %s", SDL_GetError());

    if(!(*sdl_r = SDL_CreateRenderer(*sdl_w, -1,
                    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)))
        cdlv_diev("Could not create SDL_Renderer: %s", SDL_GetError());

    if(!(IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG) & (IMG_INIT_JPG | IMG_INIT_PNG)))
        cdlv_diev("Could not init SDL_image: %s", IMG_GetError());

    if(TTF_Init() == -1)
        cdlv_diev("Could not init SDL_ttf: %s", TTF_GetError());
    
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

void sdl_clean(sdl_base* base) {
    if(base->window || base->renderer)
        sdl_clean_(&base->window, &base->renderer, base->gamepads);
    free(base);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

