
#include "board.h"

typedef struct {
    rt_uint16_t timeout;
    rt_thread_t thread;
    uart_port_cfg_t *ucfg;
    uart_type_e     dst_serial_type;
} xfer_dst_serial_cfg_t;
static xfer_dst_serial_cfg_t s_xfer_dst_serial_cfgs[BOARD_UART_MAX] = {{0,0}};

static void __xfer_dst_serial_reopen(int port)
{
    xfer_dst_serial_cfg_t *cfg = &s_xfer_dst_serial_cfgs[port];
    serial_helper_cfg(port, cfg->ucfg);
    serial_helper_open(port);
}

void xfer_dst_serial_recv_buffer(int port, void *buffer, int size)
{
    if(g_xfer_net_dst_uart_trt[port] || g_xfer_net_dst_uart_dtu[port]) {
        vZigbee_SendBuffer((uint8_t *)buffer, size);
    } else {
        if(g_xfer_net_dst_uart_map[port] >= 0) {
            xfer_net_send(g_xfer_net_dst_uart_map[port], (uint8_t *)buffer, size);
        }
    }
}

static void __xfer_dst_serial_worker(void *parameter)
{
    while (1) {
        int port = (int)(long)parameter;
        xfer_dst_serial_cfg_t *cfg = &s_xfer_dst_serial_cfgs[port];
        uint8_t buffer[1024] = {0};
        while (serial_helper_is_open(port)) {
            int pos = 0;
            int s_rc = serial_helper_select(port, -1);
            int n = serial_helper_recv(port, buffer, sizeof(buffer));
            if (n > 0) {
                pos = n;
                while (1) {
                    s_rc = serial_helper_select(port, cfg->timeout * 1000);
                    if (s_rc == -1) {
                        usleep(100 * 1000);
                        break;
                    } else if (s_rc == 0) {
                        xfer_dst_serial_recv_buffer(port, buffer, pos);
                        break;
                    } else {
                        n = serial_helper_recv(port, &buffer[pos], sizeof(buffer) - pos);
                        pos += n;
                        if (pos >= sizeof(buffer)) {
                            xfer_dst_serial_recv_buffer(port, buffer, pos);
                            break;
                        }
                    }
                }
            } else if (n <= 0) {
                if (n < 0 && errno == EBADF) __xfer_dst_serial_reopen(port);
                rt_thread_delay(500);
            }
        }
        rt_thread_delay(1000);
    }
}


rt_bool_t xfer_dst_serial_open(int port)
{
    xfer_dst_uart_cfg *pucfg = RT_NULL;
    rt_int8_t dst_type = -1;
    xfer_dst_serial_cfg_t *cfg = &s_xfer_dst_serial_cfgs[port];
    xfer_cfg_t *xfer = g_xfer_net_dst_uart_map[port]>=0?&g_tcpip_cfgs[g_xfer_net_dst_uart_map[port]].cfg.xfer:RT_NULL;

    if(xfer && XFER_M_GW == xfer->mode) {
        dst_type = xfer->cfg.gw.dst_type;
        cfg->ucfg = &xfer->cfg.gw.dst_cfg.uart_cfg.cfg;
        cfg->dst_serial_type = xfer->cfg.gw.dst_cfg.uart_cfg.type;
    } else if(xfer && XFER_M_TRT == xfer->mode) {
        dst_type = xfer->cfg.trt.dst_type;
        cfg->ucfg = &xfer->cfg.trt.dst_cfg.uart_cfg.cfg;
        cfg->dst_serial_type = xfer->cfg.trt.dst_cfg.uart_cfg.type;
    } else {
        if(port != BOARD_ZGB_UART && port != BOARD_LORA_UART) {
            cfg->ucfg = &g_uart_cfgs[port].port_cfg;
        }
    }

    if (dst_type >= PROTO_DEV_RS1 && dst_type <= PROTO_DEV_RS_MAX) {
        __xfer_dst_serial_reopen(port);
    }
    serial_helper_rx_enable(port);
    
    cfg->timeout = 50;
    
    BOARD_CREAT_NAME(sz_thread, "xfer_com_%d", port);
    cfg->thread = \
        rt_thread_create(sz_thread, __xfer_dst_serial_worker, (void *)(long)port, 0, 0, 0);
    if (cfg->thread) {
        rt_thread_startup(cfg->thread);
    }
    
    return RT_TRUE;
}

void xfer_dst_serial_send(int port, const void *data, rt_size_t size)
{
    serial_helper_send(port, data, size);
}

