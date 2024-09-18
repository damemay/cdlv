#include "cdlv.h"
#include "resource.h"
#include "scene.h"

#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#define _width 960
#define _height 544
#define _title "cdlv2 sample"

void error(cdlv* base) {
    if(base->error == cdlv_ok) return;
    switch(base->error) {
        case cdlv_ok: printf("CDLV OK\n"); break;
        case cdlv_memory_error: printf("CDLV Memory error: "); break;
        case cdlv_file_error: printf("CDLV File error: "); break;
        case cdlv_read_error: printf("CDLV Read error: "); break;
        case cdlv_parse_error: printf("CDLV Parse error: "); break;
        case cdlv_video_error: printf("CDLV Video error: "); break;
        case cdlv_fatal_error: printf("CDLV Fatal error: "); break;
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
    printf("%d - %.*s\n", scene->index, count, (char*)key);
    dic_forEach(scene->resources, foreach_res, NULL);
    return 1;
}

int main(int argc, char* argv[]) {
    char script_path[cdlv_max_string_size];
    if(argc < 2) {
	strcpy(script_path, "../res/sample/sample.cdlv");
    } else {
	if(strstr(argv[1], ".cdlv")) strncpy(script_path, argv[1], cdlv_max_string_size-1);
	else {
	    fprintf(stderr, "Usage: %s [script]\r\n [script]\tpath to .cdlv file. defaults to ../res/sample/sample.cdlv\r\n", argv[0]);
	    exit(1);
	}
    }

    // Initialize your SDL2, SDL2_image, SDL2_ttf:
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG);
    TTF_Init();
    SDL_Event event;
    SDL_Window* _window = SDL_CreateWindow(_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, _width, _height, SDL_WINDOW_SHOWN);
    SDL_Renderer* _renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_TARGETTEXTURE);

    // Initialize CDLV "base":
    cdlv base = {0};
    cdlv_init(&base, _width, _height);

    // Setup your config. It can be empty for defaults.
    cdlv_config config = {0};
    cdlv_set_config(&base, config);

    // Add script - it will load in all text data and create structures.
    cdlv_add_script(&base, script_path);
    error(&base);

    // cdlv_play loads global resources and first scene's resources.
    // Sets cdlv.is_playing to true.
    cdlv_play(&base, _renderer);
    error(&base);

    printf("global resources:\n");
    dic_forEach(base.resources, foreach_res, NULL);
    printf("scenes:\n");
    dic_forEach(base.scenes, foreach_scene, NULL);

    bool _running = true;
    while(_running&&base.is_playing) {
        while(SDL_PollEvent(&event) != 0) {
            if(event.type == SDL_QUIT) {
                _running = false;
                break;
            }
            // Add cdlv_event into your event polling loop for keyhandling:
            if(base.is_playing) {
                cdlv_event(&base, _renderer, event);
                error(&base);
            }
        }
        SDL_RenderClear(_renderer);
        // Add cdlv_loop between your SDL_RenderClear and SDL_RenderPresent:
        if(base.is_playing) {
            cdlv_loop(&base, _renderer);
            error(&base);
        }
        SDL_RenderPresent(_renderer);
    }

    // cdlv_free will free all data and structures of cdlv.
    // Use it only when you're done with cdlv in program.
    // If you want to change game state from CDLV to something else,
    // it's better to use cdlv_stop(cdlv*).
    cdlv_free(&base);

    SDL_DestroyRenderer(_renderer);
    SDL_DestroyWindow(_window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}
