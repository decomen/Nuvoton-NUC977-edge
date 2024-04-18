/*
 * File      : startup.c
 */

#include <board.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

extern int  rt_application_init(void);

static int __check_das_self(void)
{
#define DAS_LOCK_FILE       "/var/run/das.pid"
    int fd;
    int lock_result;
    struct flock lock;
    char pid_str[32] = {0};
    
    fd = open(DAS_LOCK_FILE, O_RDWR | O_CREAT);
    if (fd < 0) {
        printf ("open %s failed (%s).\n", DAS_LOCK_FILE, strerror(errno));
        return -1;
    }
    lock_result = lockf(fd, F_TLOCK, 0);
    if (lock_result < 0) {
        read(fd, pid_str, sizeof(pid_str));
        printf ("das is already running (pid : %s).\n", pid_str);
        return -1;
    }
    sprintf(pid_str, "%d", getpid());
    ftruncate(fd, 0);
    write(fd, pid_str, strlen(pid_str));
    printf ("das running (pid : %s).\n", pid_str);
    return 0;
}

void rtthread_startup(void)
{
    das_os_init();
    
    /* init board */
    rt_hw_board_init();

    /* init application */
    rt_application_init();

    return ;
}

void das_show_version(void)
{
    char cmd[1024] = "echo '";
    char buf[32] = {0};
    int pos = 6;    // echo '
    time_t t = time(0);
    ctime_r(&t, buf);
    if (buf[0]) buf[strlen(buf) - 1] = '\0';
    pos += sprintf(&cmd[pos], "==================================================\n");
    pos += sprintf(&cmd[pos], "=============%s=============\n", buf);
    pos += sprintf(&cmd[pos], "==================================================\n");
    pos += sprintf(&cmd[pos], "  ___    _    ___ \n");
    pos += sprintf(&cmd[pos], " |   \\  /_\\  / __|\n");
    pos += sprintf(&cmd[pos], " | |) |/ _ \\ \\__ \\\n");
    pos += sprintf(&cmd[pos], " |___//_/ \\_\\|___/\n\n");
    pos += sprintf(&cmd[pos], " Version: %d.%02d build %s, %s\n", DAS_VER_VERCODE / 100, DAS_VER_VERCODE % 100, __DATE__, __TIME__);
    pos += sprintf(&cmd[pos], " 2018 - 2020 Copyright by jay\n");
    pos += sprintf(&cmd[pos], "==================================================\n");

    printf("%s\n", &cmd[6]);
    
    pos += sprintf(&cmd[pos], "' > /tmp/das_version.log");
    my_system(cmd);
}

int main(void)
{
    if (__check_das_self() != 0) {
        return -1;
    }

   // extern void mbus603_test(void);
   // mbus603_test();

    
    das_show_version();
    rtthread_startup();
    
    for (;;) sleep(1000);
}

