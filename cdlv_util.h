#ifndef CDLV_UTIL_H
#define CDLV_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include "cdlv.h"
#include "cdlv_macros.h"
#include "cdlv_types.h"

int cdlv_choice_create(cdlv_base* base);
void cdlv_choice_add(cdlv_base* base, const char* line);
void cdlv_choice_handler(cdlv_base* base, size_t ch);

void cdlv_scene_load(cdlv_base* base, const size_t prev, const size_t index);
void cdlv_update(cdlv_base* base);
void cdlv_handle_keys(cdlv_base* base, SDL_Event* e);
int cdlv_canvas_create(cdlv_base* base, const size_t w, const size_t h, const size_t fps, SDL_Renderer** r);
int cdlv_read_file(cdlv_base* base, const char* file, SDL_Renderer** r);

char* cdlv_read_file_to_str(const char* path);
char** cdlv_read_file_in_lines(const char* path, size_t* line_count);
void cdlv_free_file_in_lines(char** file, const size_t line_count);

#endif
