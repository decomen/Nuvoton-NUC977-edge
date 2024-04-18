
#include<unistd.h>
#include<signal.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/param.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<time.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<limits.h>

#define BUFSZ                   256

#define TOOL_NAME               "das"
//#define TOOL_BASE_PATH          "/home/jay/ubuntu_share/work1/dm/das_rtu/"
#define TOOL_BASE_PATH          "/usr/fs/bin/"
#define TOOL_LOG_PATH           "/media/nand/data/"

int does_service_work(void)
{
    FILE* fp;
    char buf[BUFSZ];
    int count;
    char command[256];

    sprintf(command, "ps -ef | grep " TOOL_NAME " | grep -v grep | grep -v das_daemon | wc -l");
    if((fp = popen(command, "r")) != NULL) {
        if((fgets(buf,BUFSZ,fp))!= NULL) {
            count = atoi(buf);
        }
        pclose(fp);
        return count;
    }

    return 0;
}

int main(void)
{
    FILE *fp;
    time_t t;
    int count;

    while(1) {
        sleep(2);
        fp = fopen(TOOL_LOG_PATH TOOL_NAME ".log", "a");
        if(fp) {
            count = does_service_work();
            //printf("count = %d\n", count);
            time(&t);
            if(count > 0) {
                //fprintf(fp,"current time is:%s and the process exists, the count is %d\n", asctime(localtime(&t)), count);
            } else {
                printf("current time is:%s and the process does not exist, restart it!\n", asctime(localtime(&t)));
                fprintf(fp,"current time is:%s and the process does not exist, restart it!\n", asctime(localtime(&t)));
                system(TOOL_BASE_PATH TOOL_NAME " &");
            }
            fclose(fp);
        }
    }
    return 0;
}

