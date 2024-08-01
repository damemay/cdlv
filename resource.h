#ifndef CDLV_RESOURCE_H
#define CDLV_RESOURCE_H

#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include "cdlv.h"

typedef enum {
    cdlv_resource_image,
    cdlv_resource_video,
} cdlv_resource_type;

typedef struct cdlv_video {
    AVFormatContext* format_context;
    AVCodecContext* codec_context;
    struct SwsContext* sws_context;
    AVCodec* codec;
    AVFrame* frame;
    AVPacket* packet;
    AVFrame* picture;
    uint8_t* buffer;
    int video_stream;
    SDL_Rect rect;
    SDL_Texture* texture;
} cdlv_video;

typedef struct cdlv_resource {
    cdlv_resource_type type;
    bool loaded;
    char* path;
    union {
        SDL_Texture* image;
        cdlv_video* video;
    };
} cdlv_resource;

cdlv_error cdlv_resource_new(cdlv* base, const cdlv_resource_type type, char* path, cdlv_resource** resource);
cdlv_error cdlv_resource_load(cdlv* base, cdlv_resource* resource, SDL_Renderer* renderer);
void cdlv_resource_unload(cdlv_resource* resource);
void cdlv_resource_free(cdlv_resource* resource);
int cdlv_resources_free(cdlv_dict* resources);

#endif