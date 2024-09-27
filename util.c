#include <errno.h>
#include <string.h>
#include "util.h"
#include "resource.h"

void dic_forEach(cdlv_dict* dic, cdlv_foreach f, void *user) {
    for(size_t i=0; i<dic->len; i++) {
	struct sdic_i* it = (struct sdic_i*)dic->data[i];
	f(it->key, it->value, user);
    }
}

static inline char* find_first_whitespace(const char* line) {
    char* whitespace = strchr(line, ' ');
    if(whitespace) return whitespace;
    whitespace = strchr(line, '\t');
    if(whitespace) return whitespace;
    return NULL;
}

cdlv_error cdlv_extract_from_quotes(cdlv* base, const char* line, char** output) {
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

cdlv_error cdlv_extract_non_quote(cdlv* base, const char* line, char** output) {
    char* start_position = NULL;
    bool scene_name = line[0] == '!' || line[0] == '@' ? true : false;
    if(scene_name) start_position = find_first_whitespace(line);
    else start_position = (char*)line;
    char* end_position = find_first_whitespace(start_position+1);
    if(!end_position) {
        end_position = start_position + strlen(line);
        //cdlv_logv("Did not find name on line: %s", line);
        //cdlv_err(cdlv_parse_error);
    }
    size_t size = scene_name ? end_position-start_position : end_position-start_position + 1;
    char str_content[size];
    sprintf(str_content, "%.*s", (int)size-1, scene_name ? start_position+1 : start_position);
    cdlv_strdup(output, str_content, size);
    cdlv_err(cdlv_ok);
}

cdlv_error cdlv_extract_filename(cdlv* base, const char* line, char** output) {
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

static inline cdlv_error cdlv_extract_file_extension(cdlv* base, const char* line, char** output) {
    char* dot_position = strrchr(line, '.');
    if(!dot_position) {
        cdlv_logv("Did not find filename on line: %s", line);
        cdlv_err(cdlv_parse_error);
    }
    size_t size = strlen(line)-(dot_position-line)+1;
    char str_content[size];
    sprintf(str_content, "%.*s", (int)(size-1), dot_position);
    cdlv_strdup(output, str_content, size);
    cdlv_err(cdlv_ok);
}

cdlv_error cdlv_extract_resource_kv(cdlv* base, const char* line, char** key, char** value) {
    cdlv_error res;
    if(line[0] == '\"') {
        if((res = cdlv_extract_from_quotes(base, line, value)) != cdlv_ok) cdlv_err(res);
        if((res = cdlv_extract_filename(base, *value, key)) != cdlv_ok) cdlv_err(res);
        cdlv_err(cdlv_ok);
    } else {
        if((res = cdlv_extract_non_quote(base, line, key)) != cdlv_ok) cdlv_err(res);
        if((res = cdlv_extract_from_quotes(base, line, value)) != cdlv_ok) cdlv_err(res);
        cdlv_err(cdlv_ok);
    }
    cdlv_logv("Could not parse resource: %s", line);
    cdlv_err(cdlv_parse_error);
}

cdlv_error cdlv_strcat_new(cdlv* base, const char* first, const char* second, char** output) {
    char new[strlen(first)+strlen(second)+1];
    sprintf(new, "%s%s", first, second);
    if(*output) free(*output);
    cdlv_strdup(output, new, strlen(new)+1);
    cdlv_err(cdlv_ok);
}

cdlv_error cdlv_add_new_resource(cdlv* base, const char* base_path, const char* line, cdlv_dict* resources) {
    cdlv_error res;
    char* key, *path;
    cdlv_resource* resource;
    if((res = cdlv_extract_resource_kv(base, line, &key, &path)) != cdlv_ok) cdlv_err(res);
    if((res = cdlv_strcat_new(base, base_path, path, &path)) != cdlv_ok) cdlv_err(res);
    char* file_extension;
    if((res = cdlv_extract_file_extension(base, path, &file_extension)) != cdlv_ok) cdlv_err(res);
    bool is_image = strcmp(".jpg", file_extension) == 0 || strcmp(".jpeg", file_extension) == 0 || strcmp(".png", file_extension) == 0;
    if(is_image) {
        if((res = cdlv_resource_new(base, cdlv_resource_image, path, &resource)) != cdlv_ok) cdlv_err(res);
    } else {
        if((res = cdlv_resource_new(base, cdlv_resource_video, path, &resource)) != cdlv_ok) cdlv_err(res);
    }
    free(file_extension);
    sdic_add(resources, key, resource);
    cdlv_err(cdlv_ok);
}

cdlv_error cdlv_add_new_resource_from_path(cdlv* base, const char* base_path, const char* name, const char* path, cdlv_dict* resources) {
    cdlv_error res;
    char* endpath = NULL;
    cdlv_resource* resource;
    if((res = cdlv_strcat_new(base, base_path, path, &endpath)) != cdlv_ok) cdlv_err(res);
    char* file_extension;
    if((res = cdlv_extract_file_extension(base, path, &file_extension)) != cdlv_ok) cdlv_err(res);
    bool is_image = strcmp(".jpg", file_extension) == 0 || strcmp(".jpeg", file_extension) == 0 || strcmp(".png", file_extension) == 0;
    if(is_image) {
        if((res = cdlv_resource_new(base, cdlv_resource_image, endpath, &resource)) != cdlv_ok) cdlv_err(res);
    } else {
        if((res = cdlv_resource_new(base, cdlv_resource_video, endpath, &resource)) != cdlv_ok) cdlv_err(res);
    }
    free(file_extension);
    sdic_add(resources, name, resource);
    cdlv_err(cdlv_ok);
}

cdlv_error cdlv_read_file_to_str(cdlv* base, const char* path, char** string) {
    FILE* file = fopen(path, "rb");
    if(!file) {
        cdlv_logv("%s",strerror(errno));
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
        cdlv_logv("%s",strerror(errno));
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
