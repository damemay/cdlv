#ifndef CDLV_MACROS_H
#define CDLV_MACROS_H

#define diev(msg, ...) printf("[DIE] " msg "\n", __VA_ARGS__), exit(EXIT_FAILURE)
#define die(msg) printf("[DIE] " msg "\n")
#define logv(msg, ...) printf("[LOG] " msg "\n", __VA_ARGS__)
#define log(msg) printf("[LOG] " msg "\n")

#define cdlv_tag_scene       "!scene"
#define cdlv_tag_bg          "!bg"
#define cdlv_tag_anim        "!anim"
#define cdlv_tag_anim_once   "!anim_once"
#define cdlv_tag_script      "!script"

#define duplicate_string(dest, src, size)   \
{                                           \
    *dest = malloc(size);                   \
    if(!dest)                               \
        die("Could not "                    \
        "allocate destination for "         \
        "string duplication!");             \
    strcpy(*dest, src);                     \
}
#define alloc_ptr_arr(arr_ptr, count, size_type)    \
{                                                   \
    *arr_ptr = calloc(count, sizeof(size_type));    \
    if(!*arr_ptr)                                   \
        die("Could not allocate "                   \
        "memory for array!");                       \
}

#endif
