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

void cdlv_start(cdlv_base* base);
cdlv_error cdlv_config_from_file(cdlv_config* c, const char* path);
cdlv_base* cdlv_init_from_script(cdlv_config* config, const char* path, SDL_Renderer** r);

cdlv_base* cdlv_create(cdlv_config* config);

void cdlv_loop_start(cdlv_base* base, SDL_Event* e, int* run);
void cdlv_render(cdlv_base* base, SDL_Renderer** r);
void cdlv_loop_end(cdlv_base* base, SDL_Renderer** r);

void cdlv_text_create(cdlv_base* base, const char* path, const size_t size, const uint32_t wrap, const size_t x, const size_t y, const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a, SDL_Renderer* renderer);
void cdlv_text_render(cdlv_base* base, SDL_Renderer* r);
void cdlv_text_update(cdlv_base* base, const char* content);

void cdlv_clean_text(cdlv_base* base);
void cdlv_clean_all(cdlv_base* base);

#endif
