#ifndef MDLV_H
#define MDLV_H

#include "mongoose/mongoose.h"
#include "common.h"
#include "cdlv.h"

typedef struct mdlv_script {
    char* name;
    cdlv* instance;
    struct mdlv_script* next;
} mdlv_script_t;

typedef struct mdlv {
    char* host;
    char* path;
    char* web_root;
    mdlv_script_t* scripts;
    struct mg_mgr manager;
} mdlv;

cdlv_error mdlv_init(mdlv* base);
void mdlv_free(mdlv* base);

#endif
