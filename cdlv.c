#include "cdlv.h"
#include "file.h"
#include "parse.h"
#include "util.h"
#include "resource.h"
#include "scene.h"

void cdlv_init(cdlv* base, uint16_t width, uint16_t height) {
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

cdlv_error cdlv_add_script(cdlv* base, const char* path) {
    cdlv_error res;
    size_t lines;
    char* script = NULL;

    if((res = cdlv_read_file_to_str(base, path, &script)) != cdlv_ok) {
        cdlv_err(res);
    }

    if((res = cdlv_parse_script(base, script)) != cdlv_ok) {
        cdlv_err(res);
    }

    free(script);
    cdlv_err(cdlv_ok);
}

static inline int free_dict(void *key, int count, void **value, void *user) {
    free(*value);
}

void cdlv_free(cdlv* base) {
    free(base->resources_path);
    cdlv_resources_free(base->resources);
    cdlv_scenes_free(base->scenes);
}