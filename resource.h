#ifndef CDLV_RESOURCE_H
#define CDLV_RESOURCE_H

#include "cdlv.h"

typedef enum {
    cdlv_resource_image,
    cdlv_resource_video,
} cdlv_resource_type;

typedef struct cdlv_video {
#ifdef CDLV_FFMPEG
    AVFormatContext* format_context;
    AVCodecContext* codec_context;
    AVCodec* codec;
    AVFrame* frame;
    AVPacket* packet;
#endif
    int video_stream;
    cdlv_vec2 dimensions;
    cdlv_yuv_plane plane;
    cdlv_yuv_pitch pitch;
    double fps;
    bool is_playing;
    bool loop;
    bool eof;
    void* texture;
} cdlv_video;

typedef struct cdlv_resource {
    cdlv_resource_type type;
    bool loaded;
    char* path;
    union {
        cdlv_video* video;
        void* image;
    };
} cdlv_resource;

cdlv_error cdlv_resource_new(cdlv* base, const cdlv_resource_type type, char* path, cdlv_resource** resource);
cdlv_error cdlv_resource_load(cdlv* base, cdlv_resource* resource);
void cdlv_resource_unload(cdlv* base, cdlv_resource* resource);
void cdlv_resource_free(cdlv* base, cdlv_resource* resource);
void cdlv_resources_free(cdlv* base, cdlv_dict* resources);

cdlv_error cdlv_play_video(cdlv* base, cdlv_video* video);

#endif
