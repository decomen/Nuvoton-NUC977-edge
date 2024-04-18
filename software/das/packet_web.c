
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>


#include <fcntl.h>

static char data_buf[1024*1024] = {0};

static int file_index = 0;

static int gen_file(FILE *fp, int index,const char *filepath)
{
    char *path = strchr(filepath,'/');
    char path_cmd[64] = {0};
    if ((filepath[0] == '.' && filepath[1] == '/') || filepath[0] == '/') {
        path++;
        path = strchr(path,'/');
    }
    sprintf(path_cmd,"static const char path_%d[] = \"%s\";\n",index,++path);
    fputs(path_cmd,fp);
   // printf("%s\n",path_cmd);

    
    int len = 0;
    struct stat state;
    if(stat(filepath,&state) == 0){
		len = state.st_size;
    }
    //char len_cmd[64] = {0};
    //sprintf(len_cmd,"static const int length_%d = %d;\n",index,len);
    //fputs(len_cmd,fp);
   // printf("%s\n",len_cmd);


    memset(data_buf,0,sizeof(data_buf));
    sprintf(data_buf,"static const char file_%d[] = {\n",index);
    int fd = open(filepath, O_RDONLY);
    if (fd < 0){  
         printf("open %s error\n",filepath);
         return -1;
    }
    
    unsigned char data[1024 * 100] = {0};
   // unsigned char data_tmp[1024 * 100] = 0;
    
    int n = 0;
    int pos = 0;
    char *p = &data_buf[strlen(data_buf)];
    while(1){
        n = read(fd, data,sizeof(data));
        if(n > 0){
            char tmp[10] = {0};
            for(int i = 0; i < n; i++){
                p += sprintf(p,"0x%02x,",data[i]);
                pos++;
                if(pos % 40 == 0){
                    p += sprintf(p,"\n");
                }
            }
        }
        if(n <= 0) break;
    }
    close(fd);
    
    strcpy(&(data_buf[p - data_buf - 1]),"\n};\n");
    fputs(data_buf,fp);
    //printf("%s\n",data_buf);

    
}

static int readFileList(char *basePath, FILE *fp)
{
    DIR *dir;
    struct dirent *ptr;
    char base[1000];

    if ((dir=opendir(basePath)) == NULL)
    {
        perror("Open dir error...");
        exit(1);
    }

    while ((ptr=readdir(dir)) != NULL)
    {
        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    ///current dir OR parrent dir
            continue;
        else if(ptr->d_type == 8){    ///file
            char d_name[256] = {0};
            sprintf(d_name,"%s/%s",basePath,ptr->d_name);
            //printf("d_name:%s\n",d_name);
            gen_file(fp, file_index++, d_name);
            fputs("\n",fp);
        }else if(ptr->d_type == 10) {   ///link file
            //printf("d_name:%s/%s\n",basePath,ptr->d_name);
        }else if(ptr->d_type == 4) {   ///dir
            memset(base,0,sizeof(base));
            strcpy(base,basePath);
            strcat(base,"/");
            strcat(base,ptr->d_name);
            readFileList(base,fp);
        }
    }
    closedir(dir);
    return 1;
}



int main(int argc, char* argv[])
{
    if(argc != 3){
        printf("usage:%s [src_path] [des_path]\r\n",argv[0]);
        return -1;
    }
    
    file_index = 0;

    FILE *fp = NULL;
    fp  = fopen(argv[2],"w+");
    if(fp == NULL){
        printf("open %s failed\r\n",argv[2]);
        return -1;
    }
    readFileList(argv[1],fp);

    fputs("\nconst struct das_romfs_file_node s_das_web_files[] = {\n",fp);

    int i;
    char das_buf[64] = {0};
    for( i = 0 ; i < file_index; i++){
        memset(das_buf,0,sizeof(das_buf));
        sprintf(das_buf,"{path_%d,sizeof(file_%d),file_%d},\n",i,i,i);
        fputs(das_buf,fp);
    }

    fputs("{NULL,0,NULL},\n};",fp);

    
    
    fclose(fp);
    return 0;
}

