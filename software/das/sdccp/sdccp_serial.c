/**
 * example of adding sdccp serial external library
 */

#include "board.h"
#include "sdccp_dust.h"
#include "dlt645.h"
#include "dlt645_1997.h"

#define sdccp_serial_printf(_fmt,...)   rt_kprintf( "[sdccp serial]" _fmt, ##__VA_ARGS__ )

typedef struct {
    rt_uint16_t timeout;
    rt_thread_t thread;
} sdccp_serial_cfg_t;
static sdccp_serial_cfg_t s_sdccp_serial_cfgs[BOARD_UART_MAX] = {{0,0}};

// 目前有: PROTO_DLT645 PROTO_DLT645_1997 PROTO_DUST PROTO_MBUS603
rt_bool_t sdccp_serial_check(int index)
{
    int port = nUartGetInstance(index);
    if (port >= 0) {
        if (
            (
                PROTO_DLT645 == g_uart_cfgs[port].proto_type || 
                PROTO_DLT645_1997 == g_uart_cfgs[port].proto_type || 
                PROTO_DUST == g_uart_cfgs[port].proto_type || 
                PROTO_MBUS603 == g_uart_cfgs[port].proto_type
            )
           ) {

            return (!g_xfer_net_dst_uart_occ[port]);
        }
    }

    return RT_FALSE;
}

void sdccp_serial_senddata(int index, const void *data, rt_size_t size)
{
    serial_helper_send(nUartGetInstance(index), data, size);
}

static void __dlt645_send(int index,mdBYTE *pdata,mdBYTE len);
static void __dust_send(int index,mdBYTE *pdata,mdBYTE len);
static void __mbus603_send(int index,mdBYTE *pdata,mdBYTE len);

static void __sdccp_serial_recv_buffer(int port, void *buffer, int size)
{
    if (PROTO_DLT645 == g_uart_cfgs[port].proto_type) {
        Dlt645_PutBytes(nUartGetIndex(port), (mdBYTE *)buffer, size);
    } else if (PROTO_DLT645_1997 == g_uart_cfgs[port].proto_type) {
        Dlt645_1997_PutBytes(nUartGetIndex(port), (mdBYTE *)buffer, size);
    } else if (PROTO_DUST == g_uart_cfgs[port].proto_type) {
        Dust_PutBytes(nUartGetIndex(port), (mdBYTE *)buffer, size);
    } else if (PROTO_MBUS603 == g_uart_cfgs[port].proto_type) {
        Mbus603_PutBytes(nUartGetIndex(port), (mdBYTE *)buffer, size);
    }
}

static uint32_t __sdccp_serial_get_timeout(int port)
{
    if (g_uart_cfgs[port].port_cfg.baud_rate > 19200) {
        return 35 * 500;
    } else {
        return 500 * (7UL * 220000UL) / (2UL * g_uart_cfgs[port].port_cfg.baud_rate);
    }
}

static void __sdccp_serial_reopen(int port)
{
    serial_helper_cfg(port, &g_uart_cfgs[port].port_cfg);
    serial_helper_open(port);
}

static void __sdccp_serial_worker(void *parameter)
{
    while (1) {
        int port = (int)(long)parameter;
        sdccp_serial_cfg_t *cfg = &s_sdccp_serial_cfgs[port];
        uint8_t buffer[1024] = {0};
        while (serial_helper_is_open(port)) {
            int pos = 0;
            int s_rc = serial_helper_select(port, -1);
            int n = serial_helper_recv(port, buffer, sizeof(buffer));
            if (n > 0) {
                pos = n;
                while (1) {
                    s_rc = serial_helper_select(port, __sdccp_serial_get_timeout(port));
                    if (s_rc == -1) {
                        usleep(100 * 1000);
                        break;
                    } else if (s_rc == 0) {
                        __sdccp_serial_recv_buffer(port, buffer, pos);
                        break;
                    } else {
                        n = serial_helper_recv(port, &buffer[pos], sizeof(buffer) - pos);
                        pos += n;
                        if (pos >= sizeof(buffer)) {
                            __sdccp_serial_recv_buffer(port, buffer, pos);
                            break;
                        }
                    }
                }
            } else if (n <= 0) {
                if (n < 0 && errno == EBADF) __sdccp_serial_reopen(port);
                rt_thread_delay(500);
            }
        }
        rt_thread_delay(1000);
    }
}

void sdccp_serial_openall(void)
{
    for (int index = 0; index < BOARD_UART_MAX; index++) {
        if (sdccp_serial_check(index)) {
            int port = nUartGetInstance(index);
            __sdccp_serial_reopen(port);

            if (s_sdccp_serial_cfgs[port].timeout<=1) {
                s_sdccp_serial_cfgs[port].timeout = 20;
            }
            
            serial_helper_rx_enable(port);

            if (PROTO_DLT645 == g_uart_cfgs[port].proto_type) {
                dlt645_init(index, __dlt645_send);
            } else if (PROTO_DLT645_1997 == g_uart_cfgs[port].proto_type) {
                dlt645_1997_init(index, __dlt645_send);
            } else if (PROTO_DUST == g_uart_cfgs[port].proto_type) {
                dust_init(index, __dust_send);
            } else if (PROTO_MBUS603 == g_uart_cfgs[port].proto_type) {
                Mbus603_init(index, __mbus603_send);
            }

            BOARD_CREAT_NAME(sz_thread, "sdccp_s%d", port);
            s_sdccp_serial_cfgs[port].thread = \
                rt_thread_create(sz_thread, __sdccp_serial_worker, (void *)(long)port, 0, 0, 0);
            if (s_sdccp_serial_cfgs[port].thread) {
                rt_thread_startup(s_sdccp_serial_cfgs[port].thread);
            }
        }
    }
}

static void __dlt645_send(int index,mdBYTE *pdata,mdBYTE len)
{
    sdccp_serial_senddata(index, (const void *)pdata, len);
}

static void __dust_send(int index,mdBYTE *pdata,mdBYTE len)
{
    sdccp_serial_senddata(index, (const void *)pdata, len);
}

static void __mbus603_send(int index,mdBYTE *pdata,mdBYTE len)
{
    sdccp_serial_senddata(index, (const void *)pdata, len);
}

void sdccp_serial_closeall(void)
{
    for (int index = 0; index < BOARD_UART_MAX; index++) {
        if (sdccp_serial_check(index)) {
            int port = nUartGetInstance(index);
            sdccp_serial_cfg_t *cfg = &s_sdccp_serial_cfgs[port];
            if (cfg->thread) {
                rt_thread_delete(cfg->thread);
                cfg->thread = NULL;
            }
            serial_helper_close(port);
        }
    }
}

