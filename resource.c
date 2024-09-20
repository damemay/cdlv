#include "resource.h"
#include "util.h"
#include "cdlv.h"
#include "hashdict.c/hashdict.h"

cdlv_error cdlv_resource_new(cdlv* base, const cdlv_resource_type type, char* path, cdlv_resource** resource) {
    cdlv_resource* new_resource = malloc(sizeof(cdlv_resource));
    if(!new_resource) {
        cdlv_logv("Could not allocate memory for resource from file: %s", path);
        cdlv_err(cdlv_callback_error);
    }
    new_resource->type = type;
    new_resource->loaded = false;
    new_resource->path = path;
    new_resource->image = NULL;
    *resource = new_resource;
    cdlv_err(cdlv_ok);
}

static inline cdlv_error cdlv_resource_image_load(cdlv* base, cdlv_resource* resource) {
    if(base->image_config.load_callback)
	resource->image = base->image_config.load_callback(resource->path, base->image_config.user_data);
    if(!resource->image) {
	cdlv_err(cdlv_callback_error);
    }
    cdlv_err(cdlv_ok);
}

static inline cdlv_error cdlv_resource_video_load(cdlv* base, cdlv_resource* resource) {
    resource->video = malloc(sizeof(cdlv_video));
    if(!resource->video) {
        cdlv_log("Could not allocate memory for new video");
        cdlv_err(cdlv_memory_error);
    }
    resource->video->is_playing = false;
    resource->video->loop = false;
    resource->video->eof = false;
#ifdef CDLV_FFMPEG
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
    if(base->video_config.load_callback)
	resource->video->texture = base->video_config.load_callback(resource->video->codec_context->width, resource->video->codec_context->height, base->video_config.user_data);
    if(!resource->video->texture) {
	cdlv_err(cdlv_callback_error);
    }
    resource->video->packet = av_packet_alloc();
    if(!resource->video->packet) {
        cdlv_log("Could not allocate memory for video packet");
        cdlv_err(cdlv_memory_error);
    }
    resource->video->dimensions.x = resource->video->codec_context->width;
    resource->video->dimensions.y = resource->video->codec_context->height;
    resource->video->fps = av_q2d(resource->video->format_context->streams[resource->video->video_stream]->r_frame_rate);
#else
    if(base->video_config.load_callback)
	resource->video->texture = base->video_config.load_callback(0, 0, base->video_config.user_data);
#endif

    cdlv_err(cdlv_ok);
}

cdlv_error cdlv_resource_load(cdlv* base, cdlv_resource* resource) {
    cdlv_error res;
    if(resource->type == cdlv_resource_image) {
        if((res = cdlv_resource_image_load(base, resource)) != cdlv_ok) cdlv_err(res);
    } else if(resource->type == cdlv_resource_video) {
        if((res = cdlv_resource_video_load(base, resource)) != cdlv_ok) cdlv_err(res);
    }
    resource->loaded = true;
    cdlv_logv("loaded resource: %s", resource->path);
    cdlv_err(cdlv_ok);
}

static inline void cdlv_resource_image_unload(cdlv* base, cdlv_resource* resource) {
    if(base->image_config.free_callback)
	base->image_config.free_callback(resource->image);
    resource->image = NULL;
} 

static inline void cdlv_resource_video_unload(cdlv* base, cdlv_resource* resource) {
#ifdef CDLV_FFMPEG
    av_packet_free(&resource->video->packet);
    av_frame_free(&resource->video->frame);
    avcodec_free_context(&resource->video->codec_context);
    avformat_close_input(&resource->video->format_context);
#endif
    if(base->video_config.free_callback)
	base->video_config.free_callback(resource->video->texture);
    free(resource->video);
    resource->video = NULL;
} 

void cdlv_resource_unload(cdlv* base, cdlv_resource* resource) {
    if(resource->type == cdlv_resource_image) cdlv_resource_image_unload(base, resource);
    else if(resource->type == cdlv_resource_video) cdlv_resource_video_unload(base, resource);
    resource->loaded = false;
}

