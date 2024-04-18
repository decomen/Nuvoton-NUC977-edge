
#include <queue.h>
#include <board.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include <pty.h>


static int s_websock_pty_m_fd = -1;
static int s_websock_pty_s_fd = -1;
static char s_websock_pty_name[128] = {0};
static rt_thread_t s_websock_pty_read_thread = NULL;

static void websock_pty_read_thread(void *parameter)
{
    uint8_t buffer[2048];
    printf("websock_pty_read_thread start\n");
    while (1) {
        if (s_websock_pty_s_fd >= 0) {
            int size = read(s_websock_pty_s_fd, buffer, sizeof(buffer));
            printf("websock_pty_read_thread read : %d\n", size);
            if (size > 0) {
                ws_vm_rcv_write(0, (void *)buffer, (rt_size_t)size);
            }
        } else {
            sleep(2);
        }
    }
}

void ws_console_recv_pack(void *buffer, int size)
{
    
}

void ws_console_init( void )
{
    int rc = openpty(&s_websock_pty_m_fd, &s_websock_pty_s_fd, s_websock_pty_name, NULL, NULL);
    if (rc != -1) {
        const char *pty_name = ptsname(s_websock_pty_m_fd);
        printf("Name of slave side is <%s> fd = %d/n", pty_name, s_websock_pty_s_fd);
        strcpy(s_websock_pty_name, pty_name);
        s_websock_pty_read_thread = rt_thread_create("ws_pty", websock_pty_read_thread, NULL, 0, 0, 0);
        if (s_websock_pty_read_thread) rt_thread_startup(s_websock_pty_read_thread);
	} else {
        perror("ws_console_init");
    }
}

int ws_console_open( void )
{
    if (s_websock_pty_s_fd >= 0) {
        ioctl(s_websock_pty_s_fd, TIOCCONS);
        return 0;
    }
    return -1;
}

void ws_console_close( void )
{
    int tty = open("/dev/ttyS0", O_RDONLY | O_WRONLY);
    if (tty >= 0) {
        ioctl(tty, TIOCCONS); 
        close(tty);
    }
}

