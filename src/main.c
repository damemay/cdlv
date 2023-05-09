#include "cdlv.h"

#define WIDTH  960
#define HEIGHT 544
#define TITLE  "cdlv"

int main(int argc, char* argv[]) {
    cdlv_base* base = cdlv_create(TITLE, WIDTH, HEIGHT);
    cdlv_read_file(base, "../0.adv");
    cdlv_start(base);

    // for(size_t i=0; i<base->scene_count; ++i) cdlv_scene_info(base, i);
    while(base->run) {
        cdlv_loop_start(base);
        cdlv_render(base);
        cdlv_loop_end(base);
    }

    cdlv_clean(base);
    return EXIT_SUCCESS;
}
