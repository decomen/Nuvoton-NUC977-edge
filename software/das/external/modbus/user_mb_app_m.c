#include "board.h"
#include "net_helper.h"
#include "modbus.h"
#include "modbus_helper.h"

extern modbus_customize_backend_t g_zgb_customize_backend;
extern modbus_customize_backend_t g_lora_customize_backend;

rt_err_t xMBRTU_ASCIIMasterPollReStart(rt_uint8_t ucPort, eMBMode eMode)
{
    vMBRTU_ASCIIMasterPollStop(ucPort);

    modbus_t *mb_ctx = NULL;
    if (eMode == MB_RTU) {
        mb_ctx = modbus_new_rtu(
            das_do_get_uart_driver_name(ucPort), 
            g_uart_cfgs[ucPort].port_cfg.baud_rate, 
            das_do_get_uart_parity_char(g_uart_cfgs[ucPort].port_cfg.parity), 
            g_uart_cfgs[ucPort].port_cfg.data_bits, 
            g_uart_cfgs[ucPort].port_cfg.stop_bits
        );
    } else if (eMode == MB_ASCII) {
        mb_ctx = modbus_new_ascii(
            das_do_get_uart_driver_name(ucPort), 
            g_uart_cfgs[ucPort].port_cfg.baud_rate, 
            das_do_get_uart_parity_char(g_uart_cfgs[ucPort].port_cfg.parity), 
            g_uart_cfgs[ucPort].port_cfg.data_bits, 
            g_uart_cfgs[ucPort].port_cfg.stop_bits
        );
    }

    if (mb_ctx) {
        if (ucPort == BOARD_ZGB_UART) {
            modbus_set_customize_backend(mb_ctx, &g_zgb_customize_backend);
        } else if (ucPort == BOARD_LORA_UART) {
            modbus_set_customize_backend(mb_ctx, &g_lora_customize_backend);
        }
        g_modbus_rtu_user_data[ucPort].port_type = DEV_TO_WS_PORT(ucPort);
        g_modbus_rtu_user_data[ucPort].net_port = -1;
        g_modbus_rtu_user_data[ucPort].index  = -1;
        modbus_set_user_data(mb_ctx, &g_modbus_rtu_user_data[ucPort]);
        modbus_set_monitor(mb_ctx, &g_modbus_monitor);
        modbus_set_debug(mb_ctx, FALSE);
        modbus_set_error_recovery(mb_ctx, MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL);
        if (ucPort == BOARD_ZGB_UART || ucPort == BOARD_LORA_UART) {
            if (eMode == MB_RTU) {
                if (ucPort == BOARD_LORA_UART) {
                    modbus_set_response_timeout(mb_ctx, 3, 0);
                } else {
                    modbus_set_response_timeout(mb_ctx, 1, 500000);
                }
            } else {
                modbus_set_response_timeout(mb_ctx, 3, 0);
            }
        } else {
            if (eMode == MB_RTU) {
                if (g_uart_cfgs[ucPort].port_cfg.baud_rate < 9600) {
                    modbus_set_response_timeout(mb_ctx, 1, 500000);
                } else {
                    modbus_set_response_timeout(mb_ctx, 0, 500000);
                }
            } else {
                modbus_set_response_timeout(mb_ctx, 1, 0);
            }
        }
        modbus_connect(mb_ctx);
        //modbus_set_slave(mb_ctx, g_uart_cfgs[ucPort].slave_addr);
    }
    g_mb_mutex_uart[ucPort] = rt_mutex_create();
    g_mb_ctx_uart[ucPort] = mb_ctx;
    return RT_EOK;
}

void vMBRTU_ASCIIMasterPollStop(rt_uint8_t ucPort)
{
    if (g_mb_ctx_uart[ucPort]) {
        modbus_close(g_mb_ctx_uart[ucPort]);
        modbus_free(g_mb_ctx_uart[ucPort]);
        g_mb_ctx_uart[ucPort] = NULL;

        rt_mutex_delete(g_mb_mutex_uart[ucPort]);
        g_mb_mutex_uart[ucPort] = NULL;
    }
}

