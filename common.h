#ifndef CDLV_COMMON_H
#define CDLV_COMMON_H

#include <stdbool.h>
#include <stdint.h>

#define cdlv_max_string_size    UINT16_MAX
#define cdlv_small_string       UINT8_MAX
#define cdlv_max_choice_count   UINT8_MAX
#define cdlv_max_menu_entries   UINT8_MAX

#define cdlv_ascii_count        128
#define cdlv_font_atlas_size    1024

typedef struct dictionary cdlv_dict;
typedef struct cdlv_color {
    uint8_t r; 
    uint8_t g; 
    uint8_t b; 
    uint8_t a; 
} cdlv_color;
typedef struct cdlv_vec2 {
    uint64_t x;
    uint64_t y;
} cdlv_vec2;
typedef struct cdlv_yuv_plane {
    uint8_t* y;
    uint8_t* u;
    uint8_t* v;
} cdlv_yuv_plane;
typedef struct cdlv_yuv_pitch {
    int y;
    int u;
    int v;
} cdlv_yuv_pitch;


#endif
