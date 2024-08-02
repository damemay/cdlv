#ifndef CDLV_PLAY_H
#define CDLV_PLAY_H

#include "cdlv.h"
#include "util.h"
#include "scene.h"
#include "parse.h"

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
                base->current_bg = resource;
            }
        } else {
            if(dic_find(base->resources, name, strlen(name))) {
                cdlv_resource* resource = *base->resources->value;
                base->current_bg = resource;
            } else if(dic_find(scene->resources, name, strlen(name))) {
                cdlv_resource* resource = *scene->resources->value;
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
    bool loop = true;
    bool is_playing = true;
    if(is_playing) {
        if((ret = av_read_frame(video->format_context, video->packet)) == AVERROR_EOF) {
            if(!loop) {
                is_playing = false;
                cdlv_err(cdlv_ok);
            }
            av_seek_frame(video->format_context, video->video_stream, 0, 0);
            av_packet_unref(video->packet);
            cdlv_err(cdlv_ok);
        }
        if(video->packet->stream_index == video->video_stream) {
            if((ret = avcodec_send_packet(video->codec_context, video->packet)) < 0) {
                cdlv_log("Could not send packet for decoding");
                cdlv_err(cdlv_video_error);
            }
            while(ret >= 0) {
                ret = avcodec_receive_frame(video->codec_context, video->frame);
                if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    is_playing = false;
                    cdlv_err(cdlv_ok);
                } else if(ret < 0) {
                    cdlv_log("Could not decode frame");
                    cdlv_err(cdlv_video_error);
                }
                sws_scale(video->sws_context, (uint8_t const* const*)video->frame->data, video->frame->linesize, 0, video->codec_context->height, video->picture->data, video->picture->linesize);
                SDL_Delay((1000*video->sleep_time)-10);
                SDL_UpdateYUVTexture(video->texture, &video->rect, video->picture->data[0], video->picture->linesize[0], video->picture->data[1], video->picture->linesize[1], video->picture->data[2], video->picture->linesize[2]);
                SDL_RenderCopy(renderer, video->texture, NULL, NULL);
            }
        }
        av_packet_unref(video->packet);
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
    }
    cdlv_err(cdlv_ok);
}

static inline cdlv_error cdlv_parse_scene_line(cdlv* base, cdlv_scene* scene, SDL_Renderer* renderer) {
    if(scene->script->size == 0) {
        cdlv_log("Scene's script is empty");
        cdlv_err(cdlv_parse_error);
    }
    cdlv_error res;
    if(base->current_line == scene->script->size-1) {
        if((res = cdlv_set_scene(base, base->current_scene_index+1, renderer)) != cdlv_ok) cdlv_err(res);
    } else if(base->current_line < scene->script->size) {
        char* line = SCL_ARRAY_GET(scene->script, base->current_line, char*);
        printf("%s\n",line);
        if(line[0] == '@') {
            if((res = cdlv_parse_prompt(base, line, scene, renderer)) != cdlv_ok) cdlv_err(res);
            ++base->current_line;
            if((res = cdlv_parse_scene_line(base, scene, renderer)) != cdlv_ok) cdlv_err(res);
        } // else cdlv_text_update
    }
    cdlv_err(cdlv_ok);
}

static inline cdlv_error cdlv_key_handler(cdlv* base, SDL_Renderer* renderer, SDL_Event event) {
    cdlv_error res;
    cdlv_scene* scene = (cdlv_scene*)base->current_scene;
    if(event.type == SDL_KEYUP && event.key.repeat == 0) {
        switch(event.key.keysym.sym) {
                case SDLK_RETURN:
                    ++base->current_line;
                    puts("registered");
                    if((res = cdlv_parse_scene_line(base, scene, renderer)) != cdlv_ok) cdlv_err(res);
                    break;
        }
    }
    if(event.type == SDL_CONTROLLERBUTTONUP) {
        switch(event.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_A:
                    if((res = cdlv_parse_scene_line(base, scene, renderer)) != cdlv_ok) cdlv_err(res);
                    break;
        }
    }
    cdlv_err(cdlv_ok);
}

static inline cdlv_error cdlv_play_loop(cdlv* base, SDL_Renderer* renderer) {
    cdlv_error res;
    if(!base->current_scene) {
        if((res = cdlv_set_scene(base, 0, renderer)) != cdlv_ok) cdlv_err(res);
        cdlv_scene* scene = (cdlv_scene*)base->current_scene;
        if((res = cdlv_parse_scene_line(base, scene, renderer)) != cdlv_ok) cdlv_err(res);
    }
    if((res = cdlv_render(base, renderer)) != cdlv_ok) cdlv_err(res);
    cdlv_err(cdlv_ok);
}

#endif