rt_err_t xMBTCPMasterPollReStart(rt_uint8_t ucPort)
{
    vMBTCPMasterPollStop(ucPort);
    modbus_t *mb_ctx = NULL;
    if (NET_IS_ENET(ucPort)) {
        mb_ctx = modbus_new_tcp(
            das_do_get_net_driver_name(DAS_NET_TYPE_ETH, 0),  
            g_tcpip_cfgs[ucPort].peer, 
            g_tcpip_cfgs[ucPort].port
        );
    }
#if NET_HAS_GPRS
    else if (NET_IS_GPRS(ucPort)) {
        mb_ctx = modbus_new_tcp(
            das_do_get_net_driver_name(DAS_NET_TYPE_GPRS, 0), 
            g_tcpip_cfgs[ucPort].peer, 
            g_tcpip_cfgs[ucPort].port
        );
    }
#endif
    if (mb_ctx) {
        g_modbus_tcp_user_data[ucPort].port_type = NET_IS_ENET(ucPort) ? WS_PORT_NET : WS_PORT_GPRS;
        g_modbus_tcp_user_data[ucPort].net_port = g_tcpip_cfgs[ucPort].port;
        g_modbus_tcp_user_data[ucPort].index  = ucPort;
        modbus_set_user_data(mb_ctx, &g_modbus_tcp_user_data[ucPort]);
        modbus_set_monitor(mb_ctx, &g_modbus_monitor);
        modbus_set_debug(mb_ctx, FALSE);
        modbus_set_error_recovery(mb_ctx, MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL);
        if (NET_IS_ENET(ucPort)) {
            modbus_set_response_timeout(mb_ctx, 0, 500000);
        } else {
            modbus_set_response_timeout(mb_ctx, 3, 0);
        }
        //modbus_set_slave(mb_ctx, g_tcpip_cfgs[ucPort].cfg.normal.maddress);
    }
    g_mb_mutex_tcp[ucPort] = rt_mutex_create();
    g_mb_ctx_tcp[ucPort] = mb_ctx;

    return RT_EOK;
}

void vMBTCPMasterPollStop(rt_uint8_t ucPort)
{
    if (g_mb_ctx_tcp[ucPort]) {
        modbus_close(g_mb_ctx_tcp[ucPort]);
        modbus_free(g_mb_ctx_tcp[ucPort]);
        g_mb_ctx_tcp[ucPort] = NULL;
        
        rt_mutex_delete(g_mb_mutex_tcp[ucPort]);
        g_mb_mutex_tcp[ucPort] = NULL;
    }
}

rt_err_t xMBRTU_OverTCPMasterPollReStart(rt_uint8_t ucPort)
{
    vMBTCPMasterPollStop(ucPort);

    modbus_t *mb_ctx = NULL;
    if (NET_IS_ENET(ucPort)) {
        mb_ctx = modbus_new_rtu_over_tcp(
            das_do_get_net_driver_name(DAS_NET_TYPE_ETH, 0),  
            g_tcpip_cfgs[ucPort].peer, 
            g_tcpip_cfgs[ucPort].port
        );
    }
#if NET_HAS_GPRS
    else if (NET_IS_GPRS(ucPort)) {
        mb_ctx = modbus_new_rtu_over_tcp(
            das_do_get_net_driver_name(DAS_NET_TYPE_GPRS, 0), 
            g_tcpip_cfgs[ucPort].peer, 
            g_tcpip_cfgs[ucPort].port
        );
    }
#endif
    if (mb_ctx) {
        g_modbus_tcp_user_data[ucPort].port_type = NET_IS_ENET(ucPort) ? WS_PORT_NET : WS_PORT_GPRS;
        g_modbus_tcp_user_data[ucPort].net_port = g_tcpip_cfgs[ucPort].port;
        g_modbus_tcp_user_data[ucPort].index  = ucPort;
        modbus_set_user_data(mb_ctx, &g_modbus_tcp_user_data[ucPort]);
        modbus_set_monitor(mb_ctx, &g_modbus_monitor);
        modbus_set_debug(mb_ctx, FALSE);
        modbus_set_error_recovery(mb_ctx, MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL);
        if (NET_IS_ENET(ucPort)) {
            modbus_set_response_timeout(mb_ctx, 0, 500000);
        } else {
            modbus_set_response_timeout(mb_ctx, 3, 0);
        }
        //modbus_set_slave(mb_ctx, g_tcpip_cfgs[ucPort].cfg.normal.maddress);
    }
    g_mb_mutex_tcp[ucPort] = rt_mutex_create();
    g_mb_ctx_tcp[ucPort] = mb_ctx;

    return RT_EOK;
}

