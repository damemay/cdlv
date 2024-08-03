#include "cdlv.h"
#include "file.h"
#include "parse.h"
#include "util.h"
#include "resource.h"
#include "scene.h"
#include "play.h"
#include "text.h"

void cdlv_init(cdlv* base, uint16_t width, uint16_t height) {
    base->error = 0;
    base->call_stop = 0;
    base->scene_count = 0;
    base->width = width;
    base->height = height;
    base->current_tick = SDL_GetTicks64();
    base->last_tick = SDL_GetTicks64();
    base->elapsed_ticks = 0.0f;
    base->accum = 0.0f;
    base->resources = dic_new(0);
    base->scenes = dic_new(0);
    base->can_interact = true;
}

void cdlv_set_config(cdlv* base, const cdlv_config config) {
    base->config = config;
    if(!base->config.text_font[0]) strcpy(base->config.text_font, "../res/fonts/roboto.ttf");
    if(!base->config.text_size) base->config.text_size = 32;
    if(!base->config.text_wrap) base->config.text_wrap = 900;
    if(!base->config.text_xy.x) base->config.text_xy.x = 50;
    if(!base->config.text_xy.y) base->config.text_xy.y = 400;
    if(!base->config.text_color.a) base->config.text_color.a = 255;
}

static inline cdlv_error extract_path(cdlv* base, const char* path) {
    char* sl = strrchr(path, '/');
    if(!sl) {
        sl = strrchr(path, '\\');
        if(!sl) {
            cdlv_logv("Detected improper script file path: %s", path);
            cdlv_err(cdlv_file_error);
        }
    }
    char basepath[sl-path+2];
    sprintf(basepath, "%.*s", (int)(sl-path+1), path);
    cdlv_strdup(&base->resources_path, basepath, sl-path+2);
    cdlv_err(cdlv_ok);
}

cdlv_error cdlv_add_script(cdlv* base, const char* path) {
    cdlv_error res;
    char* script = NULL;

    if((res = extract_path(base, path)) != cdlv_ok) cdlv_err(res);
    if((res = cdlv_read_file_to_str(base, path, &script)) != cdlv_ok) cdlv_err(res);
    if((res = cdlv_parse_script(base, script)) != cdlv_ok) cdlv_err(res);

    free(script);
    cdlv_err(cdlv_ok);
}

typedef struct {
    cdlv* base;
    SDL_Renderer* renderer;
} loader_args;

static inline int load_global_resources(void *key, int count, void **value, void *user) {
    cdlv_resource* resource = (cdlv_resource*)*value;
    loader_args* args = (loader_args*)user;
    cdlv_resource_load(args->base, resource, args->renderer);
    return 1;
}

static inline int unload_global_resources(void *key, int count, void **value, void *user) {
    cdlv_resource* resource = (cdlv_resource*)*value;
    cdlv_resource_unload(resource);
    return 1;
}

cdlv_error cdlv_play(cdlv* base, SDL_Renderer* renderer) {
    loader_args args = {
        .base = base,
        .renderer = renderer,
    };
    dic_forEach(base->resources, load_global_resources, &args);
    cdlv_error res;
    cdlv_vec2 xy = {
        .x = base->config.text_xy.x,
        .y = base->config.text_xy.y,
    };
    cdlv_color color = {
        .r = base->config.text_color.r,
        .g = base->config.text_color.g,
        .b = base->config.text_color.b,
        .a = base->config.text_color.a,
    };
    if((res = cdlv_text_create(base, base->config.text_font, base->config.text_size, base->config.text_wrap, xy, color, renderer)) != cdlv_ok) cdlv_err(res);
    base->is_playing = true;
    cdlv_err(cdlv_ok);
}

cdlv_error cdlv_event(cdlv* base, SDL_Renderer* renderer, SDL_Event event) {
    cdlv_error res;
    if((res = cdlv_key_handler(base, renderer, event)) != cdlv_ok) cdlv_err(res);
    cdlv_err(cdlv_ok);
}

cdlv_error cdlv_loop(cdlv* base, SDL_Renderer* renderer) {
    cdlv_error res;
    if((res = cdlv_play_loop(base, renderer)) != cdlv_ok) cdlv_err(res);
    cdlv_err(cdlv_ok);
}

cdlv_error cdlv_stop(cdlv* base) {
    base->is_playing = false;
    dic_forEach(base->resources, unload_global_resources, NULL);
    cdlv_text_free((cdlv_text*)base->text);
    cdlv_err(cdlv_ok);
}

void cdlv_free(cdlv* base) {
    free(base->resources_path);
    cdlv_resources_free(base->resources);
    cdlv_scenes_free(base->scenes);
}