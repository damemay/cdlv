#pragma once
#include "cdlv.h"
#include <sys/stat.h>

static inline char* zdlv_get(void** input, size_t* size, char* find, size_t* ret_s) {
    size_t start = 0, end = 0, i = 0;
    char* input_ = *input;
    size_t size_ = *size;
    while(i<size_) {
        if(input_[i] == find[0]) {
            size_t count = 0;
            size_t len = strlen(find);
            for(size_t c=0; find[c]!='\0'; c++)
                if(input_[i+c] == find[c]) count++;
            if(count == len) {
                i += count;
                if(start == 0) {
                    start = i;
                    continue;
                } else {
                    end = i-count;
                    break;
                }
            }
        }
        i++;
    }

    if(start == 0 && end == 0) return NULL;

    if(end == 0) end = size_;
    size_t ret_size = end-start;
    char* ret = malloc(ret_size);
    memcpy(ret, input_+start, ret_size);

    size_t changed_size = size_-ret_size;
    char* changed = malloc(changed_size);
    memcpy(changed, input_+ret_size, changed_size);
    free(*input);

    *input = changed;
    *size = changed_size;
    *ret_s = ret_size;

    return ret;
}

static inline char* zdlv_read_file(const char* path, size_t* buf_size) {
    FILE* file = fopen(path, "rb");
    if(!file) {
        cdlv_logv("%s", path);
        perror("fopen");
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char* code = malloc(size);
    if(!code) {
        fclose(file), cdlv_logv("Could not allocate memory for file: %s", path);
        return NULL;
    }

    if(fread(code, size, 1, file) != 1) {
        fclose(file), free(code), cdlv_logv("Could not read file: %s", path);
        return NULL;
    }

    fclose(file);
    *buf_size = size;
    return code;
}