rt_err_t modbus_write_registers_with_uart(int port, int slave,          int addr, int nb, const uint16_t * data)
{
    rt_err_t rc = -1;
    if (port < BOARD_UART_MAX && g_mb_ctx_uart[port] && g_mb_mutex_uart[port]) {
        rt_mutex_take(g_mb_mutex_uart[port]);
        modbus_set_slave(g_mb_ctx_uart[port], slave);
        rc = modbus_write_registers(g_mb_ctx_uart[port], addr, nb, data);
        rt_mutex_release(g_mb_mutex_uart[port]);
    }
    return rc;
}

/*
rt_err_t modbus_write_registers_with_tcp(int port, int slave,          int addr, int nb, const uint16_t * data)
{
    rt_err_t rc = -1;
    if (port < BOARD_TCPIP_MAX && g_mb_ctx_tcp[port] && g_mb_mutex_tcp[port]) {
        rt_mutex_take(g_mb_mutex_tcp[port]);
        modbus_set_slave(g_mb_ctx_tcp[port], slave);
        rc = modbus_write_registers(g_mb_ctx_tcp[port], addr, nb, data);
        rt_mutex_release(g_mb_mutex_tcp[port]);
    }
    return rc;
}*/


rt_err_t modbus_write_registers_with_tcp(int port, int slave,  int addr, int nb, const uint16_t * pdata)
{
    rt_err_t rc = -1;
    if (port < BOARD_TCPIP_MAX && g_mb_ctx_tcp[port] && g_mb_mutex_tcp[port]) {
        modbus_user_data_t *data = (modbus_user_data_t *)modbus_get_user_data(g_mb_ctx_tcp[port]);
        if (data && (data->port_type == WS_PORT_NET || data->port_type == WS_PORT_GPRS)) {
            if (g_tcpip_states[data->index] && g_tcpip_states[data->index]->eState == TCPIP_STATE_CONNED) {
                rt_mutex_take(g_mb_mutex_tcp[port]);
                modbus_set_slave(g_mb_ctx_tcp[port], slave);
                rc = modbus_write_registers(g_mb_ctx_tcp[port], addr, nb, pdata);
                rt_mutex_release(g_mb_mutex_tcp[port]);
            }
        }
    }
    return rc;
}





rt_err_t modbus_write_registers_with(int dev_type, int port, int slave, int addr, int nb, const uint16_t * data)
{
    if (PROTO_DEV_IS_RS(dev_type) || PROTO_DEV_IS_ZIGBEE(dev_type) || PROTO_DEV_IS_LORA(dev_type)) {
        return modbus_write_registers_with_uart(port, slave, addr, nb, data);
    } else if (PROTO_DEV_IS_NET(dev_type) || PROTO_DEV_IS_GPRS(dev_type)) {
        return modbus_write_registers_with_tcp(port, slave, addr, nb, data);
    }
}

rt_err_t modbus_read_registers_with_uart(int port, int slave,          int addr, int nb, uint16_t regs[128])
{
    rt_err_t rc = -1;
    if (port < BOARD_UART_MAX && g_mb_ctx_uart[port] && g_mb_mutex_uart[port]) {
        rt_mutex_take(g_mb_mutex_uart[port]);
        modbus_set_slave(g_mb_ctx_uart[port], slave);
        rc = modbus_read_registers(g_mb_ctx_uart[port], addr, nb, regs);
        rt_mutex_release(g_mb_mutex_uart[port]);
    }
    return rc;
}

rt_err_t modbus_read_registers_quick_with_uart(int port, int slave,          int addr, int nb, uint16_t regs[128], int usec)
{
    rt_err_t rc = -1;
    if (port < BOARD_UART_MAX && g_mb_ctx_uart[port] && g_mb_mutex_uart[port]) {
        rt_mutex_take(g_mb_mutex_uart[port]);
        modbus_set_slave(g_mb_ctx_uart[port], slave);
        modbus_set_response_timeout(g_mb_ctx_uart[port], 0, usec);
        rc = modbus_read_registers(g_mb_ctx_uart[port], addr, nb, regs);
        rt_mutex_release(g_mb_mutex_uart[port]);
    }
    return rc;
}

