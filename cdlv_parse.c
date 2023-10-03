#include "cdlv.h"
#include "cdlv_util.h"

static inline void scenes_alloc(cdlv_base* base, const size_t count) {
    base->scenes = calloc(count, sizeof(cdlv_scene));
    if(!base->scenes)
        cdlv_die("Could not allocate memory for scene list!");

    for(size_t i=0; i<count; ++i) {
        base->scenes[i] = malloc(sizeof(cdlv_scene));
        if(!base->scenes[i])
            cdlv_diev("Could not allocate memory for scene: %lu", i);
        base->scenes[i]->images             = NULL;
        base->scenes[i]->image_paths        = NULL;
        base->scenes[i]->image_count        = 0;
        base->scenes[i]->script             = NULL;
        base->scenes[i]->line_count         = 0;
        base->scenes[i]->type               = cdlv_none;
    }
}

static inline size_t count_scenes(char* const* file, const size_t line_count) {
    size_t scenes = 0;
    for(size_t i=1; i<line_count; ++i)
        if(strstr(file[i], cdlv_tag_scene))
            ++scenes;
    return scenes;
}

static inline cdlv_type check_tag(const char* line) {
    if(strstr(line, cdlv_tag_scene))            return cdlv_scene_decl;
    else if(strstr(line, cdlv_tag_bg))          return cdlv_static_scene;
    else if(strstr(line, cdlv_tag_anim_once))   return cdlv_anim_once_scene;
    else if(strstr(line, cdlv_tag_anim_wait))   return cdlv_anim_wait_scene;
    else if(strstr(line, cdlv_tag_anim_text))   return cdlv_anim_text_scene;
    else if(strstr(line, cdlv_tag_anim))        return cdlv_anim_scene;
    else if(strstr(line, cdlv_tag_script))      return cdlv_script;
    else                                        return cdlv_none;
}

static inline size_t whitespace_check(const char* line) {
    return strspn(line, " ");
}

static inline void count_scene_data(cdlv_base* base, char* const* file, const size_t line_count) {
    cdlv_type parse_m = cdlv_none;
    size_t scene_idx = 0;

    #define cdlv_temp_scene base->scenes[scene_idx-1]
    #define cdlv_increment(var) \
    if(!whitespace_check(file[i])) break; \
    ++var
    for(size_t i=0; i<line_count; ++i) {
        switch(check_tag(file[i])) {
            case cdlv_scene_decl: ++scene_idx; break;
            case cdlv_static_scene: case cdlv_anim_once_scene:
            case cdlv_anim_wait_scene: case cdlv_anim_text_scene:
            case cdlv_anim_scene:
                if(!whitespace_check(file[i])) break;
                cdlv_temp_scene->type = check_tag(file[i]);
                parse_m = check_tag(file[i]); break;
            case cdlv_script:
                if(!whitespace_check(file[i])) break;
                parse_m = check_tag(file[i]); break;
            case cdlv_none: switch(parse_m) {
                case cdlv_static_scene: case cdlv_anim_once_scene:
                case cdlv_anim_wait_scene: case cdlv_anim_text_scene:
                case cdlv_anim_scene: cdlv_increment(cdlv_temp_scene->image_count); break;
                case cdlv_script: cdlv_increment(cdlv_temp_scene->line_count); break;
                case cdlv_none: case cdlv_scene_decl: break;
            }
        }
    }
    #undef cdlv_increment
    #undef cdlv_temp_scene
}

static inline int dup_script_line(const char* line, cdlv_scene* scene, size_t* index) {
    size_t w_space = 0;
    size_t str_size = 0;

    if(!(w_space = whitespace_check(line))) return 0;
    str_size = (strlen(line+w_space)+1);

    cdlv_duplicate_string(&scene->script[*index], line+w_space, str_size);
    ++(*index);
    return 1;
}

static inline int dup_image_path(const char* line, const char* path, cdlv_scene* scene, size_t* index) {
    size_t w_space = 0;
    size_t str_size = 0;

    if(!(w_space = whitespace_check(line))) return 0;
    str_size = (strlen(line+w_space)+strlen(path)+3);
    char str[str_size];
    sprintf(str, "%s%s", path, line+w_space); \
    cdlv_duplicate_string(&scene->image_paths[*index], str, str_size);

    ++(*index);
    return 1;
}

static inline void copy_scene_data(cdlv_base* base, char* const* file, const size_t line_count) {
    cdlv_type parse_m = cdlv_none;
    size_t scene_idx = 0;
    size_t image_idx = 0;
    size_t line_idx = 0;

    #define cdlv_temp_scene base->scenes[scene_idx-1]
    #define cdlv_data_alloc(arr_name, count_name) \
        parse_m = check_tag(file[i]); \
        cdlv_alloc_ptr_arr(&cdlv_temp_scene->arr_name, cdlv_temp_scene->count_name, char*)
    for(size_t i=0; i<line_count; ++i) {
        switch(check_tag(file[i])) {
            case cdlv_scene_decl:
                ++scene_idx; image_idx = 0; line_idx = 0; break;
            case cdlv_static_scene: case cdlv_anim_once_scene:
            case cdlv_anim_wait_scene: case cdlv_anim_text_scene:
            case cdlv_anim_scene: cdlv_data_alloc(image_paths, image_count); break;
            case cdlv_script: cdlv_data_alloc(script, line_count); break;
            case cdlv_none: switch(parse_m) {
                case cdlv_static_scene: case cdlv_anim_once_scene:
                case cdlv_anim_wait_scene: case cdlv_anim_text_scene:
                case cdlv_anim_scene: 
                    if(!dup_image_path(file[i], base->canvas->path, cdlv_temp_scene, &image_idx)) continue;
                    break;
                case cdlv_script:
                    if(!dup_script_line(file[i], cdlv_temp_scene, &line_idx)) continue;
                    break;
                case cdlv_none: case cdlv_scene_decl: break;
            }
        }
    }
    #undef cdlv_data_alloc
    #undef cdlv_temp_scene
}

void cdlv_read_file(cdlv_base* base, const char* file, SDL_Renderer** r) {
    size_t lines;
    char** script = cdlv_read_file_in_lines(file, &lines);
    size_t canvas_w, canvas_h, framerate;
    char path[cdlv_small_string];
    if(!sscanf(script[0], "%lu %lu %lu %s",
                &canvas_w, &canvas_h, &framerate, path))
        cdlv_diev("Wrong data on the first line: %s", script[0]);

    cdlv_canvas_create(base, path, canvas_w, canvas_h, framerate, r);
    cdlv_text_create(base, base->config->text_font, base->config->text_size,
            base->config->text_wrap,
            base->config->text_xy.x, base->config->text_xy.y,
            base->config->text_color.r, base->config->text_color.g,
            base->config->text_color.b, base->config->text_color.a,
            *r);

    base->scene_count = count_scenes(script, lines);
    scenes_alloc(base, base->scene_count);
    count_scene_data(base, script, lines);
    copy_scene_data(base, script, lines);
    cdlv_free_file_in_lines(script, lines);
}
