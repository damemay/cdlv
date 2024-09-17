#ifndef CDLV_PLAY_H
#define CDLV_PLAY_H

#include "cdlv.h"
#include "util.h"
#include "scene.h"
#include "parse.h"
#include "text.h"
#include "resource.h"

typedef struct {
    uint16_t index;
    cdlv_scene* scene;
} cdlv_scene_searcher;

static inline int find_scene_index(void* key, int count, void** value, void* user) {
    cdlv_scene* scene = (cdlv_scene*)*value;
    cdlv_scene_searcher* arg = (cdlv_scene_searcher*)user;
    if(scene->index == arg->index) {
        arg->scene = scene;
        return 0;
    }
    return 1;
}

static inline cdlv_error cdlv_set_scene(cdlv* base, const uint16_t index, SDL_Renderer* renderer) {
    cdlv_scene_searcher search = {
        .index = index,
        .scene = NULL,
    };
    dic_forEach(base->scenes, find_scene_index, &search);
    if(search.scene) {
        cdlv_error res;
        base->current_scene = search.scene;
        base->current_scene_index = index;
        if((res = cdlv_scene_load(base, search.scene, renderer)) != cdlv_ok) cdlv_err(res);
    } else {
        cdlv_logv("Could not find scene with index: %d", search.index);
        cdlv_err(cdlv_fatal_error);
    }
    cdlv_err(cdlv_ok);
}

static inline cdlv_error cdlv_parse_prompt(cdlv* base, const char* line, cdlv_scene* scene, SDL_Renderer* renderer) {
    cdlv_error res;
    if(strstr(line, cdlv_tag_prompt_bg)) {
        char* name;
        if((res = cdlv_extract_non_quote(base, line, &name)) != cdlv_ok) cdlv_err(res);
        if(strchr(name, '.')) {
            char* vname;
            if((res = cdlv_extract_filename(base, name, &vname)) != cdlv_ok) cdlv_err(res);
            if((res = cdlv_add_new_resource_from_path(base, scene->resources_path, vname, name, scene->resources)) != cdlv_ok) cdlv_err(res);
            if(dic_find(scene->resources, vname, strlen(vname))) {
                cdlv_resource* resource = *scene->resources->value;
                cdlv_resource_load(base, resource, renderer);
                if(resource->type == cdlv_resource_video) {
                    resource->video->is_playing = true;
                    if(strstr(line, cdlv_tag_time_loop)) resource->video->loop = true;
                    else if(strstr(line, cdlv_tag_time_once)) resource->video->loop = false;
                    base->accum = 1.1f;
                }
                base->current_bg = resource;
            }
        } else {
            if(dic_find(base->resources, name, strlen(name))) {
                cdlv_resource* resource = *base->resources->value;
                if(resource->type == cdlv_resource_video) {
                    resource->video->is_playing = true;
                    if(strstr(line, cdlv_tag_time_loop)) resource->video->loop = true;
                    else if(strstr(line, cdlv_tag_time_once)) resource->video->loop = false;
                    base->accum = 1.1f;
                }
                base->current_bg = resource;
            } else if(dic_find(scene->resources, name, strlen(name))) {
                cdlv_resource* resource = *scene->resources->value;
                if(resource->type == cdlv_resource_video) {
                    resource->video->is_playing = true;
                    if(strstr(line, cdlv_tag_time_loop)) resource->video->loop = true;
                    else if(strstr(line, cdlv_tag_time_once)) resource->video->loop = false;
                    base->accum = 1.1f;
                }
                base->current_bg = resource;
            } else {
                cdlv_logv("Prompt to unknown resource: %s", name);
                cdlv_err(cdlv_parse_error);
            }
        }
        free(name);
    }
    cdlv_err(cdlv_ok);
}

static inline cdlv_error cdlv_play_video(cdlv* base, cdlv_video* video, SDL_Renderer* renderer) {
    int ret = 0;
    if(video->is_playing) {
        base->accum += base->elapsed_ticks * video->fps;
        if(base->accum > 1) {
            base->accum = 0;
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
                    printf("Frame %d/%ld pts %ld dts %ld\n", video->codec_context->frame_num, video->format_context->streams[video->video_stream]->nb_frames, video->frame->pts, video->frame->pkt_dts);
                    if (video->frame->linesize[0] > 0 && video->frame->linesize[1] > 0 && video->frame->linesize[2] > 0) {
                        ret = SDL_UpdateYUVTexture(video->texture, NULL, video->frame->data[0], video->frame->linesize[0], video->frame->data[1], video->frame->linesize[1], video->frame->data[2], video->frame->linesize[2]);
                    } else if (video->frame->linesize[0] < 0 && video->frame->linesize[1] < 0 && video->frame->linesize[2] < 0) {
                        ret = SDL_UpdateYUVTexture(video->texture, NULL, video->frame->data[0] + video->frame->linesize[0] * (video->frame->height- 1), -video->frame->linesize[0], video->frame->data[1] + video->frame->linesize[1] * (AV_CEIL_RSHIFT(video->frame->height, 1) - 1), -video->frame->linesize[1], video->frame->data[2] + video->frame->linesize[2] * (AV_CEIL_RSHIFT(video->frame->height, 1) - 1), -video->frame->linesize[2]);
                    }
                    if(video->frame->pts+video->frame->duration == video->format_context->streams[video->video_stream]->duration && video->eof) {
                        video->is_playing = false;
                        if(base->call_stop) cdlv_stop(base);
                        cdlv_err(cdlv_ok);
                    }
                }
            }
            av_packet_unref(video->packet);
        }
    }
    cdlv_err(cdlv_ok);
}

