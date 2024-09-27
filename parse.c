#include "parse.h"
#include "util.h"
#include "scene.h"

static inline size_t strspn_whitespace(const char* line) {
    return strspn(line, " \t");
}

static inline cdlv_error add_new_scene(cdlv* base, const char* line, cdlv_scene** scene) {
    char* name = NULL;
    cdlv_error result = cdlv_extract_non_quote(base, line, &name);
    if(result != cdlv_ok) cdlv_err(result);
    cdlv_scene* new_scene;
    if((result = cdlv_scene_new(base, base->resources_path, &new_scene)) != cdlv_ok) cdlv_err(result);
    new_scene->index = base->scene_count;
    base->scene_count++;
    sdic_add(base->scenes, name, new_scene);
    *scene = new_scene;
    cdlv_err(cdlv_ok);
}

static inline cdlv_error parse_global_resource(cdlv* base, cdlv_parse_mode* mode, const char* line) {
    if(strstr(line, cdlv_tag_closure)) {
        *mode = cdlv_parse_global;
        cdlv_err(cdlv_ok);
    }
    cdlv_error res;
    if((res = cdlv_add_new_resource(base, base->resources_path, line, base->resources)) != cdlv_ok) cdlv_err(res);
    cdlv_err(cdlv_ok);
}

static inline cdlv_error parse_global_definition(cdlv* base, cdlv_parse_mode* mode, const char* line, cdlv_scene** scene) {
    if(strstr(line, cdlv_tag_define_resources_path)) {
        char* new_path;
        cdlv_error result = cdlv_extract_from_quotes(base, line, &new_path);
        if(result != cdlv_ok) cdlv_err(result);
        if((result = cdlv_strcat_new(base, base->resources_path, new_path, &base->resources_path)) != cdlv_ok) cdlv_err(result);
        free(new_path);
        cdlv_err(result);
    } else if(strstr(line, cdlv_tag_define_resources)) {
        *mode = cdlv_parse_global_resources;
        cdlv_err(cdlv_ok);
    } else if(strstr(line, cdlv_tag_define_scene)) {
        cdlv_error result = add_new_scene(base, line, scene);
        if(result != cdlv_ok) cdlv_err(cdlv_ok);
        *mode = cdlv_parse_scene;
        cdlv_err(cdlv_ok);
    }

    cdlv_logv("Did not find proper define tag while parsing in global mode: %s", line);
    cdlv_err(cdlv_parse_error);
}

static inline cdlv_error parse_scene_resource(cdlv* base, cdlv_parse_mode* mode, const char* line, cdlv_scene** scene) {
    if(strstr(line, cdlv_tag_closure)) {
        *mode = cdlv_parse_scene;
        cdlv_err(cdlv_ok);
    }
    cdlv_scene* current_scene = *scene;
    cdlv_error res;
    if((res = cdlv_add_new_resource(base, current_scene->resources_path, line, current_scene->resources)) != cdlv_ok) cdlv_err(res);
    cdlv_err(cdlv_ok);
}

static inline cdlv_error parse_scene_definition(cdlv* base, cdlv_parse_mode* mode, const char* line, cdlv_scene** scene) {
    cdlv_scene* current_scene = *scene;
    if(strstr(line, cdlv_tag_define_resources_path)) {
        char* new_path;
        cdlv_error result = cdlv_extract_from_quotes(base, line, &new_path);
        if(result != cdlv_ok) cdlv_err(result);
        if((result = cdlv_strcat_new(base, base->resources_path, new_path, &current_scene->resources_path)) != cdlv_ok) cdlv_err(result);
        free(new_path);
        cdlv_err(result);
    } else if(strstr(line, cdlv_tag_define_resources)) {
        *mode = cdlv_parse_scene_resources;
        cdlv_err(cdlv_ok);
    }

    cdlv_logv("Did not find proper define tag while parsing in scene mode: %s", line);
    cdlv_err(cdlv_parse_error);
}

static inline cdlv_error parse_definition(cdlv* base, cdlv_parse_mode* mode, const char* line, cdlv_scene** scene) {
    cdlv_parse_mode current_mode = *mode;
    cdlv_error res;
    switch(current_mode) {
        case cdlv_parse_global:
            if((res = parse_global_definition(base, mode, line, scene)) != cdlv_ok) cdlv_err(res);
            break;
        case cdlv_parse_global_resources:
            cdlv_logv("Definitions are not allowed in resources context: %s", line);
            cdlv_err(cdlv_parse_error);
            break;
        case cdlv_parse_scene:
            if((res = parse_scene_definition(base, mode, line, scene)) != cdlv_ok) cdlv_err(res);
            break;
        case cdlv_parse_scene_resources:
            cdlv_logv("Definitions are not allowed in resources context: %s", line);
            cdlv_err(cdlv_parse_error);
            break;
    }

    cdlv_err(cdlv_ok);
}

static inline cdlv_error add_script_line(cdlv* base, cdlv_scene* scene, const char* line) {
    char* dup_line;
    cdlv_strdup(&dup_line, line, strlen(line)+1);
    sarr_add(scene->script, dup_line);
    cdlv_err(cdlv_ok);
}

static inline cdlv_error parse_nprefix_line(cdlv* base, cdlv_parse_mode* mode, const char* line, cdlv_scene** scene) {
    cdlv_parse_mode current_mode = *mode;
    cdlv_error res;
    switch(current_mode) {
        case cdlv_parse_global:
            cdlv_logv("Only definitions are allowed in global context: %s", line);
            cdlv_err(cdlv_parse_error);
            break;
        case cdlv_parse_global_resources:
            if((res = parse_global_resource(base, mode, line)) != cdlv_ok) cdlv_err(res);
            break;
        case cdlv_parse_scene:
            if(strstr(line, cdlv_tag_closure)) {
                *mode = cdlv_parse_global;
                cdlv_err(cdlv_ok);
            }
            if(!strlen(line)) break;
            if((res = add_script_line(base, *scene, line)) != cdlv_ok) cdlv_err(res);
            break;
        case cdlv_parse_scene_resources:
            if((res = parse_scene_resource(base, mode, line, scene)) != cdlv_ok) cdlv_err(res);
            break;
    }

    cdlv_err(cdlv_ok);
}

cdlv_error cdlv_parse_script(cdlv* base, char* script) {
    cdlv_parse_mode mode = 0;
    cdlv_scene* scene = NULL;
    cdlv_error res;

    char* token = strtok(script, "\r\n");
    while(token != NULL) {
        size_t w = strspn_whitespace(token);
        char* line = token+w;
        if(line[0] == '#') { 
            goto next_line;
        } else if(line[0] == '!') {
            if((res = parse_definition(base, &mode, line, &scene)) != cdlv_ok) cdlv_err(res);
        } else {
            if((res = parse_nprefix_line(base, &mode, line, &scene)) != cdlv_ok) cdlv_err(res);
        }
        next_line:
        token = strtok(NULL, "\r\n");
    }

    cdlv_err(cdlv_ok);
}
