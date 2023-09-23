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
    bool path_exists;
} cdlv_menu;


cdlv_menu* cdlv_menu_create(cdlv_base* base, const char* path, sdl_base* sdl);
void cdlv_menu_render(cdlv_base* base, sdl_base* sdl);
void cdlv_menu_handle_keys(cdlv_base** base, cdlv_menu** menu, sdl_base* sdl);
void cdlv_menu_clean(cdlv_menu* menu);

#endif
