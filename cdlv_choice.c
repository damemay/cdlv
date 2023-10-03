#include "cdlv.h"
#include "cdlv_util.h"

int cdlv_choice_create(cdlv_base* base) {
    base->choice = malloc(sizeof(cdlv_choice));
    if(!base->choice) {
        cdlv_log("Could not allocate memory for a new scripted choice!");
        return -1;
    }

    base->choice->count               = 0;
    base->choice->current             = 0;
    base->choice->state               = cdlv_parsing;
    base->choice->destinations        = NULL;
    base->choice->options             = NULL;
    base->choice->prompt              = NULL;
    cdlv_alloc_ptr_arr(&base->choice->destinations, cdlv_max_choice_count, size_t);
    cdlv_alloc_ptr_arr(&base->choice->options, cdlv_max_choice_count, char*);
    cdlv_alloc_ptr_arr(&base->choice->prompt, cdlv_max_string_size, char);
    
    return 0;
}

static inline void cdlv_choice_clean(cdlv_base* base) {
    for(size_t i=0;i<cdlv_max_choice_count; ++i)
        if(base->choice->options[i]) free(base->choice->options[i]);
    free(base->choice->options);
    free(base->choice->prompt);
    free(base->choice->destinations);
    free(base->choice);
    base->choice = NULL;
}

void cdlv_choice_add(cdlv_base* base, const char* line) {
    const char chars[12] = "1234567890 ";
    size_t d = 0;
    size_t s = 0;
    if((sscanf(line, "%lu", &d)) != 0)
        if(d < base->scene_count)
            s = strspn(line, chars);
    if(s>0 && base->choice->count < cdlv_max_choice_count) {
        base->choice->destinations[base->choice->count] = d;
        cdlv_duplicate_string(&base->choice->options[base->choice->count], line+s, strlen(line+s)+1);
        ++base->choice->count;
    }
}

static inline bool cdlv_choice_switch(cdlv_base* base, size_t ch) {
    if(base->choice->destinations[ch]) {
        cdlv_scene_load(base, base->c_scene, base->choice->destinations[ch]);
        return true;
    }
    return false;
}

void cdlv_choice_handler(cdlv_base* base, size_t ch) {
    if(cdlv_choice_switch(base, ch)) cdlv_choice_clean(base);
}

