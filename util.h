#ifndef CDLV_UTIL_H
#define CDLV_UTIL_H

#include "cdlv.h"

#define cdlv_logv(msg, ...) ({if(base->config.log_buffer && base->config.log_callback) snprintf(base->config.log_buffer, base->config.log_buffer_size, msg, __VA_ARGS__), base->config.log_callback(base->config.log_buffer);})
#define cdlv_log(msg) ({if(base->config.log_buffer && base->config.log_callback) snprintf(base->config.log_buffer, base->config.log_buffer_size, msg), base->config.log_callback(base->config.log_buffer);})
#define cdlv_err(err) ({base->error = err; return err;})

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

#endif
