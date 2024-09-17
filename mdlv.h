#ifndef MDLV_H
#define MDLV_H

#include "mongoose/mongoose.h"
#include "common.h"
#include "cdlv.h"

typedef struct mdlv {
    cdlv* cdlv;
    char host[cdlv_max_string_size];
    char path[cdlv_max_string_size];
    struct mg_mgr* manager;
} mdlv;

void mdlv_init(mdlv* base);
void mdlv_free(mdlv* base);

#endif
