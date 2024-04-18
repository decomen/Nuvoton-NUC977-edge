#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "das_util.h"

struct das_romfs_file_node {
    const char *path;
    const int length;
    const char *data;
};

#include "das_romfs_web.c"

int das_romfs_web_init(const char *dir_path)
{
    const struct das_romfs_file_node *files = s_das_web_files;
    char file_path[256];
    while (files->path) {
        FILE *fp = NULL;
        sprintf(file_path, "%s/%s", dir_path, files->path);
        das_mkdir_p(file_path, 0755);
        fp  = fopen(file_path, "w+");
        if(fp == NULL){
            printf("open %s failed\r\n", file_path);
            return -1;
        }
        fwrite(files->data, files->length, 1, fp);
        fclose(fp);
        files++;
    }
}

