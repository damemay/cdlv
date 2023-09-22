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

cdlv_base* cdlv_create();
void cdlv_clean_scenes(cdlv_base* base);
void cdlv_clean_canvas(cdlv_base* base);
void cdlv_clean_text(cdlv_base* base);
void cdlv_clean_all(cdlv_base* base);

void cdlv_loop_start(cdlv_base* base, SDL_Event* e, int* run);
void cdlv_render(cdlv_base* base, SDL_Renderer** r);
void cdlv_loop_end(cdlv_base* base, SDL_Renderer** r);
void cdlv_handle_keys(cdlv_base* base, SDL_Event* e);

void cdlv_canvas_create(cdlv_base* base, const size_t w, const size_t h, const size_t fps, SDL_Renderer** r);

void cdlv_read_file(cdlv_base* base, const char* file, SDL_Renderer** r);
void cdlv_start(cdlv_base* base);
cdlv_base* cdlv_init_from_script(const char* path, SDL_Renderer** r);

void cdlv_text_create(cdlv_base* base, const char* path, const size_t size, const uint32_t wrap, const size_t x, const size_t y, const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a, SDL_Renderer* renderer);
void cdlv_text_render(cdlv_base* base, SDL_Renderer* r);
void cdlv_text_update(cdlv_base* base, const char* content);

cdlv_menu* cdlv_menu_create(cdlv_base* base, const char* path, SDL_Renderer* r);
void cdlv_menu_render(cdlv_base* base, SDL_Renderer* r);
void cdlv_menu_handle_keys(cdlv_base** base, cdlv_menu** menu, sdl_base* sdl);
void cdlv_menu_clean(cdlv_menu* menu);

#endif
