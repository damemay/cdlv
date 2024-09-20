#ifndef CDLV_H
#define CDLV_H

#include <stdlib.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include "common.h"

typedef enum {
    cdlv_ok,
    cdlv_memory_error,
    cdlv_file_error,
    cdlv_read_error,
    cdlv_video_error,
    cdlv_parse_error,
    cdlv_fatal_error,
    cdlv_callback_error,
} cdlv_error;

typedef void (*cdlv_log_callback)(char* buf);
typedef void (*cdlv_error_callback)(cdlv_error error, void* user_data);

typedef struct cdlv_log_config {
    cdlv_log_callback callback;
    char* buffer;
    size_t buffer_size;
} cdlv_log_config;

typedef struct cdlv_error_config {
    cdlv_error_callback callback;
    void* user_data;
} cdlv_error_config;

typedef void* (*cdlv_image_load_cb)(const char* path, void* user_data);
typedef void (*cdlv_image_render_cb)(void* image, void* user_data);
typedef void (*cdlv_image_free_cb)(void* image);

typedef struct cdlv_image_config {
    cdlv_image_load_cb load_callback;
    cdlv_image_render_cb render_callback;
    cdlv_image_free_cb free_callback;
    void* user_data;
} cdlv_image_config;

typedef void* (*cdlv_video_load_cb)(const uint64_t width, const uint64_t height, void* user_data);
typedef void (*cdlv_video_update_cb)(cdlv_yuv_plane plane, cdlv_yuv_pitch pitch, void* texture, void* user_data);
typedef void (*cdlv_video_free_cb)(void* texture);

typedef struct cdlv_video_config {
    cdlv_video_load_cb load_callback;
    cdlv_video_free_cb free_callback;
    cdlv_video_update_cb update_callback;
    void* user_data;
    bool change_frame_bool;
} cdlv_video_config;

typedef int (*cdlv_user_update_cb)(void* user_data);
typedef void (*cdlv_line_cb)(const char* line, void* user_data);
typedef struct cdlv_user_config {
    cdlv_user_update_cb update_callback;
    cdlv_line_cb line_callback;
    void* user_data;
} cdlv_user_config;

typedef struct cdlv {
    cdlv_log_config log_config;
    cdlv_error_config error_config;
    cdlv_image_config image_config;
    cdlv_video_config video_config;
    cdlv_user_config user_config;

    bool end;

    cdlv_dict* scenes;
    uint16_t scene_count;

    uint16_t current_line;
    uint16_t current_scene_index;
    void* current_scene;
    void* current_bg;

    char* resources_path;
    cdlv_dict* resources;
} cdlv;

cdlv_error cdlv_set_script(cdlv* base, const char* path);
cdlv_error cdlv_unset_script(cdlv* base);
cdlv_error cdlv_render(cdlv* base);
cdlv_error cdlv_user_update(cdlv* base);

#endif