rt_err_t modbus_read_registers_with_tcp(int port, int slave,          int addr, int nb, uint16_t regs[128])
{
    rt_err_t rc = -1;
    if (port < BOARD_TCPIP_MAX && g_mb_ctx_tcp[port] && g_mb_mutex_tcp[port]) {
        rt_mutex_take(g_mb_mutex_tcp[port]);
        modbus_set_slave(g_mb_ctx_tcp[port], slave);
        rc = modbus_read_registers(g_mb_ctx_tcp[port], addr, nb, regs);
        rt_mutex_release(g_mb_mutex_tcp[port]);
    }
    return rc;
}

rt_err_t modbus_read_registers_with(int dev_type, int port, int dev_num, int slave, int addr, int nb)
{
    int rc;
    uint16_t regs[128] = {0};
    if (PROTO_DEV_IS_RS(dev_type) || PROTO_DEV_IS_ZIGBEE(dev_type) || PROTO_DEV_IS_LORA(dev_type)) {
        rc = modbus_read_registers_with_uart(port, slave, addr, nb, regs);
    } else if (PROTO_DEV_IS_NET(dev_type) || PROTO_DEV_IS_GPRS(dev_type)) {
        rc = modbus_read_registers_with_tcp(port, slave, addr, nb, regs);
    }
    if (rc > 0) {
        bVarManage_RefreshExtDataWithModbusMaster(
            (var_uint16_t)dev_type, 
            (var_uint16_t)dev_num, 
            (var_uint8_t)slave, 
            (var_uint16_t)addr, 
            (var_uint16_t)rc, 
            (var_uint8_t *)regs
        );
    }
    return rc;
}

rt_err_t modbus_read_input_registers_with_uart(int port, int slave,          int addr, int nb, uint16_t regs[128])
{
    rt_err_t rc = -1;
    if (port < BOARD_UART_MAX && g_mb_ctx_uart[port] && g_mb_mutex_uart[port]) {
        rt_mutex_take(g_mb_mutex_uart[port]);
        modbus_set_slave(g_mb_ctx_uart[port], slave);
        rc = modbus_read_input_registers(g_mb_ctx_uart[port], addr, nb, regs);
        rt_mutex_release(g_mb_mutex_uart[port]);
    }
    return rc;
}

rt_err_t modbus_read_input_registers_with_tcp(int port, int slave,          int addr, int nb, uint16_t regs[128])
{
    rt_err_t rc = -1;
    if (port < BOARD_TCPIP_MAX && g_mb_ctx_tcp[port] && g_mb_mutex_tcp[port]) {
        rt_mutex_take(g_mb_mutex_tcp[port]);
        modbus_set_slave(g_mb_ctx_tcp[port], slave);
        rc = modbus_read_input_registers(g_mb_ctx_tcp[port], addr, nb, regs);
        rt_mutex_release(g_mb_mutex_tcp[port]);
    }
    return rc;
}

rt_err_t modbus_read_input_registers_with(int dev_type, int port, int dev_num, int slave, int addr, int nb)
{
    int rc;
    uint16_t regs[128] = {0};
    if (PROTO_DEV_IS_RS(dev_type) || PROTO_DEV_IS_ZIGBEE(dev_type) || PROTO_DEV_IS_LORA(dev_type)) {
        rc = modbus_read_input_registers_with_uart(port, slave, addr, nb, regs);
    } else if (PROTO_DEV_IS_NET(dev_type) || PROTO_DEV_IS_GPRS(dev_type)) {
        rc = modbus_read_input_registers_with_tcp(port, slave, addr, nb, regs);
    }
    if (rc > 0) {
        bVarManage_RefreshExtDataWithModbusMaster(
            (var_uint16_t)dev_type, 
            (var_uint16_t)dev_num, 
            (var_uint8_t)slave, 
            (var_uint16_t)addr, 
            (var_uint16_t)rc, 
            (var_uint8_t *)regs
        );
    }
    return rc;
}

