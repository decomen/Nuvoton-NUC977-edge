
#include "board.h"
#include "modbus.h"
#include "modbus_helper.h"

static serial_t s_serial_list[BOARD_UART_MAX];
serial_t *g_serials[BOARD_UART_MAX] = {
    &s_serial_list[0], &s_serial_list[1], 
    &s_serial_list[2], &s_serial_list[3], 
    &s_serial_list[4]
};

void serial_helper_init(void)
{
    int n;
    for (n = 0; n < BOARD_UART_MAX; n++) {
        g_serials[n]->fd = -1;
        g_serials[n]->cfg = g_uart_cfgs[n].port_cfg;
    }
}

void serial_helper_open(int port)
{
    serial_helper_close(port);
    g_serials[port]->fd = das_do_open_hw_com(port);
    if (g_serials[port]->fd >= 0) {
        das_do_set_hw_com_info(g_serials[port]->fd, 
            g_serials[port]->cfg.baud_rate, 
            g_serials[port]->cfg.data_bits, 
            g_serials[port]->cfg.stop_bits, 
            g_serials[port]->cfg.parity
        );
    }
}

void serial_helper_cfg(int port, uart_port_cfg_t *cfg)
{
    g_serials[port]->cfg = *cfg;
    if (g_serials[port]->fd >= 0) {
        das_do_set_hw_com_info(g_serials[port]->fd, 
            g_serials[port]->cfg.baud_rate, 
            g_serials[port]->cfg.data_bits, 
            g_serials[port]->cfg.stop_bits, 
            g_serials[port]->cfg.parity
        );
    }
}

void serial_helper_close(int port)
{
    if (g_serials[port]->fd >= 0) {
        das_do_close_hw_com(g_serials[port]->fd);
        g_serials[port]->fd = -1;
    }
}

void serial_helper_tx_enable(int port)
{
    ;
}

void serial_helper_rx_enable(int port)
{
    ;
}

static void __try_get_modbus_fd(int port)
{
    if ((UART_TYPE_232 == g_uart_cfgs[port].uart_type ||
         UART_TYPE_485 == g_uart_cfgs[port].uart_type /*||
         UART_TYPE_LORA == g_uart_cfgs[port].uart_type*/) &&
        (PROTO_MODBUS_RTU == g_uart_cfgs[port].proto_type ||
         PROTO_MODBUS_ASCII == g_uart_cfgs[port].proto_type ||
         PROTO_OBMODBUS_RTU == g_uart_cfgs[port].proto_type)
       ) {
        if (!g_xfer_net_dst_uart_occ[port]) {
            if (g_mb_ctx_uart[port]) {
                g_serials[port]->fd = modbus_get_socket(g_mb_ctx_uart[port]);
            }
        }
    }
}

int serial_helper_send(int port, const void *data, rt_size_t size)
{
    int rc = 0;
    __try_get_modbus_fd(port);
    if (g_serials[port]->fd >= 0) {
        serial_helper_tx_enable(port);
        rc = write(g_serials[port]->fd, data, size);
        if (rc > 0 && PROTO_DEV_IS_RS(port)) {
            monitor_uart_send_data(port, (uint8_t *)data, rc);
        }
        if (size > 0 && g_ws_cfg.enable && 
            (
                (port == BOARD_ZGB_UART && g_ws_cfg.port_type == WS_PORT_ZIGBEE) ||
                (port == BOARD_LORA_UART && g_ws_cfg.port_type == WS_PORT_LORA) ||
                (g_ws_cfg.port_type <= WS_PORT_RS_MAX && g_ws_cfg.port_type == DEV_TO_WS_PORT(port))
            )) {
            ws_vm_snd_write(0, (void *)data, size);
        }
        serial_helper_rx_enable(port);
    }
    return rc;
}

int serial_helper_select(int port, int usec)
{
    if (g_serials[port]->fd >= 0) {
        int s_rc;
        fd_set rset;
        struct timeval tv = { .tv_sec = usec / 1000000, .tv_usec = usec % 1000000 };
        FD_ZERO(&rset); FD_SET(g_serials[port]->fd, &rset);
        
        while ((s_rc = select(g_serials[port]->fd + 1, &rset, NULL, NULL, usec < 0 ? NULL : &tv)) == -1) {
            if (errno == EINTR) {
                FD_ZERO(&rset);
                FD_SET(g_serials[port]->fd, &rset);
            } else {
                return -1;
            }
        }
        return s_rc;
    }
}

int serial_helper_recv(int port, void *data, rt_size_t size)
{
    if (g_serials[port]->fd >= 0) {
        int rc = read(g_serials[port]->fd, data, size);
        if (rc > 0 && PROTO_DEV_IS_RS(port)) {
            monitor_uart_recv_data(port, (uint8_t *)data, rc);
        }
        if (rc > 0 && g_ws_cfg.enable && 
            (
                (port == BOARD_ZGB_UART && g_ws_cfg.port_type == WS_PORT_ZIGBEE) ||
                (port == BOARD_LORA_UART && g_ws_cfg.port_type == WS_PORT_LORA) ||
                (g_ws_cfg.port_type <= WS_PORT_RS_MAX && g_ws_cfg.port_type == DEV_TO_WS_PORT(port))
            )) {
            ws_vm_rcv_write(0, (void *)data, rc);
        }
        return rc;
    }

    return -1;
}

int serial_helper_is_open(int port)
{
    return g_serials[port]->fd >= 0;
}

