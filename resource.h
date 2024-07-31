#ifndef CDLV_RESOURCE_H
#define CDLV_RESOURCE_H

#include "cdlv.h"

typedef enum {
    cdlv_resource_image,
    cdlv_resource_video,
} cdlv_resource_type;

typedef struct cdlv_resource {
    cdlv_resource_type type;
    bool loaded;
    char* path;
    SDL_Texture* image;
} cdlv_resource;

cdlv_error cdlv_resource_new(cdlv* base, const cdlv_resource_type type, char* path, cdlv_resource** resource);
cdlv_error cdlv_resource_load(cdlv* base, cdlv_resource* resource, SDL_Renderer* renderer);
cdlv_error cdlv_resource_unload(cdlv_resource* resource);
void cdlv_resource_free(cdlv_resource* resource);
int cdlv_resources_free(cdlv_dict* resources);

#endif