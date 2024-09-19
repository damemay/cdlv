#ifndef CDLV_RESOURCE_H
#define CDLV_RESOURCE_H

#include "cdlv.h"

cdlv_error cdlv_resource_new(cdlv* base, const cdlv_resource_type type, char* path, cdlv_resource** resource);
cdlv_error cdlv_resource_load(cdlv* base, cdlv_resource* resource);
void cdlv_resource_unload(cdlv* base, cdlv_resource* resource);
void cdlv_resource_free(cdlv* base, cdlv_resource* resource);
void cdlv_resources_free(cdlv* base, cdlv_dict* resources);

#endif
