#include "cdlv.h"
#include "cdlv_util.h"
#include "menu/cdlv_menu.h"
#include "zocket/zocket.h"

struct scene_paths {
    char** images;
    uint16_t image_count;
};

struct image {
    char* data;
    size_t size;
};

static inline char* read_file(const char* path, size_t* buf_size) {
    FILE* file = fopen(path, "rb");
    if(!file) {
        perror("cdlv");
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char* code = malloc(size+1);
    if(!code) {
        fclose(file), cdlv_logv("Could not allocate memory for file: %s", path);
        return NULL;
    }

    if(fread(code, size, 1, file) != 1) {
        fclose(file), free(code), cdlv_logv("Could not read file: %s", path);
        return NULL;
    }

    fclose(file);
    code[size] = '\0';
    *buf_size = size+1;
    return code;
}

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
        images[i].data = read_file(scene->images[i], &size);
        images[i].size = size;
    }
    
    zkt_data** images_ = calloc(scene->image_count, sizeof(zkt_data*));
    for(size_t i=0; i<scene->image_count; i++)
        images_[i] = zkt_data_compress(images[i].data, images[i].size, 90);

    for(size_t i=0; i<scene->image_count; i++) free(images[i].data);
    free(images);

    size_t size = 0;
    for(size_t i=0; i<scene->image_count; i++) size += 7 + strlen(scene->images[i]) + images_[i]->size; //IMG0000PATHBIN

    zkt_data* ret = malloc(sizeof(zkt_data));
    ret->buffer = malloc(size);
    char* buffer = malloc(size);
    ret->size = size;
    size_t offset = 0;
    for(size_t i=0; i<scene->image_count; i++) {
        memcpy(buffer+offset, "IMG", 3);
        offset += 3;
        const int n = snprintf(NULL, 0, "%lu", strlen(scene->images[i]));
        char str[n+1];
        snprintf(str, n+1, "%lu", strlen(scene->images[i]));
        memcpy(buffer+offset, str, n);
        offset += n;
        memcpy(buffer+offset, scene->images[i], strlen(scene->images[i]));
        offset += strlen(scene->images[i]);
        memcpy(buffer+offset, images_[i]->buffer, images_[i]->size);
        offset += images_[i]->size;
    }
    memcpy(ret->buffer, buffer, size);

    free(buffer);
    for(size_t i=0; i<scene->image_count; i++)
        zkt_data_clean(images_[i]);
    free(images_);

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

static int pack_script(const char* file, const char* name) {
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

    size_t size = 0;
    zkt_data* ret = malloc(sizeof(zkt_data));
    for(size_t i=0; i<scene_count; i++) 
        size += 3 + scenes_data[i]->size; // SCNZKT

    ret->buffer = malloc(size);
    ret->size = size;
    char* buffer = malloc(size);
    size_t offset = 0;

    for(size_t i=0; i<scene_count; i++) {
        memcpy(buffer+offset, "SCN", 3);
        offset += 3;
        memcpy(buffer+offset, scenes_data[i]->buffer, scenes_data[i]->size);
        offset += scenes_data[i]->size;
    }

    memcpy(ret->buffer, buffer, size);

    for(size_t i=0; i<scene_count; i++)
        zkt_data_clean(scenes_data[i]);
    free(scenes_data);
    free(buffer);

    zkt_data* compressed = zkt_data_compress(ret->buffer, ret->size, 90);

    write_file_zkt_data(name, compressed);
    zkt_data_clean(ret);
    zkt_data_clean(compressed);

    for(size_t i=0; i<scene_count; i++) {
        for(size_t j=0; j<scenes[i]->image_count; j++) free(scenes[i]->images[j]);
        free(scenes[i]->images);
        free(scenes[i]);
    }
    free(scenes);
    return 0;
}

int main(int argc, char** argv) {
    if(argc!=3) printf("zdlv-packer [.adv] [.adv.zkt]\n"), exit(EXIT_FAILURE);
    pack_script(argv[1], argv[2]);
    return EXIT_SUCCESS;
}
