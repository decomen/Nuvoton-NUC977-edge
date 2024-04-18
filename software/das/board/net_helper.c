
#include "board.h"
#include "net_helper.h"
#include "hjt212.h"
#include "cc_bjdc.h"
#include "dm101.h"
#include "sdccp_smf.h"
#include "modbus.h"
#include <arpa/inet.h>

tcpip_state_t *g_tcpip_states[NET_HELPER_MAX];

void __xfer_rcv_frame(int n, rt_uint8_t *frame, int len);

static net_helper_t s_net_list[NET_HELPER_MAX];

static void _net_task(void *parameter);

#define net_printf(_fmt,...)    rt_kprintf( "[netthread-%s]->" _fmt, rt_thread_self()->name, ##__VA_ARGS__ )

net_helper_t *net_get_helper(int n)
{
    if(n >= 0 && n < NET_HELPER_MAX) {
        return &s_net_list[n];
    } else {
        return RT_NULL;
    }
}

void net_init_all(void)
{
    int n, cli;
    for (n = 0; n < NET_HELPER_MAX; n++) {
        net_helper_t *net = &s_net_list[n];
        memset(net, 0, sizeof(*net));
        net->sock_fd = -1;
        for (cli = 0; cli < NET_CLI_NUMS; cli++) {
            net->tcp_fd[cli] = -1;
        }
        g_tcpip_states[n] = RT_NULL;
    }
}

tcpip_state_t *net_tcpip_state_get(int n, int s)
{
    if (s >= 0) {
        rt_enter_critical();
        tcpip_state_t *p = g_tcpip_states[n];
        while (p) {
            if (p->s == s) break;
            p = p->next;
        }
        rt_exit_critical();
        return p;
    }
    return RT_NULL;
}

tcpip_state_t *net_tcpip_state_get_by_port(int n, int port)
{
    if (port > 0) {
        rt_enter_critical();
        tcpip_state_t *p = g_tcpip_states[n];
        while (p) {
            if (p->s >= 0 && p->usRemPort == port) break;
            p = p->next;
        }
        rt_exit_critical();
        return p;
    }
    return RT_NULL;
}

tcpip_state_t *net_tcpip_state_open(int n, int s)
{
    if (s >= 0) {
        rt_enter_critical();
        tcpip_state_t *p = g_tcpip_states[n];
        tcpip_state_t *last = p;
        while (p) {
            if (p->s == s || p->s == -1) break;
            last = p;
            p = p->next;
        }
        if (p == RT_NULL) {
            p = calloc(sizeof(tcpip_state_t), 1);
            if (p) {
                p->s = s;
                p->next = NULL;
                if (last) {
                    last->next = p;
                } else {
                    g_tcpip_states[n] = p;
                }
            }
        } else if (p->s == -1) {
            p->s = s;
        }
        rt_exit_critical();
        return p;
    }
    return RT_NULL;
}

void net_tcpip_state_close(int n, int s)
{
    if (s >= 0) {
        rt_enter_critical();
        tcpip_state_t *p = g_tcpip_states[n];
        while (p) {
            if (p->s == s) {
                tcpip_state_t *next = p->next;
                memset(p, 0, sizeof(tcpip_state_t));
                p->s = -1;
                p->next = next;
                break;
            }
            p = p->next;
        }
        rt_exit_critical();
    }
}

rt_bool_t net_open(int n, int bufsz, int stack_size, const char *prefix)
{
    int cli;
    net_helper_t *net = &s_net_list[n];
    net->sock_fd = -1;
    for (cli = 0; cli < NET_CLI_NUMS; cli++) {
        net->tcp_fd[cli] = -1;
    }

    net_close(n);
    
    net->disconnect = RT_FALSE;
    net->shutdown   = RT_FALSE;
    net->buffer     = RT_KERNEL_CALLOC(bufsz);
    net->bufsz      = bufsz;
    
    BOARD_CREAT_NAME(net_mutex_name, "%s_%d", prefix, n);
    if((net->mutex = rt_mutex_create(net_mutex_name, RT_IPC_FLAG_FIFO)) == RT_NULL) {
        net_printf("init %s failed\n", net_mutex_name);
        return RT_FALSE;
    }
    
    BOARD_CREAT_NAME(net_thread_name, "%s_%d", prefix, n);
    net->thread = rt_thread_create(net_thread_name, _net_task, (void *)(long)n, stack_size, 20, 20);
    if(net->thread != RT_NULL) {
        rt_thddog_register(net->thread, 60);
        rt_thread_startup(net->thread);
    }
    return RT_TRUE;
}

rt_bool_t net_isconnect(int n)
{
    net_helper_t *net = &s_net_list[n];
    int cli;
    for (cli = 0; cli < NET_CLI_NUMS; cli++) {
        if (net->tcp_fd[0] >= 0) break;
    }
    return (cli < NET_CLI_NUMS);
}

rt_bool_t net_waitconnect(int n)
{
    while(!net_isconnect(n)) {
        rt_thread_delay(RT_TICK_PER_SECOND);
        rt_thddog_feed(RT_NULL);
    }
    
    return RT_TRUE;
}

void net_disconnect(int n)
{
    net_helper_t *net = &s_net_list[n];
    net->disconnect = RT_TRUE;

    for(int i = 0; i < 50; i++) {
        int cli;
        for (cli = 0; cli < NET_CLI_NUMS; cli++) {
            if (net->tcp_fd[0] >= 0) break;
        }
        if (cli < NET_CLI_NUMS) {
            rt_thread_delay(RT_TICK_PER_SECOND / 10);
        } else {
            break;
        }
        rt_thddog_feed(RT_NULL);
    }
}

void net_close(int n)
{
    net_helper_t *net = &s_net_list[n];

    net->shutdown = RT_TRUE;
    net_disconnect(n);

    for(int i = 0; i < 50; i++) {
        if (net->thread) {
            rt_thread_delay(RT_TICK_PER_SECOND / 10);
        } else {
            break;
        }
        rt_thddog_feed(RT_NULL);
    }

    if(net->thread) {
        rt_thddog_unregister(net->thread);
        if (RT_EOK == rt_thread_delete(net->thread)) {
            net->thread = RT_NULL;
        }
    }

    if(net->buffer) RT_KERNEL_FREE(net->buffer);
    net->buffer = RT_NULL;
    
    if(net->mutex) rt_mutex_delete(net->mutex);
    net->mutex = RT_NULL;
}

