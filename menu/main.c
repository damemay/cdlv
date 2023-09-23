#include "../cdlv.h"
#include "cdlv_menu.h"

#define WIDTH  960
#define HEIGHT 544
#define TITLE  "cdlv"

#ifdef __vita__
extern unsigned int _newlib_heap_size_user = 300*1024*1024;
#endif

int main(int argc, char* argv[]) {
    sdl_base* sdl = sdl_create(TITLE, WIDTH, HEIGHT);
    cdlv_config config = {
        .text_wrap = 1200,
        .text_x = 50,
        .text_y = 400,
        .text_color_r = 255,
        .text_color_g = 255,
        .text_color_b = 255,
        .text_color_a = 255,
    };
    cdlv_base* base = cdlv_create(&config);
    cdlv_menu* menu = NULL;

    #ifndef __vita__
    menu = cdlv_menu_create(base, "/home/mar/c/sdl_gl/cdlv/scripts", sdl);
    #else
    menu = cdlv_menu_create(base, "ux0:/data/scripts", sdl->renderer);
    #endif

    while(sdl->run) {
        switch(base->state) {
            case cdlv_main_menu:
                while(SDL_PollEvent(&sdl->event) != 0) {
                    if(sdl->event.type == SDL_QUIT) sdl->run = false;
                    cdlv_menu_handle_keys(&base, &menu, sdl);
                }
                cdlv_menu_render(base, sdl);
                break;
            case cdlv_main_run:
                cdlv_loop_start(base, &sdl->event, &sdl->run);
                cdlv_render(base, &sdl->renderer);
                break;
        }
        cdlv_loop_end(base, &sdl->renderer);
    }

    cdlv_clean_all(base);
    return EXIT_SUCCESS;
}
