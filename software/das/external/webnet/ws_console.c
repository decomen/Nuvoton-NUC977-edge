
#include <queue.h>
#include <board.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int s_websock_telnet_fd = -1;
static rt_thread_t s_websock_telnet_read_thread = NULL;

static void websock_telnet_read_thread(void *parameter)
{
    uint8_t buffer[2048];
    printf("websock_pty_read_thread start\n");
    while (1) {
        if (s_websock_telnet_fd >= 0) {
            int size = read(s_websock_telnet_fd, buffer, sizeof(buffer));
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
    if (s_websock_telnet_fd >= 0) {
        lwip_send(s_websock_telnet_fd, buffer, size, MSG_NOSIGNAL);
    }
}

void ws_console_init( void )
{
    
}

int ws_console_open( void )
{
    ws_console_close();
    s_websock_telnet_fd = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (s_websock_telnet_fd < 0) {
        elog_e("wsconsole", "socket failed, err:%d", lwip_get_error(s_websock_telnet_fd));
        goto __END;
    }
    
    struct sockaddr_in peer;
    peer.sin_family         = AF_INET;
    peer.sin_port           = htons(23);
    peer.sin_addr.s_addr    = inet_addr("127.0.0.1");
    printf("ws_console_open lwip_connect\n");
    if (lwip_connect(s_websock_telnet_fd, (struct sockaddr *)&peer, sizeof(struct sockaddr)) >= 0) {
        ;
    } else {
        elog_i("wsconsole", "connect failed, err:%d", lwip_get_error(s_websock_telnet_fd));
        goto __END;
    }
    s_websock_telnet_read_thread = rt_thread_create("wsconsole", websock_telnet_read_thread, NULL, 0, 0, 0);
    if (s_websock_telnet_read_thread) rt_thread_startup(s_websock_telnet_read_thread);

    printf("ws_console_open return 0\n");
    return 0;
    
__END:
    ws_console_close();
    printf("ws_console_open return -1\n");
    return -1;
}

void ws_console_close( void )
{
    if (s_websock_telnet_read_thread) {
        rt_thread_delete(s_websock_telnet_read_thread);
        s_websock_telnet_read_thread = NULL;
    }
    if (s_websock_telnet_fd >= 0) {
        close(s_websock_telnet_fd);
        s_websock_telnet_fd = -1;
    }
    my_system("debug off");
}

