#include "parse.h"
#include "util.h"
#include "resource.h"
#include "scene.h"

static inline size_t strspn_whitespace(const char* line) {
    return strspn(line, " \t");
}

static inline char* find_first_whitespace(const char* line) {
    char* whitespace = strchr(line, ' ');
    if(whitespace) return whitespace;
    whitespace = strchr(line, '\t');
    if(whitespace) return whitespace;
    return NULL;
}

static inline cdlv_error extract_from_quotes(cdlv* base, const char* line, char** output) {
    char* quote_open = strchr(line, '\"');
    if(!quote_open) {
        cdlv_logv("Did not find opening quote on line: %s", line);
        cdlv_err(cdlv_parse_error);
    }
    char* quote_close = strrchr(line, '\"');
    if(!quote_close) {
        cdlv_logv("Did not find closing quote on line: %s", line);
        cdlv_err(cdlv_parse_error);
    }
    char str_content[quote_close-quote_open];
    sprintf(str_content, "%.*s", (int)(quote_close-quote_open-1), quote_open+1);
    cdlv_strdup(output, str_content, quote_close-quote_open);
    cdlv_err(cdlv_ok);
}

static inline cdlv_error extract_non_quote(cdlv* base, const char* line, char** output) {
    char* start_position = NULL;
    bool scene_name = line[0] == '!' ? true : false;
    if(scene_name) start_position = find_first_whitespace(line);
    else start_position = line;
    char* end_position = find_first_whitespace(start_position+1);
    if(!end_position) {
        cdlv_logv("Did not find name on line: %s", line);
        cdlv_err(cdlv_parse_error);
    }
    size_t size = scene_name ? end_position-start_position : end_position-start_position + 1;
    char str_content[size];
    sprintf(str_content, "%.*s", (int)size-1, scene_name ? start_position+1 : start_position);
    cdlv_strdup(output, str_content, size);
    cdlv_err(cdlv_ok);
}

static inline cdlv_error extract_filename(cdlv* base, const char* line, char** output) {
    char* dot_position = strchr(line, '.');
    if(!dot_position) {
        cdlv_logv("Did not find filename on line: %s", line);
        cdlv_err(cdlv_parse_error);
    }
    size_t size = dot_position-line+1;
    char str_content[size];
    sprintf(str_content, "%.*s", (int)(size-1), line);
    cdlv_strdup(output, str_content, size);
    cdlv_err(cdlv_ok);
}

static inline cdlv_error extract_resource_kv(cdlv* base, const char* line, char** key, char** value) {
    cdlv_error res;
    if(line[0] == '\"') {
        if((res = extract_from_quotes(base, line, value)) != cdlv_ok) {
            cdlv_err(res);
        }
        if((res = extract_filename(base, *value, key)) != cdlv_ok) {
            cdlv_err(res);
        }
        cdlv_err(cdlv_ok);
    } else if(strstr(line, cdlv_tag_opening)) {
    } else {
        if((res = extract_non_quote(base, line, key)) != cdlv_ok) {
            cdlv_err(res);
        }
        if((res = extract_from_quotes(base, line, value)) != cdlv_ok) {
            cdlv_err(res);
        }
        cdlv_err(cdlv_ok);
    }
    cdlv_logv("Could not parse resource: %s", line);
    cdlv_err(cdlv_parse_error);
}

static inline cdlv_error add_new_resource(cdlv* base, const char* line, cdlv_dict* resources) {
    cdlv_error res;
    char* key, *path;
    cdlv_resource* resource;
    if((res = extract_resource_kv(base, line, &key, &path)) != cdlv_ok) {
        cdlv_err(res);
    }
    printf(" %s %d %s", key, strlen(key), path);
    if((res = cdlv_resource_new(base, cdlv_resource_image, path, &resource)) != cdlv_ok) {
        cdlv_err(res);
    }
    dic_add(resources, key, strlen(key));
    *resources->value = resource;
    cdlv_err(cdlv_ok);
}

static inline cdlv_error add_new_scene(cdlv* base, const char* line, cdlv_scene** scene) {
    printf(", extracting");
    char* name = NULL;
    cdlv_error result = extract_non_quote(base, line, &name);
    if(result != cdlv_ok) {
        cdlv_err(result);
    }
    printf(" %s", name);
    cdlv_scene* new_scene;
    if((result = cdlv_scene_new(base, base->resources_path, &new_scene)) != cdlv_ok) {
        cdlv_err(result);
    }
    dic_add(base->scenes, name, strlen(name));
    *base->scenes->value = new_scene;
    *scene = new_scene;
    cdlv_err(cdlv_ok);
}

