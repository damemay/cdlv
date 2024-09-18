#include "mdlv.h"

int main(int argc, char** argv) {
    char host[cdlv_max_string_size];
    char path[cdlv_max_string_size];
    if(argc < 3) {
	fprintf(stderr, "Usage: %s <host> <path>\r\n <host>\te.g. http://localhost:8080\r\n <path>\tpath to directory with script resources, e.g. ../res/sample/\r\n", argv[0]);
	exit(1);
    } else {
	strncpy(host, argv[1], cdlv_max_string_size-1);
	strncpy(path, argv[2], cdlv_max_string_size-1);
    }
    mdlv mdlv_base = {
	.host = host,
	.path = path,
    };
    if(mdlv_init(&mdlv_base) != cdlv_ok) exit(1);
    for(;;) mg_mgr_poll(&mdlv_base.manager, 50);
    mdlv_free(&mdlv_base);
}
