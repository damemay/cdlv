#include "cdlv.h"

static inline char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if(!file) perror("file"), exit(EXIT_FAILURE);

    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char* code = malloc(size+1);
    if(!code)
        fclose(file), cdlv_diev("Could not allocate memory for file: %s", path);

    if(fread(code, size, 1, file) != 1)
        fclose(file), free(code), cdlv_diev("Could not read file: %s", path);

    fclose(file);
    code[size] = '\0';
    return code;
}

static inline char** read_file_in_lines(const char* path, size_t* line_count) {
    FILE* file = fopen(path, "rb");
    if(!file) perror("file"), exit(EXIT_FAILURE);

    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char** code = malloc(size+1);
    if(!code)
        fclose(file), cdlv_diev("Could not allocate memory for file: %s", path);

    char line[1024];
    size_t i = 0;
    while(fgets(line, sizeof(line), file) != NULL) {
        char* n;
        if((n = strstr(line, "\r\n")) != NULL)
            line[n-line] = '\0';
        cdlv_duplicate_string(&code[i], line, sizeof(line));
        ++i;
    }

    if(feof(file)) fclose(file);

    *line_count = i;
    return code;
}

static inline void free_file_in_lines(char** file, const size_t line_count) {
    for(size_t i=0; i<line_count; ++i) free(file[i]);
    free(file);
}


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

static inline void copy_scene_data(cdlv_base* base, char* const* file, const size_t line_count) {
    cdlv_type parse_m = cdlv_none;
    size_t scene_idx = 0;
    size_t image_idx = 0;
    size_t line_idx = 0;

    #define cdlv_temp_scene base->scenes[scene_idx-1]
    #define cdlv_data_alloc(arr_name, count_name) \
    parse_m = check_tag(file[i]); \
    cdlv_alloc_ptr_arr(&cdlv_temp_scene->arr_name, cdlv_temp_scene->count_name, char*)

    #define cdlv_data_dup(arr_name, index) \
    if(!(w_space = whitespace_check(file[i]))) continue; \
    str_size = (strlen(file[i]+w_space)+1); \
    cdlv_duplicate_string(&cdlv_temp_scene->arr_name[index], file[i]+w_space, str_size); \
    ++index
    for(size_t i=0; i<line_count; ++i) {
        size_t w_space = 0;
        size_t str_size = 0;
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
                case cdlv_anim_scene: cdlv_data_dup(image_paths, image_idx); break;
                case cdlv_script: cdlv_data_dup(script, line_idx); break;
                case cdlv_none: case cdlv_scene_decl: break;
            }
        }
    }
    #undef cdlv_data_dup
    #undef cdlv_data_alloc
    #undef cdlv_temp_scene
}

void cdlv_read_file(cdlv_base* base, const char* file, SDL_Renderer** r) {
    size_t lines;
    char** script = read_file_in_lines(file, &lines);
    size_t canvas_w, canvas_h, framerate;
    char font_path[1024];
    size_t font_size;
    if(!sscanf(script[0], "%lu %lu %lu %s %lu",
                &canvas_w, &canvas_h, &framerate,
                font_path, &font_size))
        cdlv_diev("Wrong data on the first line: %s", script[0]);

    cdlv_canvas_create(base, canvas_w, canvas_h, framerate, r);
    cdlv_text_create(base, font_path, font_size,
            base->config->text_wrap,
            base->config->text_x, base->config->text_y,
            base->config->text_color_r, base->config->text_color_g,
            base->config->text_color_b, base->config->text_color_a,
            *r);

    base->scene_count = count_scenes(script, lines);
    scenes_alloc(base, base->scene_count);
    count_scene_data(base, script, lines);
    copy_scene_data(base, script, lines);
    free_file_in_lines(script, lines);
}
