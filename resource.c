#include "resource.h"
#include "util.h"

cdlv_error cdlv_resource_new(cdlv* base, const cdlv_resource_type type, char* path, cdlv_resource** resource) {
    cdlv_resource* new_resource = malloc(sizeof(cdlv_resource));
    if(!new_resource) {
        cdlv_logv("Could not allocate memory for resource from file: %s", path);
        cdlv_err(cdlv_memory_error);
    }
    new_resource->type = type;
    new_resource->loaded = false;
    new_resource->path = path;
    new_resource->image = NULL;
    *resource = new_resource;
    cdlv_err(cdlv_ok);
}

static inline cdlv_error cdlv_resource_image_load(cdlv* base, cdlv_resource* resource, SDL_Renderer* renderer) {
    resource->image = IMG_LoadTexture(renderer, resource->path);
    if(!resource->image) {
        cdlv_logv("%s", SDL_GetError());
        cdlv_err(cdlv_file_error);
    }
    cdlv_err(cdlv_ok);
}

static inline cdlv_error cdlv_resource_video_load(cdlv* base, cdlv_resource* resource, SDL_Renderer* renderer) {
    resource->video = malloc(sizeof(cdlv_video));
    if(!resource->video) {
        cdlv_log("Could not allocate memory for new video");
        cdlv_err(cdlv_memory_error);
    }
    resource->video->format_context = NULL;
    if(avformat_open_input(&resource->video->format_context, resource->path, NULL, NULL) < 0) {
        cdlv_logv("Could not open video file: %s", resource->path);
        cdlv_err(cdlv_file_error);
    }
    if(avformat_find_stream_info(resource->video->format_context, NULL) < 0) {
        cdlv_logv("Could not find stream info in video file: %s", resource->path);
        cdlv_err(cdlv_video_error);
    }
    av_dump_format(resource->video->format_context, 0, resource->path, 0);
    resource->video->video_stream = -1;
    for(size_t i=0; i < resource->video->format_context->nb_streams; i++) {
        if(resource->video->format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            resource->video->video_stream = i;
            break;
        }
    }
    if(resource->video->video_stream == -1) {
        cdlv_logv("Could not find video stream in video file: %s", resource->path);
        cdlv_err(cdlv_video_error);
    }
    resource->video->codec = (AVCodec*)avcodec_find_decoder(resource->video->format_context->streams[resource->video->video_stream]->codecpar->codec_id);
    if(!resource->video->codec) {
        cdlv_logv("Detected unsupported codec in video file: %s", resource->path);
        cdlv_err(cdlv_video_error);
    }
    resource->video->codec_context = avcodec_alloc_context3(resource->video->codec);
    if(!resource->video->codec_context) {
        cdlv_log("Could not allocate memory for codec context");
        cdlv_err(cdlv_memory_error);
    }
    if(avcodec_parameters_to_context(resource->video->codec_context, resource->video->format_context->streams[resource->video->video_stream]->codecpar) != 0) {
        cdlv_logv("Could not copy codec context in video file: %s", resource->path);
        cdlv_err(cdlv_video_error);
    }
    if(avcodec_open2(resource->video->codec_context, resource->video->codec, NULL) < 0) {
        cdlv_logv("Could not open codec in video file: %s", resource->path);
        cdlv_err(cdlv_video_error);
    }
    resource->video->frame = av_frame_alloc();
    if(!resource->video->frame) {
        cdlv_log("Could not allocate memory for video frame");
        cdlv_err(cdlv_memory_error);
    }
    resource->video->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, resource->video->codec_context->width, resource->video->codec_context->height);
    if(!resource->video->texture) {
        cdlv_logv("%s", SDL_GetError());
        cdlv_err(cdlv_memory_error);
    }
    resource->video->packet = av_packet_alloc();
    if(!resource->video->packet) {
        cdlv_log("Could not allocate memory for video packet");
        cdlv_err(cdlv_memory_error);
    }
    resource->video->sws_context = sws_getContext(resource->video->codec_context->width, resource->video->codec_context->height, resource->video->codec_context->pix_fmt, resource->video->codec_context->width, resource->video->codec_context->height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);
    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, resource->video->codec_context->width, resource->video->codec_context->height, 32);
    resource->video->buffer = av_malloc(num_bytes*sizeof(uint8_t));
    resource->video->picture = av_frame_alloc();
    av_image_fill_arrays(resource->video->picture->data, resource->video->picture->linesize, resource->video->buffer, AV_PIX_FMT_YUV420P, resource->video->codec_context->width, resource->video->codec_context->height, 32);
    resource->video->rect.w = resource->video->codec_context->width;
    resource->video->rect.h = resource->video->codec_context->height;
    double fps = av_q2d(resource->video->format_context->streams[resource->video->video_stream]->r_frame_rate);
    resource->video->sleep_time = 1.0/(double)fps;

    cdlv_err(cdlv_ok);
}

cdlv_error cdlv_resource_load(cdlv* base, cdlv_resource* resource, SDL_Renderer* renderer) {
    cdlv_error res;
    if(resource->type == cdlv_resource_image) {
        if((res = cdlv_resource_image_load(base, resource, renderer)) != cdlv_ok) cdlv_err(res);
    } else if(resource->type == cdlv_resource_video) {
        if((res = cdlv_resource_video_load(base, resource, renderer)) != cdlv_ok) cdlv_err(res);
    }
    resource->loaded = true;
    printf("loaded resource: %s\n", resource->path);
    cdlv_err(cdlv_ok);
}

static inline void cdlv_resource_image_unload(cdlv_resource* resource) {
    SDL_DestroyTexture(resource->image);
    resource->image = NULL;
} 

static inline void cdlv_resource_video_unload(cdlv_resource* resource) {
    av_frame_free(&resource->video->picture);
    av_free(resource->video->buffer);
    sws_freeContext(resource->video->sws_context);
    av_packet_free(&resource->video->packet);
    av_frame_free(&resource->video->frame);
    avcodec_free_context(&resource->video->codec_context);
    avformat_close_input(&resource->video->format_context);
    SDL_DestroyTexture(resource->video->texture);
    free(resource->video);
    resource->video = NULL;
} 

void cdlv_resource_unload(cdlv_resource* resource) {
    if(resource->type == cdlv_resource_image) {
        cdlv_resource_image_unload(resource);
    } else if(resource->type == cdlv_resource_video) {
        cdlv_resource_video_unload(resource);
    }
    resource->loaded = false;
}

void cdlv_resource_free(cdlv_resource* resource) {
    if(resource->loaded) {
        cdlv_resource_unload(resource);
    }
    free(resource->path);
    free(resource);
}

static inline int free_res(void *key, int count, void **value, void *user) {
    cdlv_resource* res = (cdlv_resource*)*value;
    cdlv_resource_free(res);
    return 1;
}

void cdlv_resources_free(cdlv_dict* resources) {
    dic_forEach(resources, free_res, NULL);
    dic_delete(resources);
}