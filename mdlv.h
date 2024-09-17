#ifndef MDLV_H
#define MDLV_H

#include "mongoose/mongoose.h"
#include "common.h"

typedef struct mdlv {
    char* host;
    char* path;
    struct mg_mgr manager;
} mdlv;

void mdlv_init(mdlv* base);
void mdlv_free(mdlv* base);

#endif