static inline cdlv_error parse_global_resource(cdlv* base, cdlv_parse_mode* mode, const char* line) {
    if(strstr(line, cdlv_tag_closure)) {
        printf("\t-- changing parse mode to global");
        *mode = cdlv_parse_global;
        cdlv_err(cdlv_ok);
    }
    printf("\t-- extracting");
    cdlv_error res;
    if((res = add_new_resource(base, line, base->resources)) != cdlv_ok) {
        cdlv_err(res);
    }
    cdlv_err(cdlv_ok);
}

static inline cdlv_error parse_global_definition(cdlv* base, cdlv_parse_mode* mode, const char* line, cdlv_scene** scene) {
    if(strstr(line, cdlv_tag_define_resources_path)) {
        printf("\t-- extracting");
        cdlv_error result = extract_from_quotes(base, line, &base->resources_path);
        if(result == cdlv_ok) printf(" %s", base->resources_path);
        cdlv_err(result);
    } else if(strstr(line, cdlv_tag_define_resources)) {
        printf("\t-- changing parse mode to global_resources");
        *mode = cdlv_parse_global_resources;
        cdlv_err(cdlv_ok);
    } else if(strstr(line, cdlv_tag_define_scene)) {
        printf("\t-- changing parse mode to parse_scene");
        cdlv_error result = add_new_scene(base, line, scene);
        if(result != cdlv_ok) {
            cdlv_err(cdlv_ok);
        }
        *mode = cdlv_parse_scene;
        cdlv_err(cdlv_ok);
    }

    cdlv_logv("Did not find proper define tag while parsing in global mode: %s", line);
    cdlv_err(cdlv_parse_error);
}

static inline cdlv_error parse_scene_resource(cdlv* base, cdlv_parse_mode* mode, const char* line, cdlv_scene** scene) {
    if(strstr(line, cdlv_tag_closure)) {
        printf("\t-- changing parse mode to scene");
        *mode = cdlv_parse_scene;
        cdlv_err(cdlv_ok);
    }
    cdlv_scene* current_scene = *scene;
    printf("\t-- extracting");
    cdlv_error res;
    if((res = add_new_resource(base, line, current_scene->resources)) != cdlv_ok) {
        cdlv_err(res);
    }
    cdlv_err(cdlv_ok);
}

static inline cdlv_error parse_scene_definition(cdlv* base, cdlv_parse_mode* mode, const char* line, cdlv_scene** scene) {
    cdlv_scene* current_scene = *scene;
    if(strstr(line, cdlv_tag_define_resources_path)) {
        printf("\t-- extracting");
        char* new_path;
        cdlv_error result = extract_from_quotes(base, line, &new_path);
        if(result != cdlv_ok) {
            cdlv_err(result);
        }
        char tmp[strlen(new_path)+strlen(base->resources_path)+1];
        sprintf(tmp, "%s%s", base->resources_path, new_path);
        free(new_path);
        free(current_scene->resources_path);
        cdlv_strdup(&current_scene->resources_path, tmp, strlen(tmp));
        printf(" %s", current_scene->resources_path);
        cdlv_err(result);
    } else if(strstr(line, cdlv_tag_define_resources)) {
        printf("\t-- changing parse mode to scene_resources");
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
            if((res = parse_global_definition(base, mode, line, scene)) != cdlv_ok) {
                cdlv_err(res);
            }
            break;
        case cdlv_parse_global_resources:
            cdlv_logv("Definitions are not allowed in resources context: %s", line);
            cdlv_err(cdlv_parse_error);
            break;
        case cdlv_parse_scene:
            if((res = parse_scene_definition(base, mode, line, scene)) != cdlv_ok) {
                cdlv_err(res);
            }
            break;
        case cdlv_parse_scene_resources:
            cdlv_logv("Definitions are not allowed in resources context: %s", line);
            cdlv_err(cdlv_parse_error);
            break;
    }

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
            if((res = parse_global_resource(base, mode, line)) != cdlv_ok) {
                cdlv_err(res);
            }
            break;
        case cdlv_parse_scene:
            if(strstr(line, cdlv_tag_closure)) {
                printf("\t-- changing parse mode to global");
                *mode = cdlv_parse_global;
                cdlv_err(cdlv_ok);
            }
            printf("\t-- adding to script");
            SCL_ARRAY_ADD((*scene)->script, line, char*);
            break;
        case cdlv_parse_scene_resources:
            if((res = parse_scene_resource(base, mode, line, scene)) != cdlv_ok) {
                cdlv_err(res);
            }
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
        printf("%s", line);
        if(line[0] == '#') { 
            printf("\t-- skipping");
            goto next_line;
        } else if(line[0] == '!') {
            if((res = parse_definition(base, &mode, line, &scene)) != cdlv_ok) {
                cdlv_err(res);
            }
        } else {
            if((res = parse_nprefix_line(base, &mode, line, &scene)) != cdlv_ok) {
                cdlv_err(res);
            }
        }
        next_line:
        printf("\n");
        token = strtok(NULL, "\r\n");
    }

    cdlv_err(cdlv_ok);
}