#include "resource.h"
#include "util.h"

cdlv_error cdlv_resource_new(cdlv* base, const cdlv_resource_type type, char* path, cdlv_resource** resource) {
    cdlv_resource* new_resource = malloc(sizeof(cdlv_resource));
    if(!new_resource) {
        cdlv_logv("Could not allocate memory for resource from file: %s", path);
        cdlv_err(cdlv_memory_error);
    }
    new_resource->type = type;
    new_resource->loaded = false;
    new_resource->path = path;
    new_resource->image = NULL;
    *resource = new_resource;
    cdlv_err(cdlv_ok);
}

void cdlv_resource_free(cdlv_resource* resource) {
    if(resource->loaded) {
        // free image
    }
    free(resource->path);
    free(resource);
}

static inline int free_res(void *key, int count, void **value, void *user) {
    cdlv_resource* res = (cdlv_resource*)*value;
    cdlv_resource_free(res);
}

int cdlv_resources_free(cdlv_dict* resources) {
    dic_forEach(resources, free_res, NULL);
    dic_delete(resources);
}