void cdlv_resource_free(cdlv* base, cdlv_resource* resource) {
    if(resource->loaded) cdlv_resource_unload(base, resource);
    free(resource->path);
    free(resource);
}

static inline int free_res(void *key, int count, void **value, void *user) {
    cdlv_resource* res = (cdlv_resource*)*value;
    cdlv* base = (cdlv*)user;
    cdlv_resource_free(base, res);
    return 1;
}

void cdlv_resources_free(cdlv* base, cdlv_dict* resources) {
    dic_forEach(resources, free_res, base);
    dic_delete(resources);
}

cdlv_error cdlv_play_video(cdlv* base, cdlv_video* video) {
#ifdef CDLV_FFMPEG
    int ret = 0;
    if(video->is_playing) {
	if(base->video_config.change_frame_bool) {
	    if((ret = av_read_frame(video->format_context, video->packet)) == AVERROR_EOF) {
    	        if(!video->loop) video->eof = true;
    	        av_seek_frame(video->format_context, video->video_stream, 0, 0);
    	        av_packet_unref(video->packet);
    	        cdlv_err(cdlv_ok);
    	    }
    	    if(video->packet->stream_index == video->video_stream || video->eof) {
    	        if(video->eof) {
    	            if((ret = avcodec_send_packet(video->codec_context, NULL)) < 0) cdlv_log("Nothing to drain");
    	        } else {
    	            if((ret = avcodec_send_packet(video->codec_context, video->packet)) < 0) {
    	                cdlv_log("Could not send packet for decoding");
    	                cdlv_err(cdlv_video_error);
    	            }
    	        }
    	        if(ret >= 0 || video->eof) {
    	            ret = avcodec_receive_frame(video->codec_context, video->frame);
    	            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
    	                cdlv_err(cdlv_ok);
    	            } else if(ret < 0) {
    	                cdlv_log("Could not decode frame");
    	                cdlv_err(cdlv_video_error);
    	            }
    	            //cdlv_logv("Frame %ld/%ld pts %ld dts %ld", video->codec_context->frame_num, video->format_context->streams[video->video_stream]->nb_frames, video->frame->pts, video->frame->pkt_dts);

    	            if (video->frame->linesize[0] > 0 && video->frame->linesize[1] > 0 && video->frame->linesize[2] > 0) {
			video->plane.y = video->frame->data[0];
			video->plane.u = video->frame->data[1];
			video->plane.v = video->frame->data[2];
			video->pitch.y = video->frame->linesize[0];
			video->pitch.u = video->frame->linesize[1];
			video->pitch.v = video->frame->linesize[2];
			if(base->video_config.update_callback)
			    base->video_config.update_callback(video->plane, video->pitch, video->texture, base->video_config.user_data);
    	            } else if (video->frame->linesize[0] < 0 && video->frame->linesize[1] < 0 && video->frame->linesize[2] < 0) {
			video->plane.y = video->frame->data[0] + video->frame->linesize[0] * (video->frame->height- 1);
			video->plane.u = video->frame->data[1] + video->frame->linesize[1] * (AV_CEIL_RSHIFT(video->frame->height, 1) - 1);
			video->plane.v = video->frame->data[2] + video->frame->linesize[2] * (AV_CEIL_RSHIFT(video->frame->height, 1) - 1);
			video->pitch.y = -video->frame->linesize[0];
			video->pitch.u = -video->frame->linesize[1];
			video->pitch.v = -video->frame->linesize[2];
			if(base->video_config.update_callback)
			    base->video_config.update_callback(video->plane, video->pitch, video->texture, base->video_config.user_data);
    	            }
    	            if(video->frame->pts+video->frame->duration == video->format_context->streams[video->video_stream]->duration && video->eof) {
    	                video->is_playing = false;
    	                cdlv_err(cdlv_ok);
    	            }
    	        }
    	    }
    	    av_packet_unref(video->packet);
	}
    }
#else
    if(video->is_playing) {
	if(base->video_config.change_frame_bool) {
    	    if(base->video_config.update_callback)
    		base->video_config.update_callback(video->plane, video->pitch, video->texture, base->video_config.user_data);
	}
    }
#endif
    cdlv_err(cdlv_ok);
}
