#ifndef CDLV_PARSE_H
#define CDLV_PARSE_H

#include "cdlv.h"

#define cdlv_tag_define_resources_path "!resources_path"
#define cdlv_tag_define_resources "!resources"
#define cdlv_tag_define_scene "!scene"

#define cdlv_tag_prompt_bg "@bg"
#define cdlv_tag_time_loop " loop"
#define cdlv_tag_time_once " once"

#define cdlv_tag_opening "{"
#define cdlv_tag_closure "}"

typedef enum {
    cdlv_parse_global,
    cdlv_parse_scene,
    cdlv_parse_global_resources,
    cdlv_parse_scene_resources,
} cdlv_parse_mode;

cdlv_error cdlv_parse_script(cdlv* base, char* script);

#endif