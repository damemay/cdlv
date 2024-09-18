#include "mdlv.h"
#include "mongoose/mongoose.h"

// endpoints:
// /list - no json - lists scripts from mdlv->path
// /script - json : {"filename":"*.cdlv", "line":1} - get line from script or get script info when line is not in json
// /anim - json : {"filename":"*"} - get animation file
// /bg - json : {"filename":"*"} - get image file

static void fn(struct mg_connection* c, int ev, void* ev_data) {
    mdlv* base = (mdlv*) c->fn_data;
    if(ev == MG_EV_HTTP_MSG) {
	struct mg_http_message* hm = (struct mg_http_message*)ev_data;
	if(mg_match(hm->uri, mg_str("/list"), NULL)) {
	    DIR* dir = opendir(base->path);
	    if(!dir) {
		char* error = mg_mprintf("Could not open %s", base->path);
	        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
			"{%m: %m}", MG_ESC("error"), MG_ESC(error));
		free(error);
	        return;
	    }
	    char files[cdlv_max_string_size];
	    strcpy(files, "[");
	    struct dirent* entry;
	    int first = 1;
	    while((entry=readdir(dir)) != NULL) {
	        if(!strcmp(entry->d_name,".") || !strcmp(entry->d_name, "..") || !strstr(entry->d_name, ".cdlv")) continue;
	        if(!first) strcat(files,",");
	        else first = 0;
	        strcat(files, "\"");
	        strcat(files,entry->d_name);
	        strcat(files, "\"");
	    }
	    strcat(files, "]");
	    closedir(dir);
	    mg_http_reply(c, 200, "Content-Type: application/json\r\n",
		    "{%m: %.*s}", MG_ESC("list"), strlen(files), files);
	} else if(mg_match(hm->uri, mg_str("/script"), NULL)) {
	    struct mg_str json = hm->body;
	    char* filename = mg_json_get_str(json, "$.filename");
	    if(!filename) {
		mg_http_reply(c, 400, "Content-Type: application/json\r\n",
			"{%m: %m}", MG_ESC("error"), MG_ESC("No filename found"));
		return;
	    }
	    struct mg_str format = mg_str("*.cdlv");
	    if(!mg_match(mg_str(filename), format, NULL)) {
		char* error = mg_mprintf("%.*s is not .cdlv file", strlen(filename), filename);
		mg_http_reply(c, 400, "Content-Type: application/json\r\n",
			"{%m: %m}", MG_ESC("error"), MG_ESC(error));
		free(filename);
		free(error);
		return;
	    }

	    char* file_path = mg_mprintf("%s%.*s", base->path, strlen(filename), filename);
	    struct mg_str script = mg_file_read(&mg_fs_posix, file_path);
	    if(!script.buf) {
		free(filename);
		char* error = mg_mprintf("Could not read %.*s", strlen(file_path), file_path);
	        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
			"{%m: %m}", MG_ESC("error"), MG_ESC(error));
		free(error);
		free(file_path);
	        return;
	    }
	    free(filename);
	    free(file_path);
	    long line = mg_json_get_long(json, "$.line", -1);
	    if(line == -1) {
		mg_http_reply(c, 400, "Content-Type: application/json\r\n",
			"{%m: %m}", MG_ESC("error"), MG_ESC("No line found"));
		return;
	    }

	    char* line_str = strtok(script.buf, "\r\n");
	    size_t i = 0;
	    while(line_str) {
		if(i==line) break;
		i++;
		line_str = strtok(NULL, "\r\n");
	    }
	    mg_http_reply(c, 200, "Content-Type: application/json\r\n",
		    "{%m: %m}", MG_ESC("line"), MG_ESC(line_str+strspn(line_str, " \t")));
	    free(script.buf);
	} else if(mg_match(hm->uri, mg_str("/anim"), NULL)) {
	    struct mg_str json = hm->body;
	    char* filename = mg_json_get_str(json, "$.filename");
	    if(!filename) {
		mg_http_reply(c, 400, "Content-Type: application/json\r\n",
			"{%m: %m}", MG_ESC("error"), MG_ESC("No filename found"));
		return;
	    }
	    char* file_path = mg_mprintf("%s%.*s", base->path, strlen(filename), filename);
	    struct mg_str img = mg_file_read(&mg_fs_posix, file_path);
	    if(!img.buf) {
		char* error = mg_mprintf("Could not read %.*s", strlen(file_path), file_path);
	        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
			"{%m: %m}", MG_ESC("error"), MG_ESC(error));
		free(error);
		free(file_path);
	        return;
	    }
	    free(filename);
	    free(file_path);
	    mg_printf(c,
	            "HTTP/1.0 200 OK\r\n"
	            "Cache-Control: no-cache\r\n"
	            "Pragma: no-cache\r\nExpires: Thu, 01 Dec 1994 16:00:00 GMT\r\n"
	            "Content-Type: video/mp4\r\n"
	            "Content-Length: %lu\r\n\r\n",
	            (unsigned long) img.len);
	    mg_send(c, img.buf, img.len);
	    mg_send(c, "\r\n", 2);
	    free(img.buf);
	} else if(mg_match(hm->uri, mg_str("/bg"), NULL)) {
	    struct mg_str json = hm->body;
	    char* filename = mg_json_get_str(json, "$.filename");
	    if(!filename) {
		mg_http_reply(c, 400, "Content-Type: application/json\r\n",
			"{%m: %m}", MG_ESC("error"), MG_ESC("No filename found"));
		return;
	    }
	    char* file_path = mg_mprintf("%s%.*s", base->path, strlen(filename), filename);
	    struct mg_str img = mg_file_read(&mg_fs_posix, file_path);
	    if(!img.buf) {
		char* error = mg_mprintf("Could not read %.*s", strlen(file_path), file_path);
	        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
			"{%m: %m}", MG_ESC("error"), MG_ESC(error));
		free(error);
		free(file_path);
	        return;
	    }
	    free(filename);
	    free(file_path);
	    mg_printf(c,
	            "HTTP/1.0 200 OK\r\n"
	            "Cache-Control: no-cache\r\n"
	            "Pragma: no-cache\r\nExpires: Thu, 01 Dec 1994 16:00:00 GMT\r\n"
	            "Content-Type: image/jpeg\r\n"
	            "Content-Length: %lu\r\n\r\n",
	            (unsigned long) img.len);
	    mg_send(c, img.buf, img.len);
	    mg_send(c, "\r\n", 2);
	    free(img.buf);
	} else {
	    struct mg_http_serve_opts opts = {.root_dir = "mdlv_root"};
	    mg_http_serve_dir(c, ev_data, &opts);
	}
    }
}

void mdlv_init(mdlv* base) {
    mg_mgr_init(&base->manager);
    mg_http_listen(&base->manager, base->host, fn, base);
}

void mdlv_free(mdlv* base) {
    mg_mgr_free(&base->manager);
}
