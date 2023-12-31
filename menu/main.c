#include "../cdlv.h"
#include "cdlv_menu.h"
#include "../zocket/zocket.h"
#include <stdlib.h>

#define WIDTH  960
#define HEIGHT 544
#define TITLE  "cdlv"

#ifdef __vita__
extern unsigned int _newlib_heap_size_user = 300*1024*1024;
#endif

int main(int argc, char* argv[]) {
    if(argc!=2) {
        puts("cdlv-menu [config path]");
        return EXIT_FAILURE;
    }

    sdl_base* sdl = sdl_create(TITLE, WIDTH, HEIGHT);

    cdlv_config config = {0};
    if(cdlv_config_from_file(&config, argv[1]) == cdlv_config_err) return EXIT_FAILURE;

    zkt_client* client = zkt_client_init(config.host, config.port);
    if(!client) return EXIT_FAILURE;

    cdlv_base* base = cdlv_create(&config);
    if(!base) return EXIT_FAILURE;

    cdlv_menu* menu = NULL;

    #ifndef __vita__
    menu = cdlv_menu_create(base, config.host, sdl, client->fd);
    #else
    menu = cdlv_menu_create(base, "ux0:/data/scripts", sdl->renderer);
    #endif

    while(sdl->run) {
        switch(base->state) {
            case cdlv_main_menu:
                while(SDL_PollEvent(&sdl->event) != 0) {
                    if(sdl->event.type == SDL_QUIT) sdl->run = false;
                    cdlv_menu_handle_keys(&base, &menu, sdl, &client);
                }
                base->c_tick = SDL_GetTicks64();
                base->e_ticks = (base->c_tick - base->l_tick) / 1000.0f;
                cdlv_menu_render(base, sdl);
                break;
            case cdlv_main_run:
                if(check_errors(base, base->error) == 0) cdlv_loop_start(base, &sdl->event, &sdl->run, &client);
                else while(SDL_PollEvent(&sdl->event) != 0) if(sdl->event.type == SDL_QUIT) sdl->run = false;
                cdlv_render(base, &sdl->renderer, &client);
                break;
        }
        cdlv_loop_end(base, &sdl->renderer);
    }

    zkt_client_clean(client);
    cdlv_clean_all(base);
    sdl_clean(sdl);
    return EXIT_SUCCESS;
}
