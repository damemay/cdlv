#include "mdlv.h"
#include "mongoose/mongoose.h"
#include "cJSON/cJSON.h"
#include "resource.h"
#include "scene.h"

static int foreach_res(void *key, int count, void **value, void *user) {
    cdlv_resource* res = (cdlv_resource*)*value;
    cJSON* resources = (cJSON*)user;
    cJSON* resource = cJSON_CreateObject();
    if(!resource) fprintf(stderr, "mdlv: cjson error\n");
    char key_buf[cdlv_max_string_size];
    snprintf(key_buf, cdlv_max_string_size-1, "%.*s", count, (char*)key);
    if(!cJSON_AddStringToObject(resource, key_buf, res->path))
	fprintf(stderr, "mdlv: cjson error\n");
    if(!cJSON_AddItemToArray(resources, resource))
	fprintf(stderr, "mdlv: cjson error\n");
    return 1;
}

static int foreach_scene(void *key, int count, void **value, void *user) {
    cdlv_scene* scene = (cdlv_scene*)*value;
    cJSON* scenes = (cJSON*)user;
    cJSON* scene_json = cJSON_CreateObject();
    if(!scene_json)
	fprintf(stderr, "mdlv: cjson error\n");
    char key_buf[cdlv_max_string_size];
    snprintf(key_buf, cdlv_max_string_size-1, "%.*s", count, (char*)key);
    if(!cJSON_AddStringToObject(scene_json, "name", key_buf))
	fprintf(stderr, "mdlv: cjson error\n");
    cJSON* resources = cJSON_AddArrayToObject(scene_json, "resources");
    if(!resources)
	fprintf(stderr, "mdlv: cjson error\n");
    dic_forEach(scene->resources, foreach_res, resources);
    if(!cJSON_AddStringToObject(scene_json, "resources_path", scene->resources_path))
	fprintf(stderr, "mdlv: cjson error\n");
    cJSON* script = cJSON_AddArrayToObject(scene_json, "script");
    for(size_t i=0; i<scene->script->size; i++) {
	char* line = SCL_ARRAY_GET(scene->script, i, char*);
	cJSON* line_json = cJSON_CreateString(line);
	if(!line_json) fprintf(stderr, "mdlv: cjson error\n");
	if(!cJSON_AddItemToArray(script, line_json))
	    fprintf(stderr, "mdlv: cjson error\n");
    }
    if(!cJSON_AddItemToArray(scenes, scene_json))
	fprintf(stderr, "mdlv: cjson error\n");
    return 1;
}

static void fn(struct mg_connection* c, int ev, void* ev_data) {
    mdlv* base = (mdlv*) c->fn_data;
    if(ev == MG_EV_HTTP_MSG) {
	struct mg_http_message* hm = (struct mg_http_message*)ev_data;
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
		    "{%m: %m, %m: %.*s}", MG_ESC("type"), MG_ESC("list"), MG_ESC("files"), strlen(files), files);
	} else if(mg_match(hm->uri, mg_str("/script"), NULL)) {
	    struct mg_str json = hm->body;
	    char* filename = mg_json_get_str(json, "$.filename");
	    if(!filename) {
		mg_http_reply(c, 400, "Content-Type: application/json\r\n",
			"{%m: %m, %m: %m}", MG_ESC("type"), MG_ESC("error"), MG_ESC("msg"), MG_ESC("No filename found"));
		return;
	    }
	    struct mg_str format = mg_str("*.cdlv");
	    if(!mg_match(mg_str(filename), format, NULL)) {
		char* error = mg_mprintf("%.*s is not .cdlv file", strlen(filename), filename);
		mg_http_reply(c, 400, "Content-Type: application/json\r\n",
			"{%m: %m, %m: %m}", MG_ESC("type"), MG_ESC("error"), MG_ESC("msg"), MG_ESC(error));
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
		current = current->next;
	    }
	    if(!found) {
		free(filename);
		char* error = mg_mprintf("%.*s is not listed in loaded scripts", strlen(filename), filename);
	        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
			"{%m: %m, %m: %m}", MG_ESC("type"), MG_ESC("error"), MG_ESC("msg"), MG_ESC(error));
		free(error);
	        return;
	    }

	    cJSON* script_info = cJSON_CreateObject();
	    if(!script_info) {
	        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
			"{%m: %m, %m: %m}", MG_ESC("type"), MG_ESC("error"), MG_ESC("msg"), MG_ESC("cjson error"));
	        return;
	    }
	    if(!cJSON_AddStringToObject(script_info, "type", "script")) {
	        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
			"{%m: %m, %m: %m}", MG_ESC("type"), MG_ESC("error"), MG_ESC("msg"), MG_ESC("cjson error"));
	        return;
	    }
	    cJSON* global_resources = cJSON_AddArrayToObject(script_info, "global_resources");
	    if(!global_resources) {
	        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
			"{%m: %m, %m: %m}", MG_ESC("type"), MG_ESC("error"), MG_ESC("msg"), MG_ESC("cjson error"));
	        return;
	    }
	    dic_forEach(current->instance->resources, foreach_res, global_resources);
	    cJSON* scenes = cJSON_AddArrayToObject(script_info, "scenes");
	    if(!scenes) {
	        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
			"{%m: %m, %m: %m}", MG_ESC("type"), MG_ESC("error"), MG_ESC("msg"), MG_ESC("cjson error"));
	        return;
	    }
	    dic_forEach(current->instance->scenes, foreach_scene, scenes);
	    char* string = cJSON_PrintUnformatted(script_info);
	    if(!string) {
	        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
			"{%m: %m, %m: %m}", MG_ESC("type"), MG_ESC("error"), MG_ESC("msg"), MG_ESC("cjson error"));
	        return;
	    }
	    mg_http_reply(c, 200, "Content-Type: application/json\r\n",
	    	"%.*s", strlen(string), string);
	    cJSON_Delete(script_info);
	    free(string);
	    return;
	} else if(mg_match(hm->uri, mg_str("/anim"), NULL)) {
	    struct mg_str json = hm->body;
	    char* filename = mg_json_get_str(json, "$.filename");
	    if(!filename) {
		mg_http_reply(c, 400, "Content-Type: application/json\r\n",
			"{%m: %m, %m: %m}", MG_ESC("type"), MG_ESC("error"), MG_ESC("msg"), MG_ESC("No filename found"));
		return;
	    }
	    struct mg_str img = mg_file_read(&mg_fs_posix, filename);
	    if(!img.buf) {
		char* error = mg_mprintf("Could not read %.*s", strlen(filename), filename);
	        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
			"{%m: %m, %m: %m}", MG_ESC("type"), MG_ESC("error"), MG_ESC("msg"), MG_ESC(error));
		free(error);
	        return;
	    }
	    free(filename);
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
			"{%m: %m, %m: %m}", MG_ESC("type"), MG_ESC("error"), MG_ESC("msg"), MG_ESC("No filename found"));
		return;
	    }
	    struct mg_str img = mg_file_read(&mg_fs_posix, filename);
	    if(!img.buf) {
		char* error = mg_mprintf("Could not read %.*s", strlen(filename), filename);
	        mg_http_reply(c, 500, "Content-Type: application/json\r\n",
			"{%m: %m, %m: %m}", MG_ESC("type"), MG_ESC("error"), MG_ESC("msg"), MG_ESC(error));
		free(error);
	        return;
	    }
	    free(filename);
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
	    struct mg_http_serve_opts opts = {.root_dir = base->web_root};
	    mg_http_serve_dir(c, hm, &opts);
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
