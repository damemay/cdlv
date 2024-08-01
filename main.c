#include "cdlv.h"
#include "resource.h"
#include "scene.h"

#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#define _width 960
#define _height 544
#define _title "cdlv2"

void error(cdlv* base) {
    if(base->error == cdlv_ok) return;
    printf("%d\n", base->error);
    switch(base->error) {
        case cdlv_ok: printf("CDLV OK\n"); break;
        case cdlv_memory_error: printf("CDLV Memory error: "); break;
        case cdlv_file_error: printf("CDLV File error: "); break;
        case cdlv_read_error: printf("CDLV Read error: "); break;
        case cdlv_parse_error: printf("CDLV Parse error: "); break;
        case cdlv_video_error: printf("CDLV Video error: "); break;
    }
    printf("%s\n", base->log);
    exit(1);
}

int foreach_res(void *key, int count, void **value, void *user) {
    cdlv_resource* res = (cdlv_resource*)*value;
    printf("\t%.*s => %s [%s]\n", count, (char*)key, res->path, res->loaded ? "loaded" : "unloaded");
    return 1;
}

int foreach_scene(void *key, int count, void **value, void *user) {
    cdlv_scene* scene = (cdlv_scene*)*value;
    printf("%.*s\n", count, (char*)key);
    dic_forEach(scene->resources, foreach_res, NULL);
    return 1;
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG);
    TTF_Init();
    SDL_Event event;
    SDL_Window* _window = SDL_CreateWindow(_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, _width, _height, SDL_WINDOW_SHOWN);
    SDL_Renderer* _renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_TARGETTEXTURE);

    cdlv base = {0};
    cdlv_init(&base, _width, _height);

    cdlv_config config = {0};
    cdlv_set_config(&base, config);

    cdlv_add_script(&base, "../res/sample/sample.cdlv");
    error(&base);

    //if(dic_find(base.resources, "empty", 5)) printf("empty => %s\n", *base.resources->value);
    //else printf("not found\n");

    cdlv_play(&base, _renderer);
    error(&base);

    printf("global resources:\n");
    dic_forEach(base.resources, foreach_res, NULL);
    printf("scenes:\n");
    dic_forEach(base.scenes, foreach_scene, NULL);

    cdlv_resource r = {
        .path = "/home/mar/cdlv/scripts/idol/last1.mp4",
        .type = cdlv_resource_video,
    };
    cdlv_resource_load(&base, &r, _renderer);
    error(&base);
    int ret = 0;
    r.video->loop = false;
    r.video->is_playing = true;
    while(r.video->is_playing) {
        if((ret = av_read_frame(r.video->format_context, r.video->packet)) == AVERROR_EOF) {
            if(!r.video->loop) {
                r.video->is_playing = false;
                break;
            }
            av_seek_frame(r.video->format_context, r.video->video_stream, 0, 0);
            av_packet_unref(r.video->packet);
            continue;
        }
        if(r.video->packet->stream_index == r.video->video_stream) {
            if((ret = avcodec_send_packet(r.video->codec_context, r.video->packet)) < 0) {
                char buf[cdlv_max_string_size];
                av_strerror(ret, buf, cdlv_max_string_size);
                puts(buf);
                puts("error sending packet for decoding"), exit(1);
            }
            while(ret >= 0) {
                ret = avcodec_receive_frame(r.video->codec_context,r.video->frame);
                if(ret==AVERROR(EAGAIN)||ret==AVERROR_EOF) break;
                else if(ret<0) puts("error while decoding"), exit(1);
                sws_scale(r.video->sws_context, (uint8_t const* const*)r.video->frame->data, r.video->frame->linesize, 0, r.video->codec_context->height, r.video->picture->data, r.video->picture->linesize);
                SDL_Delay((1000*r.video->sleep_time)-10);
                SDL_UpdateYUVTexture(r.video->texture, &r.video->rect, r.video->picture->data[0], r.video->picture->linesize[0], r.video->picture->data[1], r.video->picture->linesize[1], r.video->picture->data[2], r.video->picture->linesize[2]);
                SDL_RenderClear(_renderer);
                SDL_RenderCopy(_renderer, r.video->texture, NULL, NULL);
                SDL_RenderPresent(_renderer);
            }
        }
        av_packet_unref(r.video->packet);
    }
    cdlv_resource_unload(&r);

    bool _running = true;
    while(_running) {
        while(SDL_PollEvent(&event) != 0) {
            if(event.type == SDL_QUIT) {
                _running = false;
                break;
            }
        }
        SDL_RenderClear(_renderer);
        if(base.is_playing) cdlv_loop(&base, _renderer);
        SDL_RenderPresent(_renderer);
    }

    cdlv_stop(&base);
    cdlv_free(&base);

    SDL_DestroyRenderer(_renderer);
    SDL_DestroyWindow(_window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}