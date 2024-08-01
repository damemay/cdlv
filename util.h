#ifndef CDLV_UTIL_H
#define CDLV_UTIL_H

#include "cdlv.h"

#define cdlv_logv(msg, ...) sprintf(base->log, msg, __VA_ARGS__)
#define cdlv_log(msg) sprintf(base->log, "%.*s", strlen(msg) > cdlv_max_string_size ? (int)cdlv_max_string_size : (int)strlen(msg), msg)
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

#endif