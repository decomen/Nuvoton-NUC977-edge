
#include <board.h>
#include <stdio.h>
#include "net_helper.h"

void xfer_helper_serial_init( void )
{
    for (int n = 0; n < BOARD_UART_MAX; n++) {
        if (g_xfer_net_dst_uart_occ[n]) {
            xfer_dst_serial_open(n);
        }
    }
}

int xfer_tcpip_mode_check(int n)
{
    if (n < BOARD_TCPIP_MAX && g_tcpip_cfgs[n].enable) {
        if (g_tcpip_cfgs[n].mode == TCP_IP_M_XFER) {
            if (g_tcpip_cfgs[n].cfg.xfer.mode == XFER_M_GW) {
                if (g_tcpip_cfgs[n].cfg.xfer.cfg.gw.dst_type == PROTO_DEV_NET
                    || g_tcpip_cfgs[n].cfg.xfer.cfg.gw.dst_type == PROTO_DEV_GPRS) {
                    return g_tcpip_cfgs[n].cfg.xfer.cfg.gw.dst_cfg.tcpip_cfg.idx;
                }
            } else if (g_tcpip_cfgs[n].cfg.xfer.mode == XFER_M_TRT) {
                if (g_tcpip_cfgs[n].cfg.xfer.cfg.trt.dst_type == PROTO_DEV_NET
                    || g_tcpip_cfgs[n].cfg.xfer.cfg.trt.dst_type == PROTO_DEV_GPRS) {
                    return g_tcpip_cfgs[n].cfg.xfer.cfg.trt.dst_cfg.tcpip_cfg.idx;
                }
            }
        }
    }
    return -1;
}

int xfer_enet_to_gprs_check(int n)
{
    int gprs_idx = xfer_tcpip_mode_check(n);
    if (gprs_idx >= 0) {
        return (n == xfer_tcpip_mode_check(BOARD_ENET_TCPIP_NUM + gprs_idx));
    }
    return 0;
}

int xfer_gprs_to_enet_check(int n)
{
    int enet_idx = xfer_tcpip_mode_check(n + BOARD_ENET_TCPIP_NUM);
    if (enet_idx >= 0) {
        return (n == xfer_tcpip_mode_check(enet_idx));
    }
    return 0;
}

void xfer_helper_enet_init( void )
{
    rt_bool_t open_flag[BOARD_ENET_TCPIP_NUM] = {0};
    for (int n = 0; n < BOARD_UART_MAX; n++) {
        if (g_xfer_net_dst_uart_occ[n]) {
            if(NET_IS_ENET(g_xfer_net_dst_uart_map[n]) && !open_flag[g_xfer_net_dst_uart_map[n]]) {
                xfer_net_open(g_xfer_net_dst_uart_map[n]);
                open_flag[g_xfer_net_dst_uart_map[n]] = RT_TRUE;
            }
        }
    }

    for (int n = 0; n < BOARD_ENET_TCPIP_NUM; n++) {
        if (xfer_enet_to_gprs_check(n) && !open_flag[n]) {
            xfer_net_open(n);
            open_flag[n] = RT_TRUE;
        }
    }
}

#if NET_HAS_GPRS
void xfer_helper_gprs_init( void )
{
    rt_bool_t open_flag[BOARD_GPRS_TCPIP_NUM] = {0};
    for (int n = 0; n < BOARD_UART_MAX; n++) {
        if (g_xfer_net_dst_uart_occ[n]) {
            if(NET_IS_GPRS(g_xfer_net_dst_uart_map[n]) && !open_flag[g_xfer_net_dst_uart_map[n]-BOARD_ENET_TCPIP_NUM]) {
                xfer_net_open(g_xfer_net_dst_uart_map[n]);
                open_flag[g_xfer_net_dst_uart_map[n]-BOARD_ENET_TCPIP_NUM] = RT_TRUE;
            }
        }
    }
    
    for (int n = 0; n < BOARD_GPRS_TCPIP_NUM; n++) {
        if (xfer_gprs_to_enet_check(n) && !open_flag[n]) {
            xfer_net_open(n + BOARD_ENET_TCPIP_NUM);
            open_flag[n] = RT_TRUE;
        }
    }
}
#endif

