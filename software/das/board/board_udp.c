
#include <board.h>

#define BOARD_UDP_PORT          19       // 65555 越界了, == 19
#define BOARD_UDP_BUFSZ         512

typedef struct {
    char *name;
    void (*handler)(int fd, struct sockaddr_in *addr, cJSON *pJSON );
} udp_handle_t;

static void _udp_parse_request( int fd, struct sockaddr_in *addr, const char *szJSON );

static void _getbaseinfo(int fd, struct sockaddr_in *addr, cJSON *pJSON );

const udp_handle_t s_xHandleList[] = {
    "getbaseinfo", _getbaseinfo
};

static const udp_handle_t *_udp_find( const char* name )
{
    for( int i = 0; i < sizeof(s_xHandleList)/sizeof(udp_handle_t); i++ ) {
        if( name && strncasecmp( s_xHandleList[i].name, name, strlen(s_xHandleList[i].name)) == 0 ) {
			return &s_xHandleList[i];
        }
    }

    return RT_NULL;
}

/*static void _error_request(int fd, struct sockaddr_in *addr, cJSON *pJSON )
{
    
}*/

static void _getbaseinfo(int fd, struct sockaddr_in *addr, cJSON *pJSON )
{
    extern struct netif *netif_list;
    rt_err_t err = RT_EOK;
    char buf[64];
    cJSON *pItem = cJSON_CreateObject();

    if(pItem) {
        char* szRetJSON = RT_NULL;
        
        cJSON_AddNumberToObject( pItem, "ret", RT_EOK );
        
        // dev sn
        memset( buf, 0, sizeof(buf) );
        memcpy( buf, &g_xDevInfoReg.xDevInfo.xSN, sizeof(DevSN_t) );
        cJSON_AddStringToObject( pItem, "sn", buf );

        // dev id
        memset( buf, 0, sizeof(buf) );
        strcpy( buf, g_host_cfg.szId );
        cJSON_AddStringToObject( pItem, "id", buf );
        
        // dev oem
        memset( buf, 0, sizeof(buf) );
        memcpy( buf, &g_xDevInfoReg.xDevInfo.xOEM, sizeof(DevOEM_t) );
        cJSON_AddStringToObject( pItem, "oem", buf );
        
        cJSON_AddNumberToObject( pItem, "hw", g_xDevInfoReg.xDevInfo.usHWVer );
        cJSON_AddNumberToObject( pItem, "sw", g_xDevInfoReg.xDevInfo.usSWVer );

        // host name
        cJSON_AddStringToObject( pItem, "host", g_host_cfg.szHostName );

        // net info

        if( netif_list ) {
            cJSON_AddNumberToObject( pItem, "dhcp", netif_list->flags & NETIF_FLAG_DHCP ? 1 : 0 );
            extern ip_addr_t dns_getserver(u8_t numdns);
            ip_addr_t ip_addr = dns_getserver(0);
            snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
                netif_list->hwaddr[0], 
                netif_list->hwaddr[1], 
                netif_list->hwaddr[2], 
                netif_list->hwaddr[3], 
                netif_list->hwaddr[4], 
                netif_list->hwaddr[5]
            );
            cJSON_AddStringToObject( pItem, "mac", buf );
            cJSON_AddStringToObject( pItem, "ip", ipaddr_ntoa(&(netif_list->ip_addr)) );
            cJSON_AddStringToObject( pItem, "mask", ipaddr_ntoa(&(netif_list->netmask)) );
            cJSON_AddStringToObject( pItem, "gw", ipaddr_ntoa(&(netif_list->gw)) );
            cJSON_AddStringToObject( pItem, "d1", ipaddr_ntoa(&(ip_addr)) );
        }
        
        szRetJSON = cJSON_PrintUnformatted( pItem );

        if(szRetJSON) {
            lwip_sendto( fd, szRetJSON, strlen(szRetJSON), MSG_NOSIGNAL, (struct sockaddr *)addr, sizeof( struct sockaddr ) );
            rt_free( szRetJSON );
        }
    }
    cJSON_Delete( pItem );
}

static void _udp_parse_request( int fd, struct sockaddr_in *addr, const char *szJSON )
{
    cJSON *pCfg = szJSON ? cJSON_Parse( szJSON ) : RT_NULL;
    if( pCfg ) {
        const char *func = cJSON_GetString( pCfg, "func", VAR_NULL );
        if( func ) {
            const udp_handle_t *handle = _udp_find( func );
            if( handle && handle->handler ) {
                handle->handler( fd, addr, pCfg );
            }
        }

        cJSON_Delete( pCfg );
    }
}

static void _rt_board_udp_thread(void* parameter)
{
    struct sockaddr_in client_addr;
    char *buffer = rt_malloc( BOARD_UDP_BUFSZ );
    if( !buffer ) {
        rt_kprintf( "board_udp buffer malloc fail.\n" );
        return ;
    }

    for( ;; ) {
        int on = 1;
        int broadcast = 1;
        int nSock = lwip_socket( AF_INET, SOCK_DGRAM, 0 );
        struct sockaddr_in srv;
        
        if( nSock < 0 ) {
            goto __END;
        }

        memset( &srv, 0, sizeof(srv) );
        srv.sin_addr.s_addr = htonl(INADDR_ANY);
        srv.sin_family      = AF_INET;
        srv.sin_port        = htons(BOARD_UDP_PORT);

        if( lwip_setsockopt( nSock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast) ) < 0 ) {
            rt_kprintf( "board_udp set setsockopt SO_BROADCAST fail.\n" );
            goto __END;
        }
        if( lwip_setsockopt( nSock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ) < 0 ) {
            rt_kprintf( "board_udp set setsockopt SO_REUSEADDR fail.\n" );
            goto __END;
        }
        
        if( lwip_bind( nSock, (struct sockaddr*)&srv, sizeof(struct sockaddr) ) < 0 ) {
            lwip_close(nSock);
            rt_kprintf( "board_udp server bind failed!\n" );
            goto __END;
        }

        rt_kprintf( "board_udp server start recvfrom\n" );

        while( 1 ) {
            rt_uint32_t addr_len = sizeof(struct sockaddr);
        	int nReadLen = 0;
        	rt_thddog_suspend("lwip_recvfrom");
            nReadLen = lwip_recvfrom( nSock, buffer, BOARD_UDP_BUFSZ, 0, (struct sockaddr *)&client_addr, &addr_len );
            rt_thddog_resume();
            if( nReadLen <= 0 ) {
                int err = lwip_get_error( nSock );
    			rt_kprintf( "board_udp server socket close : %d!\n", err );    
                break;
            } else {
        	    rt_thddog_suspend("_udp_parse_request");
                _udp_parse_request( nSock, &client_addr, (const char *)buffer );
                rt_thddog_resume();
            }
        }

    __END:
        rt_kprintf( "board_udp end !\n" );

        if( nSock >= 0 ) {
            lwip_shutdown(nSock, SHUT_RDWR);
            lwip_close( nSock );
        }
    
        rt_thread_delay( RT_TICK_PER_SECOND );
        rt_thddog_feed("");
    }
    rt_thddog_exit();
}

void board_udp_init( void )
{
    rt_thread_t board_udp_thread = rt_thread_create( "boardudp", _rt_board_udp_thread, RT_NULL, 0x400, RT_THREAD_PRIORITY_MAX-1, 100 );

    if ( board_udp_thread != RT_NULL) {
        rt_thddog_register(board_udp_thread, 30);
        rt_thread_startup( board_udp_thread );
    }    
}