// n 从 0 开始
void ws_net_try_send(int n, rt_uint16_t port, void *buffer, rt_size_t size)
{
    net_helper_t *net = &s_net_list[n];
    if(g_tcpip_cfgs[n].enable) {
        rt_mutex_take(net->mutex, RT_WAITING_FOREVER);
        switch( g_tcpip_cfgs[n].tcpip_type ) {
        case TCP_IP_TCP:
        {
            tcpip_state_t *ts = net_tcpip_state_get_by_port(n, port);
            if (ts && ts->s >= 0) lwip_send(ts->s, buffer, size, MSG_NOSIGNAL);
            break;
        }
        case TCP_IP_UDP:
            if (g_tcpip_cfgs[n].port == port) {
                lwip_sendto(net->tcp_fd[0], buffer, size, MSG_NOSIGNAL, (struct sockaddr *)&net->to_addr, sizeof(struct sockaddr));
            }
            break;
        }
        rt_mutex_release(net->mutex);
    }
}

int net_send(int n, int cli, const rt_uint8_t *buffer, rt_uint16_t bufsz)
{
    int sendlen = -1;
    net_helper_t *net = &s_net_list[n];
    tcpip_type_e tcpip_type = g_tcpip_cfgs[n].tcpip_type;

    if(net->tcp_fd[cli] >= 0) {
        if (g_ws_cfg.enable && (WS_PORT_NET == g_ws_cfg.port_type || WS_PORT_GPRS == g_ws_cfg.port_type)) {
            switch( g_tcpip_cfgs[n].tcpip_type ) {
            case TCP_IP_TCP:
            {
                tcpip_state_t *ts = net_tcpip_state_get_by_port(n, g_ws_cfg.listen_port);
                if (ts && ts->s >= 0) ws_vm_snd_write( 0, (void *)buffer, bufsz );
                break;
            }
            case TCP_IP_UDP:
                if (g_tcpip_cfgs[n].port == g_ws_cfg.listen_port) {
                    ws_vm_snd_write( 0, (void *)buffer, bufsz );
                }
                break;
            }
        }
        rt_mutex_take(net->mutex, RT_WAITING_FOREVER);
        switch (tcpip_type) {
        case TCP_IP_TCP:
            elog_i("tcp", "send:%d, index:%d", bufsz, n);
            sendlen = lwip_send(net->tcp_fd[cli], buffer, bufsz, MSG_NOSIGNAL);
            break;

        case TCP_IP_UDP:
            elog_i("udp", "send:%d, index:%d", bufsz, n);
            sendlen = lwip_sendto(net->tcp_fd[cli], buffer, bufsz, MSG_NOSIGNAL, (struct sockaddr *)&net->to_addr, sizeof(struct sockaddr));
            break;
        }
        rt_mutex_release(net->mutex);
    }

    return sendlen;
}

