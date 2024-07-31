#include "cdlv.h"
#include "file.h"
#include "parse.h"
#include "util.h"
#include "resource.h"
#include "scene.h"

void cdlv_init(cdlv* base, uint16_t width, uint16_t height) {
    base->error = 0;
    base->width = width;
    base->height = height;
    base->resources = dic_new(0);
    base->scenes = dic_new(0);
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
    size_t lines;
    char* script = NULL;

    if((res = extract_path(base, path)) != cdlv_ok) {
        cdlv_err(res);
    }

    if((res = cdlv_read_file_to_str(base, path, &script)) != cdlv_ok) {
        cdlv_err(res);
    }

    if((res = cdlv_parse_script(base, script)) != cdlv_ok) {
        cdlv_err(res);
    }

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
}

static inline int unload_global_resources(void *key, int count, void **value, void *user) {
    cdlv_resource* resource = (cdlv_resource*)*value;
    cdlv_resource_unload(resource);
}

cdlv_error cdlv_play(cdlv* base, SDL_Renderer* renderer) {
    loader_args args = {
        .base = base,
        .renderer = renderer,
    };
    dic_forEach(base->resources, load_global_resources, &args);
    base->is_playing = true;
}

cdlv_error cdlv_loop(cdlv* base, SDL_Renderer* renderer) {

}

cdlv_error cdlv_stop(cdlv* base) {
    base->is_playing = false;
    dic_forEach(base->resources, unload_global_resources, NULL);
}

void cdlv_free(cdlv* base) {
    free(base->resources_path);
    cdlv_resources_free(base->resources);
    cdlv_scenes_free(base->scenes);
}