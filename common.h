#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#define cdlv_max_string_size    UINT16_MAX
#define cdlv_small_string       UINT8_MAX
#define cdlv_max_choice_count   UINT8_MAX
#define cdlv_max_menu_entries   UINT8_MAX

#define cdlv_ascii_count        128
#define cdlv_font_atlas_size    1024

typedef struct dictionary cdlv_dict;
typedef SDL_Color cdlv_color;
typedef SDL_Point cdlv_vec2;