static void prvRunAsTCPServer(int n)
{
    fd_set          readset;
    struct timeval  timeout;
    net_helper_t    *net = &s_net_list[n];
    rt_uint16_t     port;
    struct sockaddr_in srv;
    rt_bool_t       keepalive;
    int             result;
    socklen_t       len;
    int             on = 1;
    const char      *intfc_name = das_do_get_net_driver_name(NET_IS_ENET(n) ? DAS_NET_TYPE_ETH : DAS_NET_TYPE_GPRS, 0);
    int             cli;

    tcpip_state_t   *ts = RT_NULL;

    net->sock_fd    = -1;
    for (cli = 0; cli < NET_CLI_NUMS; cli++) {
        net->tcp_fd[cli] = -1;
    }

    port        = g_tcpip_cfgs[n].port;
    keepalive   = g_tcpip_cfgs[n].keepalive;

    net->sock_fd = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (net->sock_fd < 0) {
        elog_e("tcpserver", "socket failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    if (intfc_name && intfc_name[0]) {
        struct ifreq intfc; memset(&intfc, 0, sizeof(intfc));
        strncpy(intfc.ifr_ifrn.ifrn_name, intfc_name, strlen(intfc_name));
        if (setsockopt(net->sock_fd, SOL_SOCKET, SO_BINDTODEVICE, (const void *)&intfc, sizeof(intfc)) < 0) {
            //elog_e("tcpserver", "SO_BINDTODEVICE failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
            //goto __END;
        }
    }
    
    if (lwip_setsockopt(net->sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        elog_e("tcpserver", "SO_REUSEADDR failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    srv.sin_addr.s_addr = htonl(INADDR_ANY);
    srv.sin_family      = AF_INET;
    srv.sin_port        = htons(port);
    if (lwip_bind(net->sock_fd, (struct sockaddr *)&srv, sizeof(struct sockaddr)) < 0) {
        elog_e("tcpserver", "bind failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    if (lwip_listen(net->sock_fd, NET_CLI_NUMS + 1) < 0) {
        elog_e("tcpserver", "listen failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    if (lwip_fcntl(net->sock_fd, F_SETFL, O_NONBLOCK) < 0) {
        elog_e("tcpserver", "O_NONBLOCK failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    net_printf("tcp server start listen\n");

    {
        ts = net_tcpip_state_open(n, net->sock_fd);
        if (ts) {
            ts->eState = TCPIP_STATE_ACCEPT;
            {
                struct sockaddr_in sa;
                len = sizeof(sa);
                if (!lwip_getsockname(net->sock_fd, (struct sockaddr *)&sa, &len)) {
                    strcpy(ts->szLocIP, inet_ntoa(sa.sin_addr));
                    ts->usLocPort = ntohs(sa.sin_port);
                }
                if (!lwip_getpeername(net->sock_fd, (struct sockaddr *)&sa, &len)) {
                    strcpy(ts->szRemIP, inet_ntoa(sa.sin_addr));
                    ts->usRemPort = ntohs(sa.sin_port);
                }
            }
        }
    }

    if(NET_IS_NORMAL(n)) {
        switch(g_tcpip_cfgs[n].cfg.normal.proto_type) {
        case PROTO_SMF:
        smf_start_work(n);
        break;
        }
    } else if(NET_IS_XFER(n)) {
        ;
    }

    while(1) {
        while(1) {
            int max_fd = net->sock_fd;
            FD_ZERO(&readset);
            FD_SET(net->sock_fd, &readset);
            for (cli = 0; cli < NET_CLI_NUMS; cli++) {
                if(net->tcp_fd[cli] >= 0) {
                    FD_SET(net->tcp_fd[cli], &readset);
                    if(net->tcp_fd[cli] > max_fd) max_fd = net->tcp_fd[cli];
                }
            }
            timeout.tv_sec = 4; timeout.tv_usec = 0;
            rt_thddog_feed("tcp accept/read before select 5 sec");
            result = lwip_select(max_fd + 1, &readset, 0, 0, &timeout);
            rt_thddog_feed("tcp accept/read after select");
            if (result < 0) {
                elog_e("tcpserver", "accept/read select error, index:%d", n);
                goto __END;
            } else if (0 == result) {
                //net_printf("tcp accept/read select timeout!\n");
                if (net->disconnect || net->shutdown) {
                    elog_i("tcpserver", "disconnect/shutdown, index:%d", n);
                    goto __END;
                }
                continue;
            }
            break;
        }

        do {
            if(net->sock_fd >= 0 && FD_ISSET(net->sock_fd, &readset)) {
                int tmp_fd = -1;
                struct sockaddr cliaddr;
                len = sizeof(struct sockaddr_in);
                rt_thddog_feed("tcp lwip_accept");
                tmp_fd = lwip_accept(net->sock_fd, &cliaddr, &len);

                if (NET_IS_XFER(n)) {
                    for (cli = 0; cli < NET_CLI_NUMS; cli++) {
                        if (net->tcp_fd[cli] >= 0) {
                            elog_w("tcpserver", "in xfer mode, close old tcp conn, index:%d", n);
                            smf_close_cli(n, cli);
                            net_tcpip_state_close(n, net->tcp_fd[cli]);
                            lwip_shutdown(net->tcp_fd[cli], SHUT_RDWR);
                            lwip_close(net->tcp_fd[cli]);
                            net->tcp_fd[cli] = -1;
                        }
                    }
                }

                for (cli = 0; cli < NET_CLI_NUMS; cli++) {
                    if (net->tcp_fd[cli] < 0) {
                        break;
                    }
                }
                
                if(cli >= NET_CLI_NUMS) {
                    if (tmp_fd >= 0) {
                        lwip_shutdown(tmp_fd, SHUT_RDWR);
                        lwip_close(tmp_fd);
                    }
                    elog_w("tcpserver", "just keep %d conns, fd:%d, index:%d", NET_CLI_NUMS, tmp_fd, n);
                } else {
                    net->tcp_fd[cli] = tmp_fd;
                    elog_i("tcpserver", "new conn, fd:%d, index:%d", tmp_fd, n);
                    if(net->tcp_fd[cli] >= 0) {
                        
                        if(NET_IS_NORMAL(n)) {
                            switch(g_tcpip_cfgs[n].cfg.normal.proto_type) {
                            case PROTO_SMF:
                            smf_new_cli(n, cli);
                            break;
                            }
                        } else if(NET_IS_XFER(n)) {
                            ;
                        }

                        ts = net_tcpip_state_open(n, net->tcp_fd[cli]);
                        if (ts) {
                            if (ts->eState != TCPIP_STATE_CONNED) {
                                ts->ulConnTime = (rt_millisecond_from_tick(rt_tick_get())) / 1000;
                            }
                            ts->eState = TCPIP_STATE_CONNED;
                            {
                                struct sockaddr_in sa;
                                len = sizeof(sa);
                                if (!lwip_getsockname(net->tcp_fd[cli], (struct sockaddr *)&sa, &len)) {
                                    strcpy(ts->szLocIP, inet_ntoa(sa.sin_addr));
                                    ts->usLocPort = ntohs(sa.sin_port);
                                }
                                if (!lwip_getpeername(net->tcp_fd[cli], (struct sockaddr *)&sa, &len)) {
                                    strcpy(ts->szRemIP, inet_ntoa(sa.sin_addr));
                                    ts->usRemPort = ntohs(sa.sin_port);
                                }
                            }
                        }
                        if (keepalive) {
                            int val = SO_KEEPALIVE;
                            lwip_setsockopt(net->tcp_fd[cli], SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));
                            {
                                int tmp = 20;   // keepIdle in seconds
                                lwip_setsockopt(net->tcp_fd[cli], IPPROTO_TCP, TCP_KEEPIDLE, (void *)&tmp, sizeof(tmp));
                                tmp = 1;        // keepInterval in seconds
                                lwip_setsockopt(net->tcp_fd[cli], IPPROTO_TCP, TCP_KEEPINTVL, (void *)&tmp, sizeof(tmp));
                                tmp = 10;       // keepCount in seconds
                                lwip_setsockopt(net->tcp_fd[cli], IPPROTO_TCP, TCP_KEEPCNT, (void *)&tmp, sizeof(tmp));
                            }
                        }
                        if (NET_IS_XFER(n)) break;
                    }
                }
            }
            
            for (cli = 0; cli < NET_CLI_NUMS; cli++) {
                if(net->tcp_fd[cli] >= 0 && FD_ISSET(net->tcp_fd[cli], &readset)) {
                    while (1) {
                        int readlen = 0;
                        rt_thddog_feed("tcp server read with take mutex");
                        if(net->mutex) rt_mutex_take(net->mutex, RT_WAITING_FOREVER);
                        //net->pos = 0;
                        readlen = lwip_read(net->tcp_fd[cli], net->buffer, net->bufsz);
                        if(net->mutex) rt_mutex_release(net->mutex);
                        if (0 == readlen) {
                            elog_w("tcpserver", "socket close with read 0, index:%d", n);
                            smf_close_cli(n, cli);
                            net_tcpip_state_close(n, net->tcp_fd[cli]);
                            lwip_shutdown(net->tcp_fd[cli], SHUT_RDWR);
                            lwip_close(net->tcp_fd[cli]);
                            net->tcp_fd[cli] = -1;
                            break;
                        } else if (readlen < 0) {
                            if (EAGAIN == lwip_get_error(net->tcp_fd[cli])) {
                                break;
                            } else {
                                elog_w("tcpserver", "socket close with read err, err:%d, index:%d", lwip_get_error(net->tcp_fd[cli]), n);
                                smf_close_cli(n, cli);
                                net_tcpip_state_close(n, net->tcp_fd[cli]);
                                lwip_shutdown(net->tcp_fd[cli], SHUT_RDWR);
                                lwip_close(net->tcp_fd[cli]);
                                net->tcp_fd[cli] = -1;
                                break;
                            }
                        } else {
                            elog_i("tcpserver", "read:%d, index:%d", readlen, n);
                            net->pos = readlen;
                            rt_thddog_feed("tcp server ws_vm_rcv_write");
                            if( g_ws_cfg.enable && (WS_PORT_NET == g_ws_cfg.port_type || WS_PORT_GPRS == g_ws_cfg.port_type)) {
                                tcpip_state_t *ts = net_tcpip_state_get_by_port(n, g_ws_cfg.listen_port);
                                if (ts && ts->s >= 0) ws_vm_rcv_write(0, net->buffer, net->pos);
                            }
                            if(NET_IS_NORMAL(n)) {
                                switch(g_tcpip_cfgs[n].cfg.normal.proto_type) {
                                case PROTO_SMF:
                                smf_put_bytes(n, cli, (rt_uint8_t *)net->buffer, net->pos);
                                break;
                                }
                            } else if(NET_IS_XFER(n)) {
                                __xfer_rcv_frame(n, (rt_uint8_t *)net->buffer, net->pos);
                            }
                            break;
                        }
                    }
                }
            }
        } while (0);
    }

__END:
    elog_i("tcpserver", "end, index:%d", n);

    //rt_mutex_take( &s_cc_mutex[index], RT_WAITING_FOREVER );
    rt_thddog_feed("tcp server end");

    if(NET_IS_NORMAL(n)) {
        switch(g_tcpip_cfgs[n].cfg.normal.proto_type) {
        case PROTO_SMF:
        smf_exit_work(n);
        break;
        }
    } else if(NET_IS_XFER(n)) {
        ;
    }

    if(net->mutex) rt_mutex_take(net->mutex, RT_WAITING_FOREVER);
    for (cli = 0; cli < NET_CLI_NUMS; cli++) {
        if (net->tcp_fd[cli] >= 0) {
            net_tcpip_state_close(n, net->tcp_fd[cli]);
            lwip_shutdown(net->tcp_fd[cli], SHUT_RDWR);
            lwip_close(net->tcp_fd[cli]);
            net->tcp_fd[cli] = -1;
        }
    }
    if (net->sock_fd >= 0) {
        net_tcpip_state_close(n, net->sock_fd);
        lwip_shutdown(net->sock_fd, SHUT_RDWR);
        lwip_close(net->sock_fd);
        net->sock_fd = -1;
    }
    if(net->mutex) rt_mutex_release(net->mutex);
}

static void prvRunAsTCPClient(int n)
{
    fd_set          readset, writeset;
    struct timeval  timeout;
    net_helper_t    *net = &s_net_list[n];
    rt_uint16_t     port;
    const char      *peer_host;
    struct sockaddr_in peer;
    rt_bool_t       keepalive;
    int             result;
    socklen_t       len;
    const char      *intfc_name = das_do_get_net_driver_name(NET_IS_ENET(n) ? DAS_NET_TYPE_ETH : DAS_NET_TYPE_GPRS, 0);
    tcpip_state_t   *ts = RT_NULL;

    net->sock_fd    = -1;
    net->tcp_fd[0]     = -1;

    port        = g_tcpip_cfgs[n].port;
    peer_host   = g_tcpip_cfgs[n].peer;
    keepalive   = g_tcpip_cfgs[n].keepalive;

    if (!inet_aton(peer_host, (struct in_addr *)&peer.sin_addr)) {
        struct hostent *host = lwip_gethostbyname(peer_host);
        if (host && host->h_addr_list && host->h_addr) {
            memcpy(&peer.sin_addr, host->h_addr, sizeof(struct in_addr));
        } else {
            goto __END;
        }
    }

    net->sock_fd = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (net->sock_fd < 0) {
        elog_e("tcpclient", "socket failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }
    
    if (intfc_name && intfc_name[0]) {
        struct ifreq intfc; memset(&intfc, 0, sizeof(intfc));
        strncpy(intfc.ifr_ifrn.ifrn_name, intfc_name, strlen(intfc_name));  
        if (setsockopt(net->sock_fd, SOL_SOCKET, SO_BINDTODEVICE, (const void *)&intfc, sizeof(intfc)) < 0) {
            //elog_e("tcpclient", "SO_BINDTODEVICE failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
            //goto __END;
        }
    }

    net_printf("tcp client start connect\n");

    {
        ts = net_tcpip_state_open(n, net->sock_fd);
        if (ts) ts->eState = TCPIP_STATE_CONNING;
    }

	timeout.tv_sec = 4; timeout.tv_usec = 0;
	if (setsockopt(net->sock_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        elog_e("tcpclient", "SO_SNDTIMEO failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }
    peer.sin_family      = AF_INET;
    peer.sin_port        = htons(port);
	if (1) {
		rt_thddog_feed("tcp clientlwip_connect");
        if (lwip_connect(net->sock_fd, (struct sockaddr *)&peer, sizeof(struct sockaddr)) >= 0) {
            ;
        } else {
            if (EINPROGRESS == lwip_get_error(net->sock_fd)) {
                net_printf("tcp client connect timeout!\n");
            } else {
                elog_i("tcpclient", "connect failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
            }
            goto __END;
        }

		if (lwip_fcntl(net->sock_fd, F_SETFL, O_NONBLOCK) < 0) {
            elog_e("tcpclient", "O_NONBLOCK failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
	        goto __END;
	    }

        // the same
        net->tcp_fd[0] = net->sock_fd;
        net->sock_fd = -1;

        elog_i("tcpclient", "new conn, fd:%d, index:%d", net->tcp_fd[0], n);
        if(NET_IS_NORMAL(n)) {
            switch(g_tcpip_cfgs[n].cfg.normal.proto_type) {
            case PROTO_CC_BJDC:
                cc_bjdc_startwork(n);
                break;
            case PROTO_HJT212:
                hjt212_startwork(n);
                break;
            case PROTO_DM101:
                dm101_startwork(n);
                break;
            }
        } else if(NET_IS_XFER(n)) {
            ;
        }

        ts = net_tcpip_state_open(n, net->tcp_fd[0]);
        if (ts) {
            if (ts->eState != TCPIP_STATE_CONNED) {
                ts->ulConnTime = (rt_millisecond_from_tick(rt_tick_get())) / 1000;
            }
            ts->eState = TCPIP_STATE_CONNED;
            {
                struct sockaddr_in sa;
                len = sizeof(sa);
                if (!lwip_getsockname(net->tcp_fd[0], (struct sockaddr *)&sa, &len)) {
                    strcpy(ts->szLocIP, inet_ntoa(sa.sin_addr));
                    ts->usLocPort = ntohs(sa.sin_port);
                }
                if (!lwip_getpeername(net->tcp_fd[0], (struct sockaddr *)&sa, &len)) {
                    strcpy(ts->szRemIP, inet_ntoa(sa.sin_addr));
                    ts->usRemPort = ntohs(sa.sin_port);
                }
            }
        }
        if (keepalive) {
            int val = SO_KEEPALIVE;
            lwip_setsockopt(net->tcp_fd[0], SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));
            {
                int tmp = 20;   // keepIdle in seconds
                lwip_setsockopt(net->tcp_fd[0], IPPROTO_TCP, TCP_KEEPIDLE, (void *)&tmp, sizeof(tmp));
                tmp = 1;        // keepInterval in seconds
                lwip_setsockopt(net->tcp_fd[0], IPPROTO_TCP, TCP_KEEPINTVL, (void *)&tmp, sizeof(tmp));
                tmp = 10;       // keepCount in seconds
                lwip_setsockopt(net->tcp_fd[0], IPPROTO_TCP, TCP_KEEPCNT, (void *)&tmp, sizeof(tmp));
            }
        }
    }

    while (1) {
        FD_ZERO(&readset);
        FD_SET(net->tcp_fd[0], &readset);
        
        rt_thddog_feed("tcp client read before select 1 sec");
        timeout.tv_sec = 4; timeout.tv_usec = 0;
        result = lwip_select(net->tcp_fd[0] + 1, &readset, 0, 0, &timeout);
        rt_thddog_feed("tcp client read after select");
        if (result < 0) {
            elog_e("tcpclient", "read select err, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
            goto __END;
        } else if (0 == result) {
            //net_printf("tcp client read select timeout!\n");
            if (net->disconnect || net->shutdown) {
                elog_i("tcpclient", "disconnect/shutdown, fd:%d, index:%d", net->tcp_fd[0], n);
                goto __END;
            }
            continue;
        }

        if (FD_ISSET(net->tcp_fd[0], &readset)) {
            while (1) {
                int readlen = 0;
                rt_thddog_feed("tcp client read with take mutex");
                if(net->mutex) rt_mutex_take(net->mutex, RT_WAITING_FOREVER);
                //net->pos = 0;
                readlen = lwip_read(net->tcp_fd[0], net->buffer, net->bufsz);
                if(net->mutex) rt_mutex_release(net->mutex);
                if (0 == readlen) {
                    elog_w("tcpclient", "socket close with read 0, fd:%d, index:%d", net->tcp_fd[0], n);
                    goto __END;
                } else if (readlen < 0) {
                    if (EAGAIN == lwip_get_error(net->tcp_fd[0])) {
                        break;
                    } else {
                        elog_w("tcpclient", "socket close with read err, err:%d, index:%d", lwip_get_error(net->tcp_fd[0]), n);
                        goto __END;
                    }
                } else {
                    elog_i("tcpclient", "read:%d, index:%d", readlen, n);
                    net->pos = readlen;
                    rt_thddog_feed("tcp client ws_vm_rcv_write");
                    if( g_ws_cfg.enable && (WS_PORT_NET == g_ws_cfg.port_type || WS_PORT_GPRS == g_ws_cfg.port_type) ) {
                        tcpip_state_t *ts = net_tcpip_state_get_by_port(n, g_ws_cfg.listen_port);
                        if (ts && ts->s >= 0) ws_vm_rcv_write(0, net->buffer, net->pos);
                    }
                    g_xDm101Status[n].dm101_lastheart = rt_tick_get();
                    g_xDm101Status[n].dm101_lastrecv = g_xDm101Status[n].dm101_lastheart;
                    if(NET_IS_NORMAL(n)) {
                        switch(g_tcpip_cfgs[n].cfg.normal.proto_type) {
                        case PROTO_MODBUS_TCP:
                        case PROTO_MODBUS_RTU_OVER_TCP:
                            /*if(PROTO_SLAVE == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                                rt_thddog_feed("xMBPortEventPost");
                                xMBPortEventPost( BOARD_UART_MAX + n, EV_FRAME_RECEIVED );
                            } else if(PROTO_MASTER == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                                rt_thddog_feed("xMBMasterPortEventPost");
                                xMBMasterPortEventPost( BOARD_UART_MAX + n, EV_MASTER_FRAME_RECEIVED );
                            }*/
                            break;
                        case PROTO_CC_BJDC:
                            rt_thddog_feed("CC_BJDC_PutBytes");
                            CC_BJDC_PutBytes(n, net->buffer, net->pos);
                            break;
                        case PROTO_HJT212:
                            rt_thddog_feed("HJT212_PutBytes");
                            HJT212_PutBytes(n, net->buffer, net->pos);
                            break;
                        case PROTO_DM101:
                            rt_thddog_feed("dm101_put_bytes");
                            dm101_put_bytes(n, net->buffer, net->pos);
                            break;
                        }
                    } else if(NET_IS_XFER(n)) {
                        __xfer_rcv_frame(n, (rt_uint8_t *)net->buffer, net->pos);
                    }

                    break;
                }
            }
        }
    }

__END:
    elog_i("tcpclient", "end, index:%d", n);

    //rt_mutex_take( &s_cc_mutex[index], RT_WAITING_FOREVER );
    rt_thddog_feed("tcp client end");

    if(NET_IS_NORMAL(n)) {
        switch(g_tcpip_cfgs[n].cfg.normal.proto_type) {
        case PROTO_CC_BJDC:
            cc_bjdc_exitwork(n);
            break;
        case PROTO_HJT212:
            hjt212_exitwork(n);
        case PROTO_DM101:
            dm101_exitwork(n);
            break;
        }
    } else if(NET_IS_XFER(n)) {
        ;
    }

    if(net->mutex) rt_mutex_take(net->mutex, RT_WAITING_FOREVER);
    if (net->tcp_fd[0] >= 0) {
        net_tcpip_state_close(n, net->tcp_fd[0]);
        lwip_shutdown(net->tcp_fd[0], SHUT_RDWR);
        lwip_close(net->tcp_fd[0]);
        net->tcp_fd[0] = -1;
    }
    if (net->sock_fd >= 0) {
        net_tcpip_state_close(n, net->sock_fd);
        lwip_shutdown(net->sock_fd, SHUT_RDWR);
        lwip_close(net->sock_fd);
        net->sock_fd = -1;
    }
    if(net->mutex) rt_mutex_release(net->mutex);
}

static void prvRunAsUDPServer(int n)
{
    fd_set          readset;
    struct timeval  timeout;
    net_helper_t    *net = &s_net_list[n];
    rt_uint16_t     port;
    struct sockaddr_in srv;
    int             result;
    socklen_t       len;
    int             on = 1;
    const char      *intfc_name = das_do_get_net_driver_name(NET_IS_ENET(n) ? DAS_NET_TYPE_ETH : DAS_NET_TYPE_GPRS, 0);

    net->sock_fd    = -1;
    net->tcp_fd[0]  = -1;

    port        = g_tcpip_cfgs[n].port;

    net->sock_fd = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if (net->sock_fd < 0) {
        elog_e("udpserver", "socket failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }
    
    if (intfc_name && intfc_name[0]) {
        struct ifreq intfc; memset(&intfc, 0, sizeof(intfc));
        strncpy(intfc.ifr_ifrn.ifrn_name, intfc_name, strlen(intfc_name));  
        if (setsockopt(net->sock_fd, SOL_SOCKET, SO_BINDTODEVICE, (const void *)&intfc, sizeof(intfc)) < 0) {
            //elog_e("udpserver", "SO_BINDTODEVICE failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
            //goto __END;
        }
    }

    if (lwip_setsockopt(net->sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        elog_e("udpserver", "SO_REUSEADDR failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    srv.sin_addr.s_addr = htonl(INADDR_ANY);
    srv.sin_family      = AF_INET;
    srv.sin_port        = htons(port);
    if (lwip_bind(net->sock_fd, (struct sockaddr *)&srv, sizeof(struct sockaddr)) < 0) {
        elog_e("udpserver", "bind failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    if (lwip_fcntl(net->sock_fd, F_SETFL, O_NONBLOCK) < 0) {
        elog_e("udpserver", "O_NONBLOCK failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    net->tcp_fd[0]  = net->sock_fd;
    net->sock_fd = -1;

    while (1) {
        FD_ZERO(&readset);
        FD_SET(net->tcp_fd[0], &readset);
        
        rt_thddog_feed("udp server recvfrom before select 1 sec");
        timeout.tv_sec = 4; timeout.tv_usec = 0;
        result = lwip_select(net->tcp_fd[0] + 1, &readset, 0, 0, &timeout);
        rt_thddog_feed("udp server recvfrom after select");
        if (result < 0) {
            elog_e("udpserver", "recvfrom select error, err:%d, index:%d", lwip_get_error(net->tcp_fd[0]), n);
            goto __END;
        } else if (0 == result) {
            //net_printf("udp server recvfrom select timeout!\n");
            if (net->disconnect || net->shutdown) {
                elog_i("udpserver", "disconnect/shutdown, fd:%d, index:%d", net->tcp_fd[0], n);
                goto __END;
            }
            continue;
        }
        
        if (FD_ISSET(net->tcp_fd[0], &readset)) {
            while (1) {
                int readlen = 0;
                struct sockaddr_in client_addr;
                len = sizeof(struct sockaddr);
                rt_thddog_feed("udp server recvfrom with take mutex");
                if(net->mutex) rt_mutex_take(net->mutex, RT_WAITING_FOREVER);
                //net->pos = 0;
                readlen = lwip_recvfrom(net->tcp_fd[0], net->buffer, net->bufsz, 0, (struct sockaddr *)&client_addr, &len);
                if(net->mutex) rt_mutex_release(net->mutex);
                if (0 == readlen) {
                    elog_w("udpserver", "close with read 0, index:%d", n);
                    goto __END;
                } else if (readlen < 0) {
                    if (EAGAIN == lwip_get_error(net->tcp_fd[0])) {
                        break;
                    } else {
                        elog_w("udpserver", "close with read err, err:%d, index:%d", lwip_get_error(net->tcp_fd[0]), n);
                        goto __END;
                    }
                } else {
                    elog_i("udpserver", "read:%d, index:%d", readlen, n);
                    net->pos = readlen;
                    net->to_addr = client_addr;
                    rt_thddog_feed("udp server ws_vm_rcv_write");
                    if( g_ws_cfg.enable && (WS_PORT_NET == g_ws_cfg.port_type || WS_PORT_GPRS == g_ws_cfg.port_type) && (port == g_ws_cfg.listen_port) ) {
                        ws_vm_rcv_write(0, net->buffer, net->pos);
                    }
                    
                    if(NET_IS_NORMAL(n)) {
                        switch(g_tcpip_cfgs[n].cfg.normal.proto_type) {
                        case PROTO_MODBUS_TCP:
                        case PROTO_MODBUS_RTU_OVER_TCP:
                            /*if(PROTO_SLAVE == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                                rt_thddog_feed("xMBPortEventPost");
                                xMBPortEventPost( BOARD_UART_MAX + n, EV_FRAME_RECEIVED );
                            } else if(PROTO_MASTER == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                                rt_thddog_feed("xMBMasterPortEventPost");
                                xMBMasterPortEventPost( BOARD_UART_MAX + n, EV_MASTER_FRAME_RECEIVED );
                            }*/
                            break;
                        case PROTO_CC_BJDC: // no support
                        case PROTO_HJT212:
                        case PROTO_DM101:
                            break;
                        }
                    } else if(NET_IS_XFER(n)) {
                        __xfer_rcv_frame(n, (rt_uint8_t *)net->buffer, net->pos);
                    }

                    break;
                }
            }
        }
    }

__END:
    net_printf("udp server end!\n");
    elog_i("udpserver", "end, index:%d", n);

    rt_thddog_feed("udp server end");

    if(NET_IS_NORMAL(n)) {
        ;
    } else if(NET_IS_XFER(n)) {
        ;
    }

    if(net->mutex) rt_mutex_take(net->mutex, RT_WAITING_FOREVER);
    if (net->tcp_fd[0] >= 0) {
        lwip_shutdown(net->tcp_fd[0], SHUT_RDWR);
        lwip_close(net->tcp_fd[0]);
        net->tcp_fd[0] = -1;
    }
    if (net->sock_fd >= 0) {
        lwip_shutdown(net->sock_fd, SHUT_RDWR);
        lwip_close(net->sock_fd);
        net->sock_fd = -1;
    }
    if(net->mutex) rt_mutex_release(net->mutex);
}

static void prvRunAsUDPClient(int n)
{
    fd_set          readset;
    struct timeval  timeout;
    net_helper_t    *net = &s_net_list[n];
    rt_uint16_t     port;
    const char      *peer_host;
    struct sockaddr_in srv;
    int             result;
    socklen_t       len;
    int             on = 1;
    const char      *intfc_name = das_do_get_net_driver_name(NET_IS_ENET(n) ? DAS_NET_TYPE_ETH : DAS_NET_TYPE_GPRS, 0);

    net->sock_fd    = -1;
    net->tcp_fd[0]     = -1;

    port        = g_tcpip_cfgs[n].port;
    peer_host   = g_tcpip_cfgs[n].peer;

    if (!inet_aton(peer_host, (struct in_addr *)&net->to_addr)) {
        struct hostent *host = lwip_gethostbyname(peer_host);
        if (host && host->h_addr_list && host->h_addr) {
            memcpy(&net->to_addr.sin_addr, host->h_addr, sizeof(struct in_addr));
        } else {
            goto __END;
        }
    }

    net->sock_fd = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if (net->sock_fd < 0) {
        elog_e("udpclient", "socket failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }
    
    if (intfc_name && intfc_name[0]) {
        struct ifreq intfc; memset(&intfc, 0, sizeof(intfc));
        strncpy(intfc.ifr_ifrn.ifrn_name, intfc_name, strlen(intfc_name));  
        if (setsockopt(net->sock_fd, SOL_SOCKET, SO_BINDTODEVICE, (const void *)&intfc, sizeof(intfc)) < 0) {
            //elog_e("udpclient", "SO_BINDTODEVICE failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
            //goto __END;
        }
    }

    if (lwip_setsockopt(net->sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        elog_e("udpclient", "SO_REUSEADDR failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    srv.sin_addr.s_addr = htonl(INADDR_ANY);
    srv.sin_family      = AF_INET;
    srv.sin_port        = htons(port);
    if (lwip_bind(net->sock_fd, (struct sockaddr *)&srv, sizeof(struct sockaddr)) < 0) {
        elog_e("udpclient", "bind failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    if (lwip_fcntl(net->sock_fd, F_SETFL, O_NONBLOCK) < 0) {
        elog_e("udpclient", "O_NONBLOCK failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    net->tcp_fd[0]  = net->sock_fd;
    net->sock_fd = -1;

    if(NET_IS_NORMAL(n)) {
        switch(g_tcpip_cfgs[n].cfg.normal.proto_type) {
        case PROTO_CC_BJDC:
            //cc_bjdc_startwork(n);
            break;
        case PROTO_HJT212:
            //hjt212_startwork(n);
            break;
        case PROTO_DM101:
            dm101_startwork(n);
            break;
        }
    } else if(NET_IS_XFER(n)) {
        ;
    }

    while (1) {
        FD_ZERO(&readset);
        FD_SET(net->tcp_fd[0], &readset);
        
        rt_thddog_feed("udp client recvfrom before select 1 sec");
        timeout.tv_sec = 4; timeout.tv_usec = 0;
        result = lwip_select(net->tcp_fd[0] + 1, &readset, 0, 0, &timeout);
        rt_thddog_feed("udp client recvfrom after select");
        if (result < 0) {
            elog_e("udpclient", "recvfrom select error, err:%d, index:%d", lwip_get_error(net->tcp_fd[0]), n);
            goto __END;
        } else if (0 == result) {
            //net_printf("udp client recvfrom select timeout!\n");
            if (net->disconnect || net->shutdown) {
                elog_i("udpclient", "disconnect/shutdown, fd:%d, index:%d", net->tcp_fd[0], n);
                goto __END;
            }
            continue;
        }
        
        if (FD_ISSET(net->tcp_fd[0], &readset)) {
            while (1) {
                int readlen = 0;
                struct sockaddr_in client_addr;
                len = sizeof(struct sockaddr);
                rt_thddog_feed("udp client recvfrom with take mutex");
                if(net->mutex) rt_mutex_take(net->mutex, RT_WAITING_FOREVER);
                //net->pos = 0;
                readlen = lwip_recvfrom(net->tcp_fd[0], net->buffer, net->bufsz, 0, (struct sockaddr *)&client_addr, &len);
                if(net->mutex) rt_mutex_release(net->mutex);
                if (0 == readlen) {
                    elog_w("udpclient", "close with read 0, fd:%d, index:%d", net->tcp_fd[0], n);
                    goto __END;
                } else if (readlen < 0) {
                    if (EAGAIN == lwip_get_error(net->tcp_fd[0])) {
                        break;
                    } else {
                        elog_w("udpclient", "close with read err, err:%d, index:%d", lwip_get_error(net->tcp_fd[0]), n);
                        goto __END;
                    }
                } else {
                    elog_i("udpclient", "read:%d, index:%d", readlen, n);
                    if(client_addr.sin_addr.s_addr == net->to_addr.sin_addr.s_addr) {
                        net->pos = readlen;
                        rt_thddog_feed("udp client ws_vm_rcv_write");
                        if( g_ws_cfg.enable && (WS_PORT_NET == g_ws_cfg.port_type || WS_PORT_GPRS == g_ws_cfg.port_type) && (port == g_ws_cfg.listen_port) ) {
                            ws_vm_rcv_write(0, net->buffer, net->pos);
                        }
                        g_xDm101Status[n].dm101_lastheart = rt_tick_get();
                        g_xDm101Status[n].dm101_lastrecv = g_xDm101Status[n].dm101_lastheart;
                        if(NET_IS_NORMAL(n)) {
                            switch(g_tcpip_cfgs[n].cfg.normal.proto_type) {
                            case PROTO_MODBUS_TCP:
                            case PROTO_MODBUS_RTU_OVER_TCP:
                                /*if(PROTO_SLAVE == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                                    rt_thddog_feed("xMBPortEventPost");
                                    xMBPortEventPost( BOARD_UART_MAX + n, EV_FRAME_RECEIVED );
                                } else if(PROTO_MASTER == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                                    rt_thddog_feed("xMBMasterPortEventPost");
                                    xMBMasterPortEventPost( BOARD_UART_MAX + n, EV_MASTER_FRAME_RECEIVED );
                                }*/
                                break;
                            case PROTO_CC_BJDC: // not support now
                                //rt_thddog_feed("CC_BJDC_PutBytes");
                                //CC_BJDC_PutBytes(n, net->buffer, net->pos);
                                break;
                            case PROTO_HJT212: // not support now
                                //rt_thddog_feed("HJT212_PutBytes");
                                //HJT212_PutBytes(n, net->buffer, net->pos);
                                break;
                            case PROTO_DM101:
                                rt_thddog_feed("dm101_put_bytes");
                                dm101_put_bytes(n, net->buffer, net->pos);
                                break;
                            }
                        } else if(NET_IS_XFER(n)) {
                            __xfer_rcv_frame(n, (rt_uint8_t *)net->buffer, net->pos);
                        }
                    } else {
                        net_printf("udp client lwip_recvfrom err address\n");
                    }
                    break;
                }
            }
        }
    }

__END:
    elog_w("udpclient", "end, index:%d", n);

    rt_thddog_feed("udp client end");
    
    if(NET_IS_NORMAL(n)) {
        switch(g_tcpip_cfgs[n].cfg.normal.proto_type) {
        case PROTO_CC_BJDC:
            //cc_bjdc_exitwork(n);
            break;
        case PROTO_HJT212:
            //hjt212_exitwork(n);
        case PROTO_DM101:
            dm101_exitwork(n);
            break;
        }
    } else if(NET_IS_XFER(n)) {
        ;
    }

    if(net->mutex) rt_mutex_take(net->mutex, RT_WAITING_FOREVER);
    if (net->tcp_fd[0] >= 0) {
        lwip_shutdown(net->tcp_fd[0], SHUT_RDWR);
        lwip_close(net->tcp_fd[0]);
        net->tcp_fd[0] = -1;
    }
    if (net->sock_fd >= 0) {
        lwip_shutdown(net->sock_fd, SHUT_RDWR);
        lwip_close(net->sock_fd);
        net->sock_fd = -1;
    }
    if(net->mutex) rt_mutex_release(net->mutex);
}

static void _net_task(void *parameter)
{
    int n = (int)(long)parameter;
    net_helper_t    *net = &s_net_list[n];
    int             xfer_n = NET_GET_XFER(n);
    tcpip_type_e    tcpip_type;
    tcpip_cs_e      tcpip_cs;

    tcpip_type  = g_tcpip_cfgs[n].tcpip_type;
    tcpip_cs    = g_tcpip_cfgs[n].tcpip_cs;

    while (1) {
        net->disconnect = RT_FALSE;
        net->shutdown = RT_FALSE;
        rt_thddog_feed("");
        switch (tcpip_type) {
        case TCP_IP_TCP:
        {
            if (TCPIP_CLIENT == tcpip_cs) {
                rt_thddog_feed("prvRunAsTCPClient");
                prvRunAsTCPClient(n);
            } else if (TCPIP_SERVER == tcpip_cs) {
                rt_thddog_feed("prvRunAsTCPServer");
                prvRunAsTCPServer(n);
            }
            break;
        }
        case TCP_IP_UDP:
        {
            if (TCPIP_CLIENT == tcpip_cs) {
                rt_thddog_feed("prvRunAsUDPClient");
                prvRunAsUDPClient(n);
            } else if (TCPIP_SERVER == tcpip_cs) {
                rt_thddog_feed("prvRunAsUDPServer");
                prvRunAsUDPServer(n);
            }
            break;
        }
        }
        if (net->shutdown) {
            break;
        } else {
            rt_thread_delay(2 * RT_TICK_PER_SECOND);
        }
    }

    net->thread = RT_NULL;
}

