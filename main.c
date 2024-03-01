#include "mongoose.h"
#include <dirent.h>
#include <string.h>
#include <sys/types.h>

#define MAX_SIZE    5120
char path[10] = "scripts/";
char host[32] = "http://localhost:8000";

static void fn(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
	struct mg_http_message* hm = (struct mg_http_message*) ev_data;
	if(mg_http_match_uri(hm, "/list")) {
	    DIR* dir = opendir(path);
	    if(!dir) {
		mg_http_reply(c, 500, NULL, "could not open %s", path);
		return;
	    }

	    char files[MAX_SIZE];
	    strcpy(files, "[");
	    struct dirent* entry;
	    int first = 1;
	    while((entry=readdir(dir)) != NULL) {
		if(!strcmp(entry->d_name,".") || !strcmp(entry->d_name, "..") || !strstr(entry->d_name, ".adv"))
		    continue;
		if(!first) strcat(files,",");
		else first = 0;
		strcat(files, "\"");
		strcat(files,entry->d_name);
		strcat(files, "\"");
	    }
	    strcat(files, "]");
	    mg_http_reply(c, 200, NULL, files);
	} else if(mg_http_match_uri(hm, "/script")) {
	    struct mg_str format = mg_str(".adv");
	    if(!mg_strstr(hm->query, format)) {
		mg_http_reply(c, 400, NULL, "%.*s is not a .adv file",
			(int)hm->query.len, hm->query.ptr);
		return;
	    }

	    char file_path[MAX_SIZE];
	    sprintf(file_path, "%s%.*s", path, (int)hm->query.len, hm->query.ptr);
	    struct mg_str script = mg_file_read(&mg_fs_posix, file_path);
	    if(!script.ptr) {
		mg_http_reply(c, 500, NULL, "could not read %s", file_path);
		return;
	    }

	    mg_http_reply(c, 200, NULL, "%.*s",
		    (int)script.len, script.ptr);
	} else if(mg_http_match_uri(hm, "/anim")) {
	    char file_path[MAX_SIZE];
	    sprintf(file_path, "%s%.*s", path, (int)hm->query.len, hm->query.ptr);
	    struct mg_str img = mg_file_read(&mg_fs_posix, file_path);
	    if(!img.ptr) {
		mg_http_reply(c, 500, NULL, "could not read %s", file_path);
		return;
	    }
	    mg_printf(c,
		    "HTTP/1.0 200 OK\r\n"
		    "Cache-Control: no-cache\r\n"
		    "Pragma: no-cache\r\nExpires: Thu, 01 Dec 1994 16:00:00 GMT\r\n"
		    "Content-Type: video/mp4\r\n"
		    "Content-Length: %lu\r\n\r\n",
		    (unsigned long) img.len);
	    mg_send(c, img.ptr, img.len);
	    mg_send(c, "\r\n", 2);
	} else if(mg_http_match_uri(hm, "/bg")) {
	    char file_path[MAX_SIZE];
	    sprintf(file_path, "%s%.*s", path, (int)hm->query.len, hm->query.ptr);
	    struct mg_str img = mg_file_read(&mg_fs_posix, file_path);
	    if(!img.ptr) {
		mg_http_reply(c, 500, NULL, "could not read %s", file_path);
		return;
	    }
	    mg_printf(c,
		    "HTTP/1.0 200 OK\r\n"
		    "Cache-Control: no-cache\r\n"
		    "Pragma: no-cache\r\nExpires: Thu, 01 Dec 1994 16:00:00 GMT\r\n"
		    "Content-Type: image/jpeg\r\n"
		    "Content-Length: %lu\r\n\r\n",
		    (unsigned long) img.len);
	    mg_send(c, img.ptr, img.len);
	    mg_send(c, "\r\n", 2);
	} else {
	    struct mg_http_serve_opts opts = {.root_dir = "static"};
	    mg_http_serve_dir(c, ev_data, &opts);
	}
    }
}

int main(int argc, char *argv[]) {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);                                        
    mg_http_listen(&mgr, host, fn, &mgr);
    for (;;) mg_mgr_poll(&mgr, 50);                         
    mg_mgr_free(&mgr);                                        
    return 0;
}
