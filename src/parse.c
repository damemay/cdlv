#include "cdlv.h"
#define MAX_LEN 1024

static inline char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if(!file) perror("file"), exit(EXIT_FAILURE);

    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char* code = malloc(size+1);
    if(!code)
        fclose(file), diev("Could not allocate memory for file: %s", path);

    if(fread(code, size, 1, file) != 1)
        fclose(file), free(code), diev("Could not read file: %s", path);

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
        fclose(file), diev("Could not allocate memory for file: %s", path);

    char line[MAX_LEN];
    size_t i = 0;
    while(fgets(line, sizeof(line), file) != NULL) {
        char* n;
        if((n = strchr(line, '\n')) != NULL)
            line[n-line] = '\0';
        duplicate_string(&code[i], line, sizeof(line));
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
        die("Could not allocate memory for scene list!");

    for(size_t i=0; i<count; ++i) {
        base->scenes[i] = malloc(sizeof(cdlv_scene));
        if(!base->scenes[i])
            diev("Could not allocate memory for scene: %lu", i);
        base->scenes[i]->images             = NULL;
        base->scenes[i]->image_paths        = NULL;
        base->scenes[i]->image_count        = 0;
        base->scenes[i]->script             = NULL;
        base->scenes[i]->line_count         = 0;
        base->scenes[i]->type               = cdlv_none;
    }
}

static inline void cdlv_parse(cdlv_base* base, char* const* file, const size_t line_count) {
    #define cdlv_check_all_parse_tags(line, scene)          \
    if(strstr(line, cdlv_tag_scene)) {                      \
        parse_mode = cdlv_static_scene;                    \
        ++scene_idx;                                        \
        scene->type = cdlv_static_scene; break;            \
    } else if(strstr(line, cdlv_tag_bg)) {                  \
        parse_mode = cdlv_static_scene;                    \
        scene->type = cdlv_static_scene; break;            \
    } else if(strstr(line, cdlv_tag_anim_once)) {           \
        parse_mode = cdlv_anim_once_scene;                  \
        scene->type = cdlv_anim_once_scene; break;          \
    } else if(strstr(line, cdlv_tag_anim_wait)) {           \
        parse_mode = cdlv_anim_wait_scene;                  \
        scene->type = cdlv_anim_wait_scene; break;          \
    } else if(strstr(line, cdlv_tag_anim_text)) {           \
        parse_mode =  cdlv_anim_text_scene;                  \
        scene->type = cdlv_anim_text_scene; break;          \
    } else if(strstr(line, cdlv_tag_anim)) {                \
        parse_mode = cdlv_anim_scene;                       \
        scene->type = cdlv_anim_scene; break;               \
    } else if(strstr(line, cdlv_tag_script)) {              \
        parse_mode = cdlv_script; break;                    \
    }
    cdlv_type parse_mode = cdlv_none;
    size_t scene_idx = 0;
    // Count
    for(size_t i=1; i<line_count; ++i) {
        switch(parse_mode) {
            case cdlv_none:
                if(strstr(file[i], cdlv_tag_scene)) {
                    parse_mode = cdlv_static_scene;
                }
                break;
            case cdlv_static_scene:
                cdlv_check_all_parse_tags(file[i],
                        base->scenes[scene_idx]);
                if(strspn(file[i], " ") == 0) break;
                ++base->scenes[scene_idx]->image_count;
                break;
            case cdlv_anim_scene:
                cdlv_check_all_parse_tags(file[i],
                        base->scenes[scene_idx]);
                if(strspn(file[i], " ") == 0) break;
                ++base->scenes[scene_idx]->image_count;
                break;
            case cdlv_anim_once_scene:
                cdlv_check_all_parse_tags(file[i],
                        base->scenes[scene_idx]);
                if(strspn(file[i], " ") == 0) break;
                ++base->scenes[scene_idx]->image_count;
                break;
            case cdlv_anim_wait_scene:
                cdlv_check_all_parse_tags(file[i],
                        base->scenes[scene_idx]);
                if(strspn(file[i], " ") == 0) break;
                ++base->scenes[scene_idx]->image_count;
                break;
            case cdlv_anim_text_scene:
                cdlv_check_all_parse_tags(file[i],
                        base->scenes[scene_idx]);
                if(strspn(file[i], " ") == 0) break;
                ++base->scenes[scene_idx]->image_count;
                break;
            case cdlv_script:
                cdlv_check_all_parse_tags(file[i],
                        base->scenes[scene_idx]);
                if(strspn(file[i], " ") == 0) break;
                ++base->scenes[scene_idx]->line_count;
                break;
        }
    }
    #undef cdlv_check_all_parse_tags
    parse_mode = cdlv_none;
    scene_idx = 0;
    size_t image_idx = 0;
    size_t script_line_idx = 0;
    #define cdlv_check_all_parse_tags(line, scene)          \
    if(strstr(line, cdlv_tag_scene)) {                      \
        parse_mode = cdlv_static_scene;                    \
        ++scene_idx;                                        \
        image_idx = 0;                                      \
        script_line_idx = 0;                                \
        break;            \
    } else if(strstr(line, cdlv_tag_bg)) {                  \
        parse_mode = cdlv_static_scene;                    \
        alloc_ptr_arr(&scene->image_paths,                  \
                scene->image_count, char*);                 \
        break;            \
    } else if(strstr(line, cdlv_tag_anim_once)) {           \
        parse_mode = cdlv_anim_once_scene;                  \
        alloc_ptr_arr(&scene->image_paths,                  \
                scene->image_count, char*);                 \
        break;          \
    } else if(strstr(line, cdlv_tag_anim_wait)) {           \
        parse_mode = cdlv_anim_wait_scene;                  \
        alloc_ptr_arr(&scene->image_paths,                  \
                scene->image_count, char*);                 \
        break;          \
    } else if(strstr(line, cdlv_tag_anim_text)) {           \
        parse_mode = cdlv_anim_text_scene;                  \
        alloc_ptr_arr(&scene->image_paths,                  \
                scene->image_count, char*);                 \
        break;          \
    } else if(strstr(line, cdlv_tag_anim)) {                \
        parse_mode = cdlv_anim_scene;                       \
        alloc_ptr_arr(&scene->image_paths,                  \
                scene->image_count, char*);                 \
        break;               \
    } else if(strstr(line, cdlv_tag_script)) {              \
        alloc_ptr_arr(&scene->script,                       \
                scene->line_count, char*);                  \
        parse_mode = cdlv_script; break;                    \
    }
    for(size_t i=1; i<line_count; ++i) {
        size_t w_space = 0;
        size_t str_size = 0;
        switch(parse_mode) {
            case cdlv_none:
                if(strstr(file[i], cdlv_tag_scene))
                    parse_mode = cdlv_static_scene;
                break;
            case cdlv_static_scene:
                cdlv_check_all_parse_tags(file[i],
                        base->scenes[scene_idx]);
                /*
                 * Current line did not have any tags,
                 * copy info about resources saved on this line.
                 */
                if((w_space = strspn(file[i], " ")) == 0) break;
                str_size = (strlen(file[i]+w_space)+1);
                duplicate_string(&base->scenes[scene_idx]->
                        image_paths[image_idx],
                        file[i]+w_space, str_size);
                ++image_idx;
                break;
            case cdlv_anim_scene:
                cdlv_check_all_parse_tags(file[i],
                        base->scenes[scene_idx]);
                /*
                 * Current line did not have any tags,
                 * copy info about resources saved on this line.
                 */
                if((w_space = strspn(file[i], " ")) == 0) break;
                str_size = (strlen(file[i]+w_space)+1);
                duplicate_string(&base->scenes[scene_idx]->
                        image_paths[image_idx],
                        file[i]+w_space, str_size);
                ++image_idx;
                break;
            case cdlv_anim_once_scene:
                cdlv_check_all_parse_tags(file[i],
                        base->scenes[scene_idx]);
                if((w_space = strspn(file[i], " ")) == 0) break;
                str_size = (strlen(file[i]+w_space)+1);
                duplicate_string(&base->scenes[scene_idx]->
                        image_paths[image_idx],
                        file[i]+w_space, str_size);
                ++image_idx;
                break;
            case cdlv_anim_wait_scene:
                cdlv_check_all_parse_tags(file[i],
                        base->scenes[scene_idx]);
                if((w_space = strspn(file[i], " ")) == 0) break;
                str_size = (strlen(file[i]+w_space)+1);
                duplicate_string(&base->scenes[scene_idx]->
                        image_paths[image_idx],
                        file[i]+w_space, str_size);
                ++image_idx;
                break;
            case cdlv_anim_text_scene:
                cdlv_check_all_parse_tags(file[i],
                        base->scenes[scene_idx]);
                if((w_space = strspn(file[i], " ")) == 0) break;
                str_size = (strlen(file[i]+w_space)+1);
                duplicate_string(&base->scenes[scene_idx]->
                        image_paths[image_idx],
                        file[i]+w_space, str_size);
                ++image_idx;
                break;
            case cdlv_script:
                cdlv_check_all_parse_tags(file[i],
                        base->scenes[scene_idx]);
                /*
                 * Current line did not have any tags,
                 * copy script text saved on this line.
                 */
                if((w_space = strspn(file[i], " ")) == 0) break;
                str_size = (strlen(file[i]+w_space)+1);
                duplicate_string(&base->scenes[scene_idx]->
                        script[script_line_idx],
                        file[i]+w_space, str_size);
                ++script_line_idx;
                break;
        }
    }
    #undef cdlv_check_all_parse_tags
}

