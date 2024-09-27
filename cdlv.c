#include "cdlv.h"
#include "parse.h"
#include "util.h"
#include "resource.h"
#include "scene.h"

static inline cdlv_error extract_path(cdlv* base, const char* path) {
    char* sl = strrchr(path, '/');
    if(!sl) {
        sl = strrchr(path, '\\');
        if(!sl) {
            cdlv_logv("Detected improper script file path: %s", path);
            cdlv_err(cdlv_file_error);
        }
    }
    char basepath[sl-path+2];
    sprintf(basepath, "%.*s", (int)(sl-path+1), path);
    cdlv_strdup(&base->resources_path, basepath, sl-path+2);
    cdlv_err(cdlv_ok);
}

static inline int load_global_resources(char *key, void *value, void *user) {
    cdlv_resource* resource = (cdlv_resource*)value;
    cdlv* base = (cdlv*)user;
    cdlv_resource_load(base, resource);
    return 1;
}

typedef struct {
    uint16_t index;
    cdlv_scene* scene;
} cdlv_scene_searcher;

static inline int find_scene_index(char* key, void* value, void* user) {
    cdlv_scene* scene = (cdlv_scene*)value;
    cdlv_scene_searcher* arg = (cdlv_scene_searcher*)user;
    if(scene->index == arg->index) {
        arg->scene = scene;
        return 0;
    }
    return 1;
}

static inline cdlv_error cdlv_set_scene(cdlv* base, const uint16_t index) {
    cdlv_scene_searcher search = {
        .index = index,
        .scene = NULL,
    };
    dic_forEach(base->scenes, find_scene_index, &search);
    if(search.scene) {
        cdlv_error res;
        base->current_scene = search.scene;
        base->current_scene_index = index;
        if((res = cdlv_scene_load(base, search.scene)) != cdlv_ok) cdlv_err(res);
    } else {
        cdlv_logv("Could not find scene with index: %d", search.index);
        cdlv_err(cdlv_fatal_error);
    }
    cdlv_err(cdlv_ok);
}

static inline cdlv_error cdlv_parse_prompt(cdlv* base, const char* line, cdlv_scene* scene) {
    cdlv_error res;
    void* find;
    if(strstr(line, cdlv_tag_prompt_bg)) {
        char* name;
        if((res = cdlv_extract_non_quote(base, line, &name)) != cdlv_ok) cdlv_err(res);
        if(strchr(name, '.')) {
            char* vname;
            if((res = cdlv_extract_filename(base, name, &vname)) != cdlv_ok) cdlv_err(res);
            if((res = cdlv_add_new_resource_from_path(base, scene->resources_path, vname, name, scene->resources)) != cdlv_ok) cdlv_err(res);
            if((find = sdic_get(scene->resources, vname))) {
                cdlv_resource* resource = (cdlv_resource*)find;
                cdlv_resource_load(base, resource);
                if(resource->type == cdlv_resource_video) {
                    resource->video->is_playing = true;
                    if(strstr(line, cdlv_tag_time_loop)) resource->video->loop = true;
                    else if(strstr(line, cdlv_tag_time_once)) resource->video->loop = false;
                    // base->accum = 1.1f;
                }
                base->current_bg = resource;
            }
        } else {
            if((find = sdic_get(base->resources, name))) {
                cdlv_resource* resource = (cdlv_resource*)find;
                if(resource->type == cdlv_resource_video) {
                    resource->video->is_playing = true;
                    if(strstr(line, cdlv_tag_time_loop)) resource->video->loop = true;
                    else if(strstr(line, cdlv_tag_time_once)) resource->video->loop = false;
                    // base->accum = 1.1f;
                }
                base->current_bg = resource;
            } else if((find = sdic_get(scene->resources, name))) {
                cdlv_resource* resource = (cdlv_resource*)find;
                if(resource->type == cdlv_resource_video) {
                    resource->video->is_playing = true;
                    if(strstr(line, cdlv_tag_time_loop)) resource->video->loop = true;
                    else if(strstr(line, cdlv_tag_time_once)) resource->video->loop = false;
                    // base->accum = 1.1f;
                }
                base->current_bg = resource;
            } else {
                cdlv_logv("Prompt to unknown resource: %s", name);
                cdlv_err(cdlv_parse_error);
            }
        }
        free(name);
    }
    cdlv_err(cdlv_ok);
}

