#include "file.h"
#include "util.h"
#include <errno.h>

cdlv_error cdlv_read_file_to_str(cdlv* base, const char* path, char** string) {
    FILE* file = fopen(path, "rb");
    if(!file) {
        cdlv_log(strerror(errno));
        cdlv_err(cdlv_file_error);
    }

    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char* code = malloc(size+1);
    if(!code) {
        fclose(file), cdlv_logv("Could not allocate memory for file: %s", path);
        cdlv_err(cdlv_memory_error);
    }

    if(fread(code, size, 1, file) != 1) {
        fclose(file), free(code), cdlv_logv("Could not read file: %s", path);
        cdlv_err(cdlv_read_error);
    }

    fclose(file);
    code[size] = '\0';
    *string = code;

    cdlv_err(cdlv_ok);
}

cdlv_error cdlv_read_file_in_lines(cdlv* base, const char* path, size_t* line_count, char*** string_array) {
    FILE* file = fopen(path, "rb");
    if(!file) {
        cdlv_log(strerror(errno));
        cdlv_err(cdlv_file_error);
    }

    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char** code = malloc(size+1);
    if(!code) {
        fclose(file), cdlv_logv("Could not allocate memory for file: %s", path);
        cdlv_err(cdlv_memory_error);
    }
    
    char* temp = malloc(size+1);
    if(!temp) {
        fclose(file), cdlv_logv("Could not allocate memory for file: %s", path);
        cdlv_err(cdlv_memory_error);
    }

    if(fread(temp, size, 1, file) != 1) {
        fclose(file), free(temp), cdlv_logv("Could not read file: %s", path);
        cdlv_err(cdlv_read_error);
    }
    temp[size] = '\0';

    char* line = strtok(temp, "\r\n");
    size_t i = 0;
    while(line) {
        cdlv_strdup(&code[i], line, strlen(line)+1);
        ++i;
        line = strtok(NULL, "\r\n");
    }

    if(feof(file)) fclose(file);
    free(temp);

    *line_count = i;
    *string_array = code;

    cdlv_err(cdlv_ok);
}

void cdlv_free_file_in_lines(char** file, const size_t line_count) {
    for(size_t i=0; i<line_count; ++i) free(file[i]);
    free(file);
}