static inline cdlv_error cdlv_render(cdlv* base, SDL_Renderer* renderer) {
    cdlv_resource* bg = (cdlv_resource*)base->current_bg;
    if(!bg) cdlv_err(cdlv_ok);
    if(bg->type == cdlv_resource_image) {
        SDL_RenderCopy(renderer, bg->image, NULL, NULL);
    } else if(bg->type == cdlv_resource_video) {
        cdlv_error res;
        if((res = cdlv_play_video(base, bg->video, renderer)) != cdlv_ok) cdlv_err(res);
        SDL_RenderCopy(renderer, bg->video->texture, NULL, NULL);
    }
    if(base->can_interact && base->is_playing) cdlv_text_render(base, renderer);
    cdlv_err(cdlv_ok);
}

static inline cdlv_error cdlv_parse_scene_line(cdlv* base, cdlv_scene* scene, SDL_Renderer* renderer) {
    if(scene->script->size == 0) {
        cdlv_log("Scene's script is empty");
        cdlv_err(cdlv_parse_error);
    }
    cdlv_error res;
    if(base->current_line == scene->script->size) {
	puts("At the end of script");
        if(base->current_scene_index+1 < base->scene_count) {
	    puts("Changing scene");
            if((res = cdlv_set_scene(base, base->current_scene_index+1, renderer)) != cdlv_ok) cdlv_err(res);
	    cdlv_scene* new_scene = (cdlv_scene*)base->current_scene;
	    if(new_scene->loaded) {
		base->current_line = 0;
		if((res = cdlv_parse_scene_line(base, new_scene, renderer)) != cdlv_ok) cdlv_err(res);
	    }
        } else {
	    puts("Calling stop");
            if(((cdlv_resource*)base->current_bg)->video->is_playing) base->call_stop = true;
            else cdlv_stop(base);
        }
    } else if(base->current_line < scene->script->size) {
        char* line = SCL_ARRAY_GET(scene->script, base->current_line, char*);
        if(!line) cdlv_err(cdlv_fatal_error);
        if(line[0] == '@') {
            if((res = cdlv_parse_prompt(base, line, scene, renderer)) != cdlv_ok) cdlv_err(res);
            ++base->current_line;
            if((res = cdlv_parse_scene_line(base, scene, renderer)) != cdlv_ok) cdlv_err(res);
        } else {
            cdlv_text_update(base, line);
        }
    }
    cdlv_err(cdlv_ok);
}

static inline cdlv_error cdlv_user_update(cdlv* base, SDL_Renderer* renderer) {
    cdlv_text* text = (cdlv_text*)base->text;
    if(text->current_char != text->content_size) {
        strcpy(text->rendered, text->content);
        text->current_char = text->content_size;
        cdlv_err(cdlv_ok);
    }

    cdlv_error res;
    cdlv_scene* scene = (cdlv_scene*)base->current_scene;
    ++base->current_line;
    if((res = cdlv_parse_scene_line(base, scene, renderer)) != cdlv_ok) cdlv_err(res);
    cdlv_err(cdlv_ok);
}

static inline cdlv_error cdlv_key_handler(cdlv* base, SDL_Renderer* renderer, SDL_Event event) {
    if(!base->can_interact) cdlv_err(cdlv_ok);
    cdlv_error res;
    if(event.type == SDL_KEYUP && event.key.repeat == 0) {
        switch(event.key.keysym.sym) {
                case SDLK_RETURN:
                    if((res = cdlv_user_update(base, renderer)) != cdlv_ok) cdlv_err(res);
                    break;
        }
    }
    if(event.type == SDL_CONTROLLERBUTTONUP) {
        switch(event.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_A:
                    if((res = cdlv_user_update(base, renderer)) != cdlv_ok) cdlv_err(res);
                    break;
        }
    }
    cdlv_err(cdlv_ok);
}

static inline cdlv_error cdlv_play_loop(cdlv* base, SDL_Renderer* renderer) {
    if(!base->is_playing) cdlv_err(cdlv_ok);
    cdlv_error res;
    base->current_tick = SDL_GetTicks64();
    base->elapsed_ticks = (base->current_tick - base->last_tick) / 1000.0f;
    if(!base->current_scene) {
        if((res = cdlv_set_scene(base, 0, renderer)) != cdlv_ok) cdlv_err(res);
        cdlv_scene* scene = (cdlv_scene*)base->current_scene;
        if((res = cdlv_parse_scene_line(base, scene, renderer)) != cdlv_ok) cdlv_err(res);
    }
    if((res = cdlv_render(base, renderer)) != cdlv_ok) cdlv_err(res);
    base->last_tick = base->current_tick;
    cdlv_err(cdlv_ok);
}

#endif
