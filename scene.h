#ifndef CDLV_SCENE_H
#define CDLV_SCENE_H

#include "common.h"
#include "array.h"
#include "cdlv.h"

typedef struct cdlv_scene {
    uint16_t index;
    bool loaded;
    char* resources_path;
    cdlv_dict* resources;
    scl_array* script;
} cdlv_scene;

cdlv_error cdlv_scene_new(cdlv* base, const char* resource_path, cdlv_scene** scene);
cdlv_error cdlv_scene_load(cdlv* base, cdlv_scene* scene);
void cdlv_scene_unload(cdlv* base, cdlv_scene* scene);
void cdlv_scene_free(cdlv* base, cdlv_scene* scene);
void cdlv_scenes_free(cdlv* base, cdlv_dict* scenes);

#endif
