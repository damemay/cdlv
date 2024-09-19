#include "scene.h"
#include "util.h"
#include "resource.h"
#include "hashdict.c/hashdict.h"

cdlv_error cdlv_scene_new(cdlv* base, const char* resource_path, cdlv_scene** scene) {
    cdlv_scene* new_scene = malloc(sizeof(cdlv_scene));
    if(!new_scene) {
        cdlv_log("Could not allocate memory for new scene");
        cdlv_err(cdlv_memory_error);
    }
    cdlv_strdup(&new_scene->resources_path, resource_path, strlen(resource_path)+1);
    new_scene->resources = dic_new(0);
    new_scene->loaded = 0;
    new_scene->script = malloc(sizeof(scl_array));
    if(!new_scene->script) {
        cdlv_log("Could not allocate memory for new scene's script");
        cdlv_err(cdlv_memory_error);
    }
    scl_array_init(new_scene->script, 128, sizeof(char*));
    *scene = new_scene;
    cdlv_err(cdlv_ok);
}

static inline int load_resources(void *key, int count, void **value, void *user) {
    cdlv_resource* resource = (cdlv_resource*)*value;
    cdlv* base = (cdlv*)user;
    cdlv_resource_load(base, resource);
    return 1;
}

static inline int unload_resources(void *key, int count, void **value, void *user) {
    cdlv_resource* resource = (cdlv_resource*)*value;
    cdlv* base = (cdlv*)user;
    cdlv_resource_unload(base, resource);
    return 1;
}

cdlv_error cdlv_scene_load(cdlv* base, cdlv_scene* scene) {
    dic_forEach(scene->resources, load_resources, base);
    scene->loaded = true;
    cdlv_err(cdlv_ok);
}

void cdlv_scene_unload(cdlv* base, cdlv_scene* scene) {
    dic_forEach(scene->resources, unload_resources, base);
    scene->loaded = false;
}

void cdlv_scene_free(cdlv* base, cdlv_scene* scene) {
    if(scene->loaded) cdlv_scene_unload(base, scene);
    free(scene->resources_path);
    cdlv_resources_free(base, scene->resources);
    for(size_t i=0; i<scene->script->size; i++) {
        char* line = SCL_ARRAY_GET(scene->script, i, char*);
        free(line);
    }
    scl_array_free(scene->script);
    free(scene->script);
    free(scene);
}

static inline int free_scene(void *key, int count, void **value, void *user) {
    cdlv_scene* scene = (cdlv_scene*)*value;
    cdlv* base = (cdlv*)user;
    cdlv_scene_free(base, scene);
    return 1;
}

void cdlv_scenes_free(cdlv* base, cdlv_dict* scenes) {
    dic_forEach(scenes, free_scene, base);
    dic_delete(scenes);
}
