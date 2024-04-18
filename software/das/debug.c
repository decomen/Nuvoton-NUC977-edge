#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{  
    int tty = -1;
    char *tty_name = NULL;
  
    if(argc < 2)
    {
        printf("miss argument\n");
        return 0;
    }
    /* 获取当前tty名称 */
    tty_name = ttyname(STDOUT_FILENO);
    printf("tty_name: %s\n", tty_name);
    if (!strcmp(argv[1], "on")) {
        /* 重定向console到当前tty */
        tty = open(tty_name, O_RDONLY | O_WRONLY);
        ioctl(tty, TIOCCONS);
        perror("ioctl TIOCCONS");
    } else if(!strcmp(argv[1], "off")) {
        /* 恢复console */
        tty = open("/dev/console", O_RDONLY | O_WRONLY);
        ioctl(tty, TIOCCONS);
        perror("ioctl TIOCCONS");
    } else {  
        printf("error argument\n");
        return 0;
    }  
    close(tty);
    return 0;
}

