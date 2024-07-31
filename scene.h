#ifndef CDLV_SCENE_H
#define CDLV_SCENE_H

#include "common.h"
#include "array.h"
#include "util.h"

typedef struct cdlv_scene {
    char* resources_path;
    cdlv_dict* resources;
    scl_array* script;
} cdlv_scene;

cdlv_error cdlv_scene_new(cdlv* base, const char* resource_path, cdlv_scene** scene);
void cdlv_scene_free(cdlv_scene* scene);
int cdlv_scenes_free(cdlv_dict* scenes);

#endif