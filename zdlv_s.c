#include "zocket/zocket.h"
#include "zdlv.h"
#include <dirent.h>
#include <string.h>

#define max_string_size 5120

char path[max_string_size];

void callback(int fd) {
    zkt_data* cmd = zkt_data_recv(fd);
    if(!cmd) zkt_err("could not receive command");
    zkt_vlog("received command: %s", (char*)cmd->buffer);

    if(strcmp(cmd->buffer, "LIST") == 0) {
        DIR* dir = opendir(path);
        if(!dir) return;

        char files[max_string_size];
        strcpy(files, "");
        struct dirent* entry;
        while((entry = readdir(dir)) != NULL) {
            if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || !strstr(entry->d_name, ".adv"))
                continue;
            strcat(files, entry->d_name);
            strcat(files, "\r\n");
        }
        zkt_vlog("%s", files);
        zkt_data_send_compress(fd, files, max_string_size, 90);
    } else if(strstr(cmd->buffer, "GETI")) {
        char fpath[max_string_size];
        sprintf(fpath, "%s/%s", path, ((char*)cmd->buffer)+5);
        zkt_vlog("%s", fpath);
        size_t size;
        char* file = zdlv_read_file(fpath, &size);
        zkt_data_send_compress(fd, file, size, 90);
        free(file);
    } else if(strstr(cmd->buffer, "GET")) {
        char fpath[max_string_size];
        sprintf(fpath, "%s/%s", path, ((char*)cmd->buffer)+4);
        zkt_vlog("%s", fpath);
        size_t size;
        char* file = zdlv_read_file(fpath, &size);
        zkt_data_send_compress(fd, file, size, 90);
        free(file);
    }

    zkt_data_clean(cmd);
}

int main(int argc, char** argv) {
    if(argc!=3) printf("%s [scripts path] [port]", argv[0]), exit(1);

    zkt_server* server = zkt_server_init(argv[2]);
    if(!server) exit(1);

    strcpy(path, argv[1]);

    while(1)
        if(zkt_server_accept(server, &callback) == -1) continue;

    zkt_server_clean(server);

    return 0;
}
