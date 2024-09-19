#include "mdlv.h"

int main(int argc, char** argv) {
    char host[cdlv_max_string_size];
    char path[cdlv_max_string_size];
    char web_root[cdlv_max_string_size];
    if(argc < 4) {
	fprintf(stderr, "Usage: %s <host> <path> <web_root>\r\n <host>\te.g. http://localhost:8080\r\n <path>\tpath to directory with script resources, e.g. ../res/sample/\r\n <web_root\tabsolute path to web_root\r\n", argv[0]);
	exit(1);
    } else {
	strncpy(host, argv[1], cdlv_max_string_size-1);
	strncpy(path, argv[2], cdlv_max_string_size-1);
	strncpy(web_root, argv[3], cdlv_max_string_size-1);
    }
    mdlv mdlv_base = {
	.host = host,
	.path = path,
	.web_root = web_root
    };
    if(mdlv_init(&mdlv_base) != cdlv_ok) exit(1);
    for(;;) mg_mgr_poll(&mdlv_base.manager, 50);
    mdlv_free(&mdlv_base);
}
