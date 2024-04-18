#include "board.h"
#include "obmodbus.h"
#include "modbus.h"
#include "obmodbus_helper.h"

rt_err_t xOBMBRTU_ASCIIMasterPollReStart(rt_uint8_t ucPort, eMBMode eMode)
{
    vOBMBRTU_ASCIIMasterPollStop(ucPort);

    obmodbus_t *mb_ctx = NULL;
    if (eMode == MB_RTU) {
        mb_ctx = obmodbus_new_rtu(
            das_do_get_uart_driver_name(ucPort), 
            g_uart_cfgs[ucPort].port_cfg.baud_rate, 
            das_do_get_uart_parity_char(g_uart_cfgs[ucPort].port_cfg.parity), 
            g_uart_cfgs[ucPort].port_cfg.data_bits, 
            g_uart_cfgs[ucPort].port_cfg.stop_bits
        );
    } else if (eMode == MB_ASCII) {
        mb_ctx = obmodbus_new_ascii(
            das_do_get_uart_driver_name(ucPort), 
            g_uart_cfgs[ucPort].port_cfg.baud_rate, 
            das_do_get_uart_parity_char(g_uart_cfgs[ucPort].port_cfg.parity), 
            g_uart_cfgs[ucPort].port_cfg.data_bits, 
            g_uart_cfgs[ucPort].port_cfg.stop_bits
        );
    }

    if (mb_ctx) {
        g_modbus_rtu_user_data[ucPort].port_type = DEV_TO_WS_PORT(ucPort);
        g_modbus_rtu_user_data[ucPort].net_port = -1;
        g_modbus_rtu_user_data[ucPort].index  = -1;
        obmodbus_set_user_data(mb_ctx, &g_modbus_rtu_user_data[ucPort]);
        obmodbus_set_monitor(mb_ctx, &g_obmodbus_monitor);
        obmodbus_set_debug(mb_ctx, FALSE);
        obmodbus_set_error_recovery(mb_ctx, OBMODBUS_ERROR_RECOVERY_LINK | OBMODBUS_ERROR_RECOVERY_PROTOCOL);
        if (eMode == MB_RTU) {
            if (g_uart_cfgs[ucPort].port_cfg.baud_rate < 9600) {
                obmodbus_set_response_timeout(mb_ctx, 1, 500000);
            } else {
                obmodbus_set_response_timeout(mb_ctx, 0, 500000);
            }
        } else {
            obmodbus_set_response_timeout(mb_ctx, 1, 0);
        }
        obmodbus_connect(mb_ctx);
        //obmodbus_set_slave(mb_ctx, g_uart_cfgs[ucPort].slave_addr);
    }
    g_obmb_mutex_uart[ucPort] = rt_mutex_create();
    g_obmb_ctx_uart[ucPort] = mb_ctx;
    return RT_EOK;
}

void vOBMBRTU_ASCIIMasterPollStop(rt_uint8_t ucPort)
{
    if (g_obmb_ctx_uart[ucPort]) {
        obmodbus_close(g_obmb_ctx_uart[ucPort]);
        obmodbus_free(g_obmb_ctx_uart[ucPort]);
        g_obmb_ctx_uart[ucPort] = NULL;

        rt_mutex_delete(g_obmb_mutex_uart[ucPort]);
        g_obmb_mutex_uart[ucPort] = NULL;
    }
}

rt_err_t obmodbus_read_registers_with_uart(int port, int slave, int function, int addr, int nb, uint8_t regs[256])
{
    rt_err_t rc = -1;
    if (port < BOARD_UART_MAX && g_obmb_ctx_uart[port] && g_obmb_mutex_uart[port]) {
        rt_mutex_take(g_obmb_mutex_uart[port]);
        obmodbus_set_slave(g_obmb_ctx_uart[port], slave);
        rc = obmodbus_read_registers(g_obmb_ctx_uart[port], function, addr, nb, regs);
        rt_mutex_release(g_obmb_mutex_uart[port]);
    }
    return rc;
}

rt_err_t obmodbus_read_registers_quick_with_uart(int port, int slave, int function, int addr, int nb, uint8_t regs[256], int usec)
{
    rt_err_t rc = -1;
    if (port < BOARD_UART_MAX && g_obmb_ctx_uart[port] && g_obmb_mutex_uart[port]) {
        rt_mutex_take(g_obmb_mutex_uart[port]);
        obmodbus_set_slave(g_obmb_ctx_uart[port], slave);
        obmodbus_set_response_timeout(g_obmb_ctx_uart[port], 0, usec);
        rc = obmodbus_read_registers(g_obmb_ctx_uart[port], function, addr, nb, regs);
        rt_mutex_release(g_obmb_mutex_uart[port]);
    }
    return rc;
}

rt_err_t obmodbus_read_registers_with(int dev_type, int port, int dev_num, int slave, int function, int addr, int nb)
{
    int rc = -1;
    uint8_t regs[256] = {0};
    if (PROTO_DEV_IS_RS(dev_type) || PROTO_DEV_IS_ZIGBEE(dev_type) || PROTO_DEV_IS_LORA(dev_type)) {
        rc = obmodbus_read_registers_with_uart(port, slave, function, addr, nb, regs);
    }
    if (rc > 0) {
        bVarManage_RefreshExtDataWithObModbusMaster(
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

