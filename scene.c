#include "scene.h"
#include "util.h"
#include "resource.h"

cdlv_error cdlv_scene_new(cdlv* base, const char* resource_path, cdlv_scene** scene) {
    cdlv_scene* new_scene = malloc(sizeof(cdlv_scene));
    if(!new_scene) {
        cdlv_log("Could not allocate memory for new scene");
        cdlv_err(cdlv_memory_error);
    }
    cdlv_strdup(&new_scene->resources_path, resource_path, strlen(resource_path)+1);
    new_scene->resources = dic_new(0);
    new_scene->script = malloc(sizeof(scl_array));
    if(!new_scene->script) {
        cdlv_log("Could not allocate memory for new scene's script");
        cdlv_err(cdlv_memory_error);
    }
    int res = scl_array_init(new_scene->script, 128, sizeof(char*));
    *scene = new_scene;
    cdlv_err(cdlv_ok);
}

void cdlv_scene_free(cdlv_scene* scene) {
    free(scene->resources_path);
    cdlv_resources_free(scene->resources);
    scl_array_free(scene->script);
    free(scene->script);
    free(scene);
}

static inline int free_scene(void *key, int count, void **value, void *user) {
    cdlv_scene* scene = (cdlv_scene*)*value;
    cdlv_scene_free(scene);
}

int cdlv_scenes_free(cdlv_dict* scenes) {
    dic_forEach(scenes, free_scene, NULL);
    dic_delete(scenes);
}