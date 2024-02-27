#include "mongoose.h"
#include <dirent.h>
#include <string.h>

#define MAX_SIZE    5120
char path[10] = "scripts/";

struct folder {
    char* path;
    char** files;
    uint16_t count;
};

struct folder* folder(char* path, char* list, size_t count) {
    struct folder* new = malloc(sizeof(struct folder));
    if(!new) return NULL;
    new->count = count;
    new->path = calloc(strlen(path)+1, sizeof(char));
    if(!new->path) return NULL;
    strcpy(new->path, path);
    new->files = calloc(new->count, sizeof(char*));
    if(!new->files) return NULL;

    char* tok = strtok(list, " ");
    size_t i = 0;
    while(tok!=NULL) {
	new->files[i] = calloc(strlen(tok)+1, sizeof(char));
	if(!new->files[i]) return NULL;
	strcpy(new->files[i], tok);
	i++;
	if(i>count) break;
    }

    return new;
}

void defolder(struct folder* folder) {
    for(uint16_t i=0; i<folder->count; i++)
	free(folder->files[i]);
    free(folder->files);
    free(folder->path);
    free(folder);
}

static void fn(struct mg_connection *c, int ev, void *ev_data) {
    if(ev == MG_EV_OPEN) {
	c->fn_data = NULL;
    } else if (ev == MG_EV_HTTP_MSG) {
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
	    char folder_path[MAX_SIZE];
	    sprintf(folder_path, "%s%.*s", path, (int)hm->query.len, hm->query.ptr);
	    DIR* dir = opendir(folder_path);
	    if(!dir) {
		mg_http_reply(c, 500, NULL, "could not open %s", path);
		return;
	    }
	    char files[MAX_SIZE];
	    strcpy(files, "");
	    struct dirent* entry;
	    size_t count = 0;
	    while((entry=readdir(dir)) != NULL) {
		if(!strcmp(entry->d_name,".") || !strcmp(entry->d_name, "..") || !strstr(entry->d_name, ".jpg"))
		    continue;
		strcat(files,entry->d_name);
		strcat(files, " ");
		count++;
	    }

	    if(c->fn_data) defolder(c->fn_data);
	    c->fn_data = folder(folder_path, files, count);
	    if(!c->fn_data) {
		mg_http_reply(c, 500, NULL, "could not list files\r\n%.*s", MAX_SIZE, files);
		return;
	    }

	    c->data[0] = 'S';
	    mg_printf(c,
		"HTTP/1.0 200 OK\r\n"
		"Cache-Control: no-cache\r\n"
		"Pragma: no-cache\r\nExpires: Thu, 01 Dec 1994 16:00:00 GMT\r\n"
		"Content-Type: multipart/x-mixed-replace; boundary=--foo\r\n\r\n");
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
		    "--foo\r\nContent-Type: image/jpeg\r\n"
		    "Content-Length: %lu\r\n\r\n",
		    (unsigned long) img.len);
	    mg_send(c, img.ptr, img.len);
	    mg_send(c, "\r\n", 2);
	} else {
	    struct mg_http_serve_opts opts = {.root_dir = "static"};
	    mg_http_serve_dir(c, ev_data, &opts);
	}

    } else if(ev == MG_EV_CLOSE) {
	if(c->fn_data) defolder(c->fn_data);
	c->fn_data = NULL;
    }
}

static void broadcast_mjpeg_frame(struct mg_mgr* mgr) {
    struct mg_connection* c;
    for(c=mgr->conns; c!=NULL; c=c->next) {
	if(c->data[0]!='S') continue;
	if(!c->fn_data) continue;
	struct folder* folder = (struct folder*) c->fn_data;
	static size_t i;
	const char* fpath = folder->files[i++%folder->count];
	struct mg_str data = mg_file_read(&mg_fs_posix, fpath);
	if(!data.ptr || !data.len) continue;
	mg_printf(c,
	    "--foo\r\nContent-Type: image/jpeg\r\n"
            "Content-Length: %lu\r\n\r\n",
            (unsigned long) data.len);
	mg_send(c, data.ptr, data.len);
	mg_send(c, "\r\n", 2);
    }
}

static void timer_cb(void* arg) {
    broadcast_mjpeg_frame(arg);
}

int main(int argc, char *argv[]) {
    struct mg_mgr mgr;
    mg_log_set(3);
    mg_mgr_init(&mgr);                                        
    mg_http_listen(&mgr, "http://localhost:8000", fn, &mgr);
    mg_timer_add(&mgr, 1000/60, MG_TIMER_REPEAT, timer_cb, &mgr);
    puts("polling...");
    for (;;) mg_mgr_poll(&mgr, 1000);                         
    mg_mgr_free(&mgr);                                        
    return 0;
}
