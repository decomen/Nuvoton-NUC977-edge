#include "board.h"
#include "obmodbus.h"
#include "modbus.h"
#include "obmodbus_helper.h"
#include <arpa/inet.h>

#define _MODBUS_REGS_START          0
#define _MODBUS_REGS_COUNT          USER_REG_EXT_DATA_END

obmodbus_t *g_obmb_ctx_uart[BOARD_UART_MAX];
rt_mutex_t g_obmb_mutex_uart[BOARD_UART_MAX];

static void __obmodbus_read(obmodbus_t *ctx, const uint8_t *data, int length)
{
    modbus_user_data_t *user_data = (modbus_user_data_t *)obmodbus_get_user_data(ctx);
    //printf("__obmodbus_read[%d] = %d\n", user_data ? user_data->port_type : -1, length);
    if (user_data->port_type >= WS_PORT_RS_MIN && 
        user_data->port_type <= WS_PORT_RS_MAX && 
        PROTO_DEV_IS_RS(WS_PORT_TO_DEV(user_data->port_type))) {
        monitor_uart_recv_data(WS_PORT_TO_DEV(user_data->port_type), (uint8_t *)data, length);
    }
    if (g_ws_cfg.enable) {
        if (user_data) {
            if (g_ws_cfg.port_type >= WS_PORT_RS_MIN && g_ws_cfg.port_type <= WS_PORT_RS_MAX && user_data->port_type == g_ws_cfg.port_type) {
                ws_vm_rcv_write(0, (void *)data, length);
            } else if (user_data->port_type == g_ws_cfg.port_type && user_data->index == g_ws_cfg.listen_index) {
                tcpip_state_t *ts = net_tcpip_state_get(user_data->index, obmodbus_get_socket(ctx));
                if (ts && ts->usRemPort == g_ws_cfg.listen_port) {
                    ws_vm_rcv_write(0, (void *)data, length);
                }
            }
        }
    }
}

static void __obmodbus_write(obmodbus_t *ctx, const uint8_t *data, int length)
{
    modbus_user_data_t *user_data = (modbus_user_data_t *)obmodbus_get_user_data(ctx);
    //printf("__obmodbus_write[%d] = %d\n", user_data ? user_data->port_type : -1, length);
    if (user_data->port_type >= WS_PORT_RS_MIN && 
        user_data->port_type <= WS_PORT_RS_MAX && 
        PROTO_DEV_IS_RS(WS_PORT_TO_DEV(user_data->port_type))) {
        monitor_uart_send_data(WS_PORT_TO_DEV(user_data->port_type), (uint8_t *)data, length);
    }
    if (g_ws_cfg.enable) {
        if (user_data) {
            if (g_ws_cfg.port_type >= WS_PORT_RS_MIN && g_ws_cfg.port_type <= WS_PORT_RS_MAX && user_data->port_type == g_ws_cfg.port_type) {
                ws_vm_snd_write(0, (void *)data, length);
            } else if (user_data->port_type == g_ws_cfg.port_type && user_data->index == g_ws_cfg.listen_index) {
                tcpip_state_t *ts = net_tcpip_state_get(user_data->index, obmodbus_get_socket(ctx));
                if (ts && ts->usRemPort == g_ws_cfg.listen_port) {
                    ws_vm_snd_write(0, (void *)data, length);
                }
            }
        }
    }
}

static void __obmodbus_state_change(obmodbus_t *ctx, int s, obmodbus_state_t state)
{
    modbus_user_data_t *data = (modbus_user_data_t *)obmodbus_get_user_data(ctx);
    if (data && (data->port_type == WS_PORT_NET || data->port_type == WS_PORT_GPRS)) {
        int n = data->index;
        if (n < BOARD_TCPIP_MAX) {
            tcpip_state_t *ts = net_tcpip_state_open(n, s);
            if (ts) {
                switch (state) {
                case MB_STATE_INIT:
                    ts->eState = TCPIP_STATE_WAIT;
                    break;
                case MB_STATE_ACCEPT:
                    ts->eState = TCPIP_STATE_ACCEPT;
                    break;
                case MB_STATE_CONNING:
                    ts->eState = TCPIP_STATE_CONNING;
                    break;
                case MB_STATE_CONNED:
                    if (ts->eState != TCPIP_STATE_CONNED) {
                        ts->ulConnTime = (rt_uint32_t)das_sys_time();
                        ts->eState = TCPIP_STATE_CONNED;
                        {
                            struct sockaddr_in sa;
                            int len = sizeof(sa);
                            if (!lwip_getsockname(s, (struct sockaddr *)&sa, &len)) {
                                strcpy(ts->szLocIP, inet_ntoa(sa.sin_addr));
                                ts->usLocPort = ntohs(sa.sin_port);
                            }
                            if (!lwip_getpeername(s, (struct sockaddr *)&sa, &len)) {
                                strcpy(ts->szRemIP, inet_ntoa(sa.sin_addr));
                                ts->usRemPort = ntohs(sa.sin_port);
                            }
                        }
                    }
                    break;
                case MB_STATE_DISCONNED:
                    net_tcpip_state_close(n, s);
                    break;
                }
            }
        }
    }
}

obmodbus_monitor_t g_obmodbus_monitor = {
    .read           = __obmodbus_read, 
    .write          = __obmodbus_write, 
    .state_change   = __obmodbus_state_change, 
};

