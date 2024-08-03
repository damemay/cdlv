#ifndef CDLV_H
#define CDLV_H

#include "common.h"
#include "array.h"
#include "hashdict.c/hashdict.h"

typedef enum {
    cdlv_ok,
    cdlv_memory_error,
    cdlv_file_error,
    cdlv_read_error,
    cdlv_video_error,
    cdlv_parse_error,
    cdlv_fatal_error,
} cdlv_error;

typedef struct cdlv_config {
    char text_font[cdlv_max_string_size];
    uint8_t text_size;
    cdlv_vec2 text_xy;
    uint16_t text_wrap;
    cdlv_color text_color;
    int text_render_bg;
    uint8_t text_speed;
    uint8_t dissolve_speed;
} cdlv_config;

typedef struct cdlv {
    bool call_stop;
    bool is_playing;
    bool can_interact;

    cdlv_error error;
    char log[cdlv_max_string_size];

    uint16_t width, height;

    cdlv_config config;

    cdlv_dict* scenes;
    uint16_t scene_count;

    uint16_t current_line;
    uint16_t current_scene_index;
    void* current_scene;
    void* current_bg;

    uint64_t current_tick, last_tick;
    float accum, elapsed_ticks;

    char* resources_path;
    cdlv_dict* resources;

    void* text;
} cdlv;

void cdlv_init(cdlv* base, uint16_t width, uint16_t height);
void cdlv_set_config(cdlv* base, const cdlv_config config);
cdlv_error cdlv_add_script(cdlv* base, const char* path);

cdlv_error cdlv_play(cdlv* base, SDL_Renderer* renderer);
cdlv_error cdlv_event(cdlv* base, SDL_Renderer* renderer, SDL_Event event);
cdlv_error cdlv_loop(cdlv* base, SDL_Renderer* renderer);
cdlv_error cdlv_stop(cdlv* base);

void cdlv_free(cdlv* base);

#endif
