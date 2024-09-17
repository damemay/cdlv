#include "mdlv.h"
#include "mongoose/mongoose.h"

static void fn(struct mg_connection* c, int ev, void* ev_data) {
    mdlv* base = (mdlv*) ev_data;
    if(ev == MG_EV_HTTP_MSG) {
	struct mg_http_message* hm = (struct mg_http_message*)base->manager;
	if(mg_match(hm->uri, mg_str("/list"), NULL)) {
	    DIR* dir = opendir(base->path);
	    if(!dir) {
	        mg_http_reply(c, 500, NULL, "could not open %s", base->path);
	        return;
	    }

	    char files[cdlv_max_string_size];
	    strcpy(files, "[");
	    struct dirent* entry;
	    int first = 1;
	    while((entry=readdir(dir)) != NULL) {
	        if(!strcmp(entry->d_name,".") || !strcmp(entry->d_name, "..") || !strstr(entry->d_name, ".adv")) continue;
	        if(!first) strcat(files,",");
	        else first = 0;
	        strcat(files, "\"");
	        strcat(files,entry->d_name);
	        strcat(files, "\"");
	    }
	    strcat(files, "]");
	    mg_http_reply(c, 200, NULL, files);
	} else if(mg_match(hm->uri, mg_str("/script"), NULL)) {
	    struct mg_str format = mg_str("*.adv");
	    if(!mg_match(hm->query, format, NULL)) {
		mg_http_reply(c, 400, NULL, "%.*s is not a .adv file", (int)hm->query.len, hm->query.buf);
		return;
	    }
	    char file_path[cdlv_max_string_size];
	    sprintf(file_path, "%s%.*s", base->path, (int)hm->query.len, hm->query.buf);
	    struct mg_str script = mg_file_read(&mg_fs_posix, file_path);
	    if(!script.buf) {
	        mg_http_reply(c, 500, NULL, "could not read %s", file_path);
	        return;
	    }
	    mg_http_reply(c, 200, NULL, "%.*s", (int)script.len, script.buf);
	} else if(mg_match(hm->uri, mg_str("/anim"), NULL)) {
	    char file_path[cdlv_max_string_size];
	    sprintf(file_path, "%s%.*s", base->path, (int)hm->query.len, hm->query.buf);
	    struct mg_str img = mg_file_read(&mg_fs_posix, file_path);
	    if(!img.buf) {
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
	    mg_send(c, img.buf, img.len);
	    mg_send(c, "\r\n", 2);
	} else if(mg_match(hm->uri, mg_str("/bg"), NULL)) {
	    char file_path[cdlv_max_string_size];
	    sprintf(file_path, "%s%.*s", base->path, (int)hm->query.len, hm->query.buf);
	    struct mg_str img = mg_file_read(&mg_fs_posix, file_path);
	    if(!img.buf) {
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
	    mg_send(c, img.buf, img.len);
	    mg_send(c, "\r\n", 2);
	} else {
	    struct mg_http_serve_opts opts = {.root_dir = "static"};
	    mg_http_serve_dir(c, ev_data, &opts);
	}
    }
}

void mdlv_init(mdlv* base) {
    mg_mgr_init(base->manager);
    mg_http_listen(base->manager, base->host, fn, base);
}

void mdlv_free(mdlv* base) {
    mg_mgr_free(base->manager);
}