void cdlv_read_file(cdlv_base* base, const char* file) {
    size_t lines;
    char** script = read_file_in_lines(file, &lines);
    size_t scene_count;
    size_t canvas_w, canvas_h, framerate;
    char font_path[MAX_LEN];
    size_t font_size;
    if(!sscanf(script[0], "%lu %lu %lu %lu %s %lu",
                &scene_count, &canvas_w, &canvas_h, &framerate,
                font_path, &font_size))
        diev("Wrong data on the first line: %s", script[0]);

    cdlv_canvas_create(base, canvas_w, canvas_h, framerate);
    cdlv_text_create(base, font_path, font_size, 800, 50, 400,
            255, 255, 255, 255);

    base->scene_count = scene_count;
    scenes_alloc(base, scene_count);

    cdlv_parse(base, script, lines);
    free_file_in_lines(script, lines);
}

void cdlv_scene_info(cdlv_base* base, const size_t index) {
    #define scene base->scenes[index]
    switch(scene->type) {
        case cdlv_anim_scene:
            printf("[[SCENE %lu] ANIM]\n", index);
            break;
        case cdlv_anim_once_scene:
            printf("[[SCENE %lu] ONCE]\n", index);
            break;
        default:
            printf("[[SCENE %lu] STATIC]\n", index);
            break;
    }
    printf("\t[resources|%lu]\n\t\t", scene->image_count);
    for(size_t i=0; i<scene->image_count; ++i)
        printf("%s, ", scene->image_paths[i]);
    printf("\n");
    printf("\t[lines|%lu]\n\t\t", scene->line_count);
    for(size_t i=0; i<scene->line_count; ++i)
        printf("%s, ", scene->script[i]);
    printf("\n");
    #undef scene
}

#undef MAX_LEN
