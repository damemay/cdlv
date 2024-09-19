#ifndef CDLV_UTIL_H
#define CDLV_UTIL_H

#include <stdio.h>
#include "cdlv.h"

#define cdlv_logv(msg, ...) ({if(base->log_config.buffer && base->log_config.callback) snprintf(base->log_config.buffer, base->log_config.buffer_size, msg, __VA_ARGS__), base->log_config.callback(base->log_config.buffer);})
#define cdlv_log(msg) ({if(base->log_config.buffer && base->log_config.callback) snprintf(base->log_config.buffer, base->log_config.buffer_size, msg), base->log_config.callback(base->log_config.buffer);})
#define cdlv_err(err) ({if(base->error_config.callback) base->error_config.callback(err, base->error_config.user_data); return err;})

#define cdlv_strdup(dest, src, size)            \
{                                               \
    *dest = calloc(size, sizeof(char));         \
    if(!*dest) {                                \
        cdlv_log("Could not "                   \
        "allocate destination for "             \
        "string duplication!");                 \
        cdlv_err(cdlv_memory_error);            \
    }                                           \
    strcpy(*dest, src);                         \
}

cdlv_error cdlv_extract_from_quotes(cdlv* base, const char* line, char** output);
cdlv_error cdlv_extract_non_quote(cdlv* base, const char* line, char** output);
cdlv_error cdlv_extract_filename(cdlv* base, const char* line, char** output);
cdlv_error cdlv_extract_resource_kv(cdlv* base, const char* line, char** key, char** value);
cdlv_error cdlv_strcat_new(cdlv* base, const char* first, const char* second, char** output);
cdlv_error cdlv_add_new_resource(cdlv* base, const char* base_path, const char* line, cdlv_dict* resources);
cdlv_error cdlv_add_new_resource_from_path(cdlv* base, const char* base_path, const char* name, const char* path, cdlv_dict* resources);

cdlv_error cdlv_read_file_to_str(cdlv* base, const char* path, char** string);
cdlv_error cdlv_read_file_in_lines(cdlv* base, const char* path, size_t* line_count, char*** string_array);
void cdlv_free_file_in_lines(char** file, const size_t line_count);

#endif
