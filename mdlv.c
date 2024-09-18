#include "mdlv.h"
#include "mongoose/mongoose.h"
#include "resource.h"
#include "scene.h"

static int foreach_res(void *key, int count, void **value, void *user) {
    cdlv_resource* res = (cdlv_resource*)*value;
    char* buf = (char*)user;
    char inbuf[cdlv_max_string_size];
    snprintf(inbuf, cdlv_max_string_size-1, "\"%.*s\":\"%s\",", count, (char*)key, res->path);
    strcat(buf, inbuf);
    return 1;
}

static int foreach_scene(void *key, int count, void **value, void *user) {
    cdlv_scene* scene = (cdlv_scene*)*value;
    char* buf = (char*)user;
    char inbuf[cdlv_max_string_size];
    snprintf(inbuf, cdlv_max_string_size-1, "\"%.*s\": [", count, (char*)key);
    strcat(buf, inbuf);
    dic_forEach(scene->resources, foreach_res, buf);
    strcat(buf, "],");
    return 1;
}

static void fn(struct mg_connection* c, int ev, void* ev_data) {
    mdlv* base = (mdlv*) c->fn_data;
    if(ev == MG_EV_HTTP_MSG) {
	struct mg_http_message* hm = (struct mg_http_message*)ev_data;
	// /list - no json - lists scripts from mdlv->path
	if(mg_match(hm->uri, mg_str("/list"), NULL)) {
	    char files[cdlv_max_string_size];
	    strcpy(files, "[");
	    mdlv_script_t* current = base->scripts;
	    for(size_t i = 0; current != NULL; i++) {
		if(i != 0) strcat(files, ",");
	        strcat(files, "\"");
	        strcat(files, current->name);
	        strcat(files, "\"");
		current = current->next;
	    }
	    strcat(files, "]");
	    mg_http_reply(c, 200, "Content-Type: application/json\r\n",
		    "{%m: %.*s}", MG_ESC("list"), strlen(files), files);
	// /script - json : {"filename":"*.cdlv", "line":1} - get line from script or get script info when line is not in json
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
	    mdlv_script_t* current = base->scripts;
	    bool found = false;
	    while(current != NULL) {
		if(strcmp(current->name, filename) == 0) {
		    found = true;
		    break;
		}
	    }
	    if(!found) {
		free(filename);
		char* error = mg_mprintf("%.*s is not listed in loaded scripts", strlen(filename), filename);
	        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
			"{%m: %m}", MG_ESC("error"), MG_ESC(error));
		free(error);
	        return;
	    }

	    char* file_path = mg_mprintf("%s%.*s", base->path, strlen(current->name), current->name);
	    struct mg_str script = mg_file_read(&mg_fs_posix, file_path);
	    if(!script.buf) {
		char* error = mg_mprintf("Could not read %.*s", strlen(file_path), file_path);
	        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
			"{%m: %m}", MG_ESC("error"), MG_ESC(error));
		free(error);
		free(file_path);
	        return;
	    }
	    free(file_path);
	    long line = mg_json_get_long(json, "$.line", -1);
	    if(line == -1) {
		char res[cdlv_max_string_size];
		char sce[cdlv_max_string_size];
		dic_forEach(current->instance->resources, foreach_res, res);
		dic_forEach(current->instance->scenes, foreach_scene, sce);
		char* msg = mg_mprintf("\"global_resources\": [%.*s], %.*s",
			strlen(res), res, strlen(sce), sce);
		mg_http_reply(c, 200, "Content-Type: application/json\r\n",
			"{%m: {%.*s}}", MG_ESC("script"), strlen(msg), msg);
		free(msg);
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
	// /anim - json : {"filename":"*"} - get animation file
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
	// /bg - json : {"filename":"*"} - get image file
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

cdlv_error mdlv_init(mdlv* base) {
    DIR* dir = opendir(base->path);
    if(!dir) return cdlv_file_error;
    struct dirent* entry;
    base->scripts = malloc(sizeof(mdlv_script_t));
    if(!base->scripts) return cdlv_memory_error;
    mdlv_script_t* current = base->scripts;
    size_t i = 0;
    while((entry=readdir(dir)) != NULL) {
        if(!strcmp(entry->d_name,".") || !strcmp(entry->d_name, "..") || !strstr(entry->d_name, ".cdlv")) continue;
	if(i != 0) {
	    current->next = malloc(sizeof(mdlv_script_t));
	    if(!current->next) return cdlv_memory_error;
	    current = current->next;
	}
	current->name = calloc(strlen(entry->d_name)+1, sizeof(char));
	if(!current->name) return cdlv_memory_error;   
	strcpy(current->name, entry->d_name);                
	current->instance = malloc(sizeof(cdlv));
	if(!current->instance) return cdlv_memory_error;
    	cdlv_init(current->instance, 0, 0);
	char path[cdlv_max_string_size];
	snprintf(path, cdlv_max_string_size-1, "%s%s", base->path, current->name);
	cdlv_add_script(current->instance, path);
	current->next = NULL;
	i++;
    }
    closedir(dir);
    mg_mgr_init(&base->manager);
    mg_http_listen(&base->manager, base->host, fn, base);
    return cdlv_ok;
}

void mdlv_free(mdlv* base) {
    for(mdlv_script_t* current = base->scripts; current != NULL; current = current->next) {
	free(current->name);
	cdlv_free(current->instance);
    }
    mg_mgr_free(&base->manager);
}
