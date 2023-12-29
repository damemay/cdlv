#ifndef CDLV_MENU_H
#define CDLV_MENU_H

#include <SDL2/SDL.h>
#include "../cdlv_types.h"

typedef struct cdlv_menu {
    char* path;
    char** files;
    char* text;
    size_t file_count;
    size_t current_choice;
    uint8_t text_speed;
    bool path_exists;
} cdlv_menu;

typedef struct sdl_base {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_GameController** gamepads;
    SDL_Event event;
    int run;
    uint16_t w, h;
    const char* title;
} sdl_base;

int check_errors(cdlv_base* base, cdlv_error err);

cdlv_menu* cdlv_menu_create(cdlv_base* base, const char* path, sdl_base* sdl);
void cdlv_menu_render(cdlv_base* base, sdl_base* sdl);
void cdlv_menu_handle_keys(cdlv_base** base, cdlv_menu** menu, sdl_base* sdl);
void cdlv_menu_clean(cdlv_menu* menu);

sdl_base* sdl_create(const char* title, const size_t w, const size_t h);
void sdl_clean(sdl_base* base);

#endif
