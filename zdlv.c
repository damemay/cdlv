#include "cdlv.h"
#include "cdlv_util.h"
#include "menu/cdlv_menu.h"
#include "zocket/zocket.h"
#include "zdlv.h"
#include <sys/stat.h>
#include <unistd.h>

struct scene_paths {
    char** images;
    uint16_t image_count;
};

struct image {
    char* data;
    size_t size;
};

static inline void write_file_zkt_data(const char* path, zkt_data* data) {
    FILE* file = fopen(path, "wb");
    if(!file) {
        perror("fopen");
        return;
    }

    fwrite(data->buffer, data->size, 1, file);
    fclose(file);
}

static inline zkt_data* pack_scene(struct scene_paths* scene) {
    struct image* images = calloc(scene->image_count, sizeof(struct image));
    for(size_t i=0; i<scene->image_count; i++) {
        size_t size;
        images[i].data = zdlv_read_file(scene->images[i], &size);
        images[i].size = size;
    }

    size_t size = 0;
    for(size_t i=0; i<scene->image_count; i++) size += 3 + images[i].size; //IMGBIN

    char* buffer = malloc(size);
    size_t offset = 0;
    for(size_t i=0; i<scene->image_count; i++) {
        memcpy(buffer+offset, "IMG", 3);
        offset += 3;
        memcpy(buffer+offset, images[i].data, images[i].size);
        offset += images[i].size;
    }
    zkt_data* ret = malloc(sizeof(zkt_data));
    ret->buffer = malloc(size);
    ret->size = size;
    memcpy(ret->buffer, buffer, size);

    free(buffer);
    for(size_t i=0; i<scene->image_count; i++) free(images[i].data);
    free(images);

    return ret;
}

static inline size_t count_scenes(char* const* file, const size_t line_count) {
    size_t scenes = 0;
    for(size_t i=1; i<line_count; ++i) if(strstr(file[i], cdlv_tag_scene)) ++scenes;
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

static inline size_t whitespace_check(const char* line) { return strspn(line, " "); }
static inline void count_scene_data(struct scene_paths** scenes, char* const* file, const size_t line_count) {
    cdlv_type parse_m = cdlv_none;
    size_t scene_idx = 0;
    for(size_t i=0; i<line_count; ++i) {
        switch(check_tag(file[i])) {
            case cdlv_scene_decl: ++scene_idx; break;
            case cdlv_static_scene: case cdlv_anim_once_scene:
            case cdlv_anim_wait_scene: case cdlv_anim_text_scene:
            case cdlv_anim_scene:
                if(!whitespace_check(file[i])) break;
                parse_m = check_tag(file[i]); break;
            case cdlv_script:
                if(!whitespace_check(file[i])) break;
                parse_m = check_tag(file[i]); break;
            case cdlv_none: switch(parse_m) {
                case cdlv_static_scene: case cdlv_anim_once_scene:
                case cdlv_anim_wait_scene: case cdlv_anim_text_scene:
                case cdlv_anim_scene:
                    if(!whitespace_check(file[i])) break;
                    ++scenes[scene_idx-1]->image_count;
                    break;
                default: break;
            }
        }
    }
}

static inline int dup_image_path(const char* line, const char* path, struct scene_paths* scene, size_t* index) {
    size_t w_space = 0;
    size_t str_size = 0;
    if(!(w_space = whitespace_check(line))) return 0;
    str_size = (strlen(line+w_space)+strlen(path)+3);
    char str[str_size];
    sprintf(str, "%s%s", path, line+w_space);
    scene->images[*index] = calloc(str_size, sizeof(char));
    strcpy(scene->images[*index], str);
    ++(*index);
    return 1;
}

static inline void copy_scene_data(struct scene_paths** scenes, const char* path, char* const* file, const size_t line_count) {
    cdlv_type parse_m = cdlv_none;
    size_t scene_idx = 0;
    size_t image_idx = 0;
    size_t line_idx = 0;
    #define cdlv_temp_scene scenes[scene_idx-1]
    for(size_t i=0; i<line_count; ++i) {
        switch(check_tag(file[i])) {
            case cdlv_scene_decl:
                ++scene_idx; image_idx = 0; line_idx = 0; break;
            case cdlv_static_scene: case cdlv_anim_once_scene:
            case cdlv_anim_wait_scene: case cdlv_anim_text_scene:
            case cdlv_anim_scene:
                parse_m = check_tag(file[i]);
                cdlv_temp_scene->images = calloc(cdlv_temp_scene->image_count, sizeof(char*)); 
                break;
            case cdlv_script: parse_m = check_tag(file[i]); break;
            case cdlv_none: switch(parse_m) {
                case cdlv_static_scene: case cdlv_anim_once_scene:
                case cdlv_anim_wait_scene: case cdlv_anim_text_scene:
                case cdlv_anim_scene: 
                    if(!dup_image_path(file[i], path, cdlv_temp_scene, &image_idx)) continue;
                    break;
                default: break;
            }
        }
    }
    #undef cdlv_temp_scene
}

static int pack_script(const char* file) {
    size_t lines;
    char** script = cdlv_read_file_in_lines(file, &lines);
    char path[cdlv_small_string];
    if(!sscanf(script[0], "%*lu %*lu %*lu %s", path)) {
        cdlv_logv("wrong data on the first line: %s", script[0]);
        return -1;
    }

    size_t scene_count = count_scenes(script, lines);
    struct scene_paths** scenes = calloc(scene_count, sizeof(struct scene_paths));
    for(size_t i=0; i<scene_count; i++) scenes[i] = calloc(1, sizeof(struct scene_paths));
    count_scene_data(scenes, script, lines);
    copy_scene_data(scenes, path, script, lines);
    cdlv_free_file_in_lines(script, lines);

    zkt_data** scenes_data = calloc(scene_count, sizeof(zkt_data*));
    for(size_t i=0; i<scene_count; i++)
        scenes_data[i] = pack_scene(scenes[i]);

    char scene_dir[cdlv_max_string_size];
    strncpy(scene_dir, file, strlen(file)-4);

    if(mkdir(scene_dir, 0755) == -1) cdlv_logv("%s already exists or another error occured", scene_dir);

    for(size_t i=0; i<scene_count; i++) {
        char file_path[cdlv_max_string_size*2];
        sprintf(file_path, "%s/%lu", scene_dir, i);
        write_file_zkt_data(file_path, scenes_data[i]);
        zkt_data_clean(scenes_data[i]);
    }

    free(scenes_data);

    for(size_t i=0; i<scene_count; i++) {
        for(size_t j=0; j<scenes[i]->image_count; j++) free(scenes[i]->images[j]);
        free(scenes[i]->images);
        free(scenes[i]);
    }
    free(scenes);
    return 0;
}

int main(int argc, char** argv) {
    if(argc!=2) printf("zdlv-packer [.adv]\n"), exit(EXIT_FAILURE);
    pack_script(argv[1]);
    return EXIT_SUCCESS;
}
