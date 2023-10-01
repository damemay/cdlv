#ifndef CDLV_H
#define CDLV_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include "cdlv_macros.h"
#include "cdlv_types.h"

sdl_base* sdl_create(const char* title, const size_t w, const size_t h);
void sdl_clean(sdl_base* base);

void cdlv_start(cdlv_base* base);
void cdlv_config_from_file(cdlv_config* c, const char* path);
cdlv_base* cdlv_init_from_script(cdlv_config* config, const char* path, SDL_Renderer** r);

cdlv_base* cdlv_create(cdlv_config* config);

void cdlv_loop_start(cdlv_base* base, SDL_Event* e, int* run);
void cdlv_render(cdlv_base* base, SDL_Renderer** r);
void cdlv_loop_end(cdlv_base* base, SDL_Renderer** r);

void cdlv_clean_scenes(cdlv_base* base);
void cdlv_clean_canvas(cdlv_base* base);
void cdlv_clean_text(cdlv_base* base);
void cdlv_clean_all(cdlv_base* base);

#endif
