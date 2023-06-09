#include "cdlv.h"

#define WIDTH  960
#define HEIGHT 544
#define TITLE  "cdlv"

#ifdef __vita__
extern unsigned int _newlib_heap_size_user = 300*1024*1024;
#endif

int main(int argc, char* argv[]) {
    cdlv_base* base = cdlv_create(TITLE, WIDTH, HEIGHT);
    cdlv_menu* menu = NULL;

    #ifndef __vita__
    menu = cdlv_menu_create(base, "/home/mar/scripts");
    #else
    menu = cdlv_menu_create(base, "ux0:/data/scripts");
    #endif

    while(base->run) {
        switch(base->state) {
            case cdlv_main_menu:
                while(SDL_PollEvent(&base->event) != 0) {
                    if(base->event.type == SDL_QUIT) base->run = false;
                    cdlv_menu_handle_keys(&base, &menu, &base->event);
                }
                cdlv_menu_render(base);
                break;
            case cdlv_main_run:
                cdlv_loop_start(base);
                cdlv_render(base);
                break;
        }
        cdlv_loop_end(base);
    }

    cdlv_clean_all(base);
    return EXIT_SUCCESS;
}
