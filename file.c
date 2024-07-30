#include "file.h"

char* cdlv_read_file_to_str(const char* path) {
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
    return code;
}

char** cdlv_read_file_in_lines(const char* path, size_t* line_count) {
    FILE* file = fopen(path, "rb");
    if(!file) {
        perror("cdlv");
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char** code = malloc(size+1);
    if(!code) {
        fclose(file), cdlv_logv("Could not allocate memory for file: %s", path);
        return NULL;
    }
    
    char* temp = malloc(size+1);
    if(!temp) {
        fclose(file), cdlv_logv("Could not allocate memory for file: %s", path);
        return NULL;
    }

    if(fread(temp, size, 1, file) != 1) {
        fclose(file), free(temp), cdlv_logv("Could not read file: %s", path);
        return NULL;
    }
    temp[size] = '\0';

    char* line = strtok(temp, "\r\n");
    size_t i = 0;
    while(line) {
        cdlv_duplicate_string(&code[i], line, strlen(line)+1);
        ++i;
        line = strtok(NULL, "\r\n");
    }

    if(feof(file)) fclose(file);
    free(temp);

    *line_count = i;
    return code;
}

void cdlv_free_file_in_lines(char** file, const size_t line_count) {
    for(size_t i=0; i<line_count; ++i) free(file[i]);
    free(file);
}