static inline cdlv_error cdlv_parse_line(cdlv* base) {
    cdlv_scene* scene = (cdlv_scene*)base->current_scene;
    if(scene->script->len == 0) {
        cdlv_log("Scene's script is empty");
        cdlv_err(cdlv_parse_error);
    }
    cdlv_error res;
    if(base->current_line == scene->script->len) {
	cdlv_log("At the end of script");
        if(base->current_scene_index+1 < base->scene_count) {
	    cdlv_log("Changing scene");
            if((res = cdlv_set_scene(base, base->current_scene_index+1)) != cdlv_ok) cdlv_err(res);
	    cdlv_scene* new_scene = (cdlv_scene*)base->current_scene;
	    if(new_scene->loaded) {
		base->current_line = 0;
		if((res = cdlv_parse_line(base)) != cdlv_ok) cdlv_err(res);
	    }
        } else {
	    cdlv_log("Setting end to true");
            base->end = true;
        }
    } else if(base->current_line < scene->script->len) {
	char* line = (char*)sarr_get(scene->script, base->current_line);
        if(!line) cdlv_err(cdlv_fatal_error);
        if(line[0] == '@') {
            if((res = cdlv_parse_prompt(base, line, scene)) != cdlv_ok) cdlv_err(res);
            ++base->current_line;
            if((res = cdlv_parse_line(base)) != cdlv_ok) cdlv_err(res);
        } else {
	    if(base->user_config.line_callback)
		base->user_config.line_callback(line, base->user_config.user_data);
	}
    }
    cdlv_err(cdlv_ok);
}

cdlv_error cdlv_set_script(cdlv* base, const char* path) {
    cdlv_error res;
    char* script = NULL;

    base->resources = sdic_new();
    base->scenes = sdic_new();

    if((res = extract_path(base, path)) != cdlv_ok) cdlv_err(res);
    if((res = cdlv_read_file_to_str(base, path, &script)) != cdlv_ok) cdlv_err(res);
    if((res = cdlv_parse_script(base, script)) != cdlv_ok) cdlv_err(res);
    free(script);

    dic_forEach(base->resources, load_global_resources, base);
    if((res = cdlv_set_scene(base, 0)) != cdlv_ok) cdlv_err(res);
    if((res = cdlv_parse_line(base)) != cdlv_ok) cdlv_err(res);

    cdlv_err(cdlv_ok);
}

static inline int unload_global_resources(char *key, void *value, void *user) {
    cdlv_resource* resource = (cdlv_resource*)value;
    cdlv* base = (cdlv*)user;
    cdlv_resource_unload(base, resource);
    return 1;
}

cdlv_error cdlv_unset_script(cdlv* base) {
    dic_forEach(base->resources, unload_global_resources, base);
    free(base->resources_path);
    cdlv_resources_free(base, base->resources);
    cdlv_scenes_free(base, base->scenes);
    cdlv_err(cdlv_ok);
}

cdlv_error cdlv_user_update(cdlv* base) {
    if(base->user_config.update_callback) {
	int cb_res = base->user_config.update_callback(base->user_config.user_data);
	if(cb_res == 0) cdlv_err(cdlv_ok);
    }
    cdlv_error res;
    ++base->current_line;
    if((res = cdlv_parse_line(base)) != cdlv_ok) cdlv_err(res);
    cdlv_err(cdlv_ok);
}

cdlv_error cdlv_render(cdlv* base) {
    cdlv_resource* bg = (cdlv_resource*)base->current_bg;
    if(!bg) cdlv_err(cdlv_ok);
    if(bg->type == cdlv_resource_image) {
	if(base->image_config.render_callback)
	    base->image_config.render_callback(bg->image, base->image_config.user_data);
    } else if(bg->type == cdlv_resource_video) {
        cdlv_error res;
        if((res = cdlv_play_video(base, bg->video)) != cdlv_ok) cdlv_err(res);
	base->video_config.change_frame_bool = false;
	if(base->image_config.render_callback)
	    base->image_config.render_callback(bg->video->texture, base->video_config.user_data);
    }
    cdlv_err(cdlv_ok);
}
