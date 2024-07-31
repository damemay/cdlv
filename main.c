#include "cdlv.h"
#include "resource.h"
#include "scene.h"

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
    }
    printf("%s\n", base->log);
    exit(1);
}

int foreach_res(void *key, int count, void **value, void *user) {
    cdlv_resource* res = (cdlv_resource*)*value;
    printf("\t%.*s => %s [%s]\n", count, (char*)key, res->path, res->loaded ? "loaded" : "unloaded");
}

int foreach_scene(void *key, int count, void **value, void *user) {
    cdlv_scene* scene = (cdlv_scene*)*value;
    printf("%.*s\n", count, (char*)key);
    dic_forEach(scene->resources, foreach_res, NULL);
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG);
    TTF_Init();
    SDL_Event event;
    SDL_Window* _window = SDL_CreateWindow(_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, _width, _height, SDL_WINDOW_SHOWN);
    SDL_Renderer* _renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);

    cdlv base = {0};
    cdlv_init(&base, _width, _height);

    cdlv_config config = {0};
    cdlv_set_config(&base, config);

    cdlv_error result = 0;
    result = cdlv_add_script(&base, "/home/mar/cdlv/scripts/test.cdlv");
    error(&base);

    //if(dic_find(base.resources, "empty", 5)) printf("empty => %s\n", *base.resources->value);
    //else printf("not found\n");

    cdlv_play(&base, _renderer);
    error(&base);

    printf("global resources:\n");
    dic_forEach(base.resources, foreach_res, NULL);
    printf("scenes:\n");
    dic_forEach(base.scenes, foreach_scene, NULL);

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