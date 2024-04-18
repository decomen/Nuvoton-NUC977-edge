#include "board.h"
#include "net_helper.h"
#include "modbus.h"
#include "modbus_helper.h"

AIResultReg_t g_xAIResultReg;
DIResultReg_t g_xDIResultReg;
DOResultReg_t g_xDOResultReg; // = { .xDOResult = {1,1,1,1,0,0} };
NetCfgReg_t g_xNetCfgReg;


static rt_thread_t s_xMBRTUSlaveThread[BOARD_UART_MAX];
static rt_thread_t s_xMBTCPSlaveThread[BOARD_TCPIP_MAX];

extern modbus_customize_backend_t g_zgb_customize_backend;
extern modbus_customize_backend_t g_lora_customize_backend;

static int __modbus_poll_message(modbus_t *ctx, uint8_t *req);

static void __vMBRTU_ASCIISlavePollTask(rt_uint8_t ucPort, eMBMode eMode)
{
    uint8_t query[1024];
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
        if (eMode == MB_RTU) {
            if (g_uart_cfgs[ucPort].port_cfg.baud_rate < 9600) {
                modbus_set_response_timeout(mb_ctx, 1, 500000);
            } else {
                modbus_set_response_timeout(mb_ctx, 0, 500000);
            }
        } else {
            modbus_set_response_timeout(mb_ctx, 1, 0);
        }
        modbus_set_slave(mb_ctx, g_uart_cfgs[ucPort].slave_addr);
    }
    g_mb_mutex_uart[ucPort] = rt_mutex_create();
    g_mb_ctx_uart[ucPort] = mb_ctx;

    while (1) {
        if (modbus_connect(mb_ctx) == -1) {
            fprintf(stderr, "modbus rtu/ascii failed(%d): %s\n", ucPort, modbus_strerror(errno));
            modbus_close(mb_ctx);
            sleep(1);
            continue;
        }
        __modbus_poll_message(mb_ctx, query);
        modbus_close(mb_ctx);
        sleep(2);
    }
    s_xMBRTUSlaveThread[ucPort] = RT_NULL;
    rt_thddog_exit();
}

void vMBRTUSlavePollTask(void *parameter)
{
    rt_uint8_t ucPort = (rt_uint8_t)(long)parameter;
    __vMBRTU_ASCIISlavePollTask(ucPort, MB_RTU);
}

void vMBASCIISlavePollTask(void *parameter)
{
    rt_uint8_t ucPort = (rt_uint8_t)(long)parameter;
    __vMBRTU_ASCIISlavePollTask(ucPort, MB_ASCII);
}

rt_err_t xMBRTU_ASCIISlavePollReStart(rt_uint8_t ucPort, eMBMode eMode)
{
    vMBRTU_ASCIISlavePollStop(ucPort);

    if (RT_NULL == s_xMBRTUSlaveThread[ucPort]) {
        BOARD_CREAT_NAME(szPoll, "mbs_pl%d", ucPort);
        s_xMBRTUSlaveThread[ucPort] = rt_thread_create(
                                      szPoll,
                                      (MB_RTU == eMode) ? (vMBRTUSlavePollTask) : (vMBASCIISlavePollTask),
                                      (void *)(long)ucPort,
                                      1024, 20, 20
                                      );

        if (s_xMBRTUSlaveThread[ucPort]) {
            rt_thddog_register(s_xMBRTUSlaveThread[ucPort], 30);
            rt_thread_startup(s_xMBRTUSlaveThread[ucPort]);
            return RT_EOK;
        }
    }

    return RT_ERROR;
}

void vMBRTU_ASCIISlavePollStop(rt_uint8_t ucPort)
{
    if (s_xMBRTUSlaveThread[ucPort]) {
        rt_thddog_unregister(s_xMBRTUSlaveThread[ucPort]);
        if (RT_EOK == rt_thread_delete(s_xMBRTUSlaveThread[ucPort])) {
            s_xMBRTUSlaveThread[ucPort] = RT_NULL;
            if (g_mb_ctx_uart[ucPort]) {
                modbus_close(g_mb_ctx_uart[ucPort]);
                modbus_free(g_mb_ctx_uart[ucPort]);
                g_mb_ctx_uart[ucPort] = NULL;
                rt_mutex_delete(g_mb_mutex_uart[ucPort]);
                g_mb_mutex_uart[ucPort] = NULL;
            }
        }
    }
}

void vMBTCPSlavePollTask(void *parameter)
{
    rt_uint8_t ucPort = (rt_uint8_t)(long)parameter;

    uint8_t query[1024];
    modbus_t *mb_ctx = NULL;
    
    if (NET_IS_ENET(ucPort)) {
        mb_ctx = modbus_new_tcp(
            das_do_get_net_driver_name(DAS_NET_TYPE_ETH, 0), 
            NULL, 
            g_tcpip_cfgs[ucPort].port
        );
    }
#if NET_HAS_GPRS
    else if (NET_IS_GPRS(ucPort)) {
        mb_ctx = modbus_new_tcp(
            das_do_get_net_driver_name(DAS_NET_TYPE_GPRS, 0), 
            NULL, 
            g_tcpip_cfgs[ucPort].port
        );
    }
#endif
    if (mb_ctx) {
        g_modbus_tcp_user_data[ucPort].port_type = NET_IS_ENET(ucPort) ? WS_PORT_NET : WS_PORT_GPRS;
        g_modbus_tcp_user_data[ucPort].net_port = g_tcpip_cfgs[ucPort].port;
        g_modbus_tcp_user_data[ucPort].index = ucPort;
        modbus_set_user_data(mb_ctx, &g_modbus_tcp_user_data[ucPort]);
        modbus_set_monitor(mb_ctx, &g_modbus_monitor);
        modbus_set_debug(mb_ctx, FALSE);
        modbus_set_error_recovery(mb_ctx, MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL);
        modbus_set_slave(mb_ctx, g_tcpip_cfgs[ucPort].cfg.normal.maddress);
    }
    g_mb_mutex_tcp[ucPort] = rt_mutex_create();
    g_mb_ctx_tcp[ucPort] = mb_ctx;

    while (1) {
        int fd_sock = modbus_tcp_listen(mb_ctx, 1);
        if (fd_sock >= 0) {
            int fd_tcp = modbus_tcp_accept(mb_ctx, &fd_sock);
            if (fd_tcp >= 0) {
                __modbus_poll_message(mb_ctx, query);
                modbus_close(mb_ctx);
            }
            if (fd_sock >= 0) close(fd_sock);
        } else {
            sleep(1);
        }
    }
    s_xMBTCPSlaveThread[ucPort] = RT_NULL;
    rt_thddog_exit();
}

rt_err_t xMBTCPSlavePollReStart(rt_uint8_t ucPort)
{
    vMBTCPSlavePollStop(ucPort);
    if (RT_NULL == s_xMBTCPSlaveThread[ucPort]) {
        BOARD_CREAT_NAME(szPoll, "mbst_pl%d", ucPort);
        s_xMBTCPSlaveThread[ucPort] = rt_thread_create(szPoll, vMBTCPSlavePollTask, (void *)(long)ucPort, 0x300, 20, 20);

        if (s_xMBTCPSlaveThread[ucPort]) {
            rt_thddog_register(s_xMBTCPSlaveThread[ucPort], 30);
            rt_thread_startup(s_xMBTCPSlaveThread[ucPort]);
            return RT_EOK;
        }
    }

    return RT_ERROR;
}

void vMBTCPSlavePollStop(rt_uint8_t ucPort)
{
    if (s_xMBTCPSlaveThread[ucPort]) {
        rt_thddog_unregister(s_xMBTCPSlaveThread[ucPort]);
        if (RT_EOK == rt_thread_delete(s_xMBTCPSlaveThread[ucPort])) {
            s_xMBTCPSlaveThread[ucPort] = RT_NULL;
            if (g_mb_ctx_tcp[ucPort]) {
                modbus_close(g_mb_ctx_tcp[ucPort]);
                modbus_free(g_mb_ctx_tcp[ucPort]);
                g_mb_ctx_tcp[ucPort] = NULL;

                rt_mutex_delete(g_mb_mutex_tcp[ucPort]);
                g_mb_mutex_tcp[ucPort] = NULL;
            }
        }
    }
}

void vMBRTU_OverTCPSlavePollTask(void *parameter)
{
    rt_uint8_t ucPort = (rt_uint8_t)(long)parameter;

    uint8_t query[1024];
    modbus_t *mb_ctx = NULL;
    
    if (NET_IS_ENET(ucPort)) {
        mb_ctx = modbus_new_rtu_over_tcp(
            das_do_get_net_driver_name(DAS_NET_TYPE_ETH, 0), 
            NULL, 
            g_tcpip_cfgs[ucPort].port
        );
    }
#if NET_HAS_GPRS
    else if (NET_IS_GPRS(ucPort)) {
        mb_ctx = modbus_new_rtu_over_tcp(
            das_do_get_net_driver_name(DAS_NET_TYPE_GPRS, 0), 
            NULL, 
            g_tcpip_cfgs[ucPort].port
        );
    }
#endif
    if (mb_ctx) {
        g_modbus_tcp_user_data[ucPort].port_type = NET_IS_ENET(ucPort) ? WS_PORT_NET : WS_PORT_GPRS;
        g_modbus_tcp_user_data[ucPort].net_port = g_tcpip_cfgs[ucPort].port;
        g_modbus_tcp_user_data[ucPort].index = ucPort;
        modbus_set_user_data(mb_ctx, &g_modbus_tcp_user_data[ucPort]);
        modbus_set_monitor(mb_ctx, &g_modbus_monitor);
        modbus_set_debug(mb_ctx, FALSE);
        modbus_set_error_recovery(mb_ctx, MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL);
        modbus_set_slave(mb_ctx, g_tcpip_cfgs[ucPort].cfg.normal.maddress);
    }
    g_mb_mutex_tcp[ucPort] = rt_mutex_create();
    g_mb_ctx_tcp[ucPort] = mb_ctx;

    while (1) {
        int fd_sock = modbus_tcp_listen(mb_ctx, 1);
        if (fd_sock >= 0) {
            int fd_tcp = -1;
            modbus_tcp_accept(mb_ctx, &fd_tcp);
            if (fd_tcp >= 0) {
                __modbus_poll_message(mb_ctx, query);
                modbus_close(mb_ctx);
            }
            close(fd_sock);
        } else {
            sleep(1);
        }
    }

    s_xMBTCPSlaveThread[ucPort] = RT_NULL;
    rt_thddog_exit();
}

rt_err_t xMBRTU_OverTCPSlavePollReStart(rt_uint8_t ucPort)
{
    vMBTCPSlavePollStop(ucPort);

    if (RT_NULL == s_xMBTCPSlaveThread[ucPort]) {
        BOARD_CREAT_NAME(szPoll, "mbst_pl%d", ucPort);
        s_xMBTCPSlaveThread[ucPort] = rt_thread_create(szPoll, vMBRTU_OverTCPSlavePollTask, (void *)(long)ucPort, 0x300, 20, 20);

        if (s_xMBTCPSlaveThread[ucPort]) {
            rt_thddog_register(s_xMBTCPSlaveThread[ucPort], 30);
            rt_thread_startup(s_xMBTCPSlaveThread[ucPort]);
            return RT_EOK;
        }
    }

    return RT_ERROR;
}

void vBigEdianAIResult(AIResult_t *in, AIResult_t *out)
{
    for (int i = 0; i < 8; i++) {
        *(var_uint32_t *)&out->fAI_xx[i] = var_htonl(*(var_uint32_t *)&in->fAI_xx[i]);
    }
}

void vLittleEdianAIResult(AIResult_t *in, AIResult_t *out)
{
    vBigEdianAIResult(in, out);
}

void vBigEdianDIResult(const DIResult_t *in, DIResult_t *out)
{
    for (int i = 0; i < 8; i++) {
        out->usDI_xx[i] = var_htons(in->usDI_xx[i]);
    }
}

void vLittleEdianDIResult(DIResult_t *in, DIResult_t *out)
{
    vBigEdianDIResult(in, out);
}

void vBigEdianDOResult(DOResult_t *in, DOResult_t *out)
{
    for (int i = 0; i < 8; i++) {
        out->usDO_xx[i] = var_htons(in->usDO_xx[i]);
    }
}

void vLittleEdianDOResult(DOResult_t *in, DOResult_t *out)
{
    vBigEdianDOResult(in, out);
}

void vBigEdianNetCfg(NetCfg_t *in, NetCfg_t *out)
{
    out->dhcp = var_htons(in->dhcp);
    out->status = var_htons(in->status);
}

void vLittleEdianNetCfg(NetCfg_t *in, NetCfg_t *out)
{
    vBigEdianNetCfg(in, out);
}

#define __swap_8(_ary,_n,_m) {var_uint8_t _tmp=_ary[_n];_ary[_n]=_ary[_m];_ary[_m]=_tmp;}
#define __swap_16(_ary,_n,_m) {var_uint16_t _tmp=_ary[_n];_ary[_n]=_ary[_m];_ary[_m]=_tmp;}

void vBigEdianExtData(var_uint8_t btType, var_uint8_t btRule, VarValue_v *in, VarValue_v *out)
{
    switch (btType) {
    case E_VAR_INT16:
    case E_VAR_UINT16:
        out->val_u16 = var_htons(in->val_u16);
        break;

    case E_VAR_INT32:
    case E_VAR_UINT32:
    case E_VAR_FLOAT: {
        var_uint8_t tmp;
        out->val_u32 = in->val_u32;
        if(0 == btRule) {
            __swap_8(out->val_ary, 0, 3);
            __swap_8(out->val_ary, 1, 2);
        } else if(1 == btRule) {
            __swap_8(out->val_ary, 0, 1);
            __swap_8(out->val_ary, 2, 3);
        } else if(2 == btRule) {
            __swap_16(out->val_reg, 0, 1);
        } else if(3 == btRule) {
            ;
        }
    }
    break;
    case E_VAR_DOUBLE:
        out->val_db = in->val_db;
        // AB CD EF GH
        if(0 == btRule) {
            __swap_8(out->val_ary, 0, 7);
            __swap_8(out->val_ary, 1, 6);
            __swap_8(out->val_ary, 2, 5);
            __swap_8(out->val_ary, 3, 4);
        } else if(1 == btRule) {
            __swap_8(out->val_ary, 0, 1);
            __swap_8(out->val_ary, 2, 3);
            __swap_8(out->val_ary, 4, 5);
            __swap_8(out->val_ary, 6, 7);
        } else if(2 == btRule) {
            __swap_16(out->val_reg, 0, 3);
            __swap_16(out->val_reg, 1, 2);
        } else if(3 == btRule) {
            ;
        }
        break;
    default:
        if (in != out) {
            memcpy(out,in, sizeof(VarValue_v));
        }
    }
}

void vLittleEdianExtData(var_uint8_t btType, var_uint8_t btRule, VarValue_v *in, VarValue_v *out)
{
    vBigEdianExtData(btType, btRule, in, out);
}

static int __modbus_holding_callback(uint8_t *pucRegBuffer, uint16_t usAddress, uint16_t usNRegs,int read_flag)
{
    int eStatus = 0;
    unsigned short  iRegIndex;
    uint16_t *regs;
    UserRegData_t xUserReg;
    uint16_t usNRegsBak = usNRegs;

    //printf("__modbus_holding_callback : %02x%02x, %d,%d\n", pucRegBuffer[0], pucRegBuffer[1], usAddress, read_flag);

    if (MB_IN_USER_REG(AI, usAddress, usNRegs)) {
        iRegIndex = usAddress - USER_REG_AI_START;
        xUserReg.xAIResultReg = g_xAIResultReg;
        vBigEdianAIResult(&xUserReg.xAIResultReg.xAIResult, &xUserReg.xAIResultReg.xAIResult);
        regs = xUserReg.xAIResultReg.regs;
    } else if (MB_IN_USER_REG(DI, usAddress, usNRegs)) {
        iRegIndex = usAddress - USER_REG_DI_START;
        xUserReg.xDIResultReg = g_xDIResultReg;
        vBigEdianDIResult(&xUserReg.xDIResultReg.xDIResult, &xUserReg.xDIResultReg.xDIResult);
        regs = xUserReg.xDIResultReg.regs;
    } else if (MB_IN_USER_REG(DO, usAddress, usNRegs)) {
        iRegIndex = usAddress - USER_REG_DO_START;
        xUserReg.xDOResultReg = g_xDOResultReg;
        vBigEdianDOResult(&xUserReg.xDOResultReg.xDOResult, &xUserReg.xDOResultReg.xDOResult);
        regs = xUserReg.xDOResultReg.regs;
    } else if (MB_IN_USER_REG(NET_CFG, usAddress, usNRegs)) {
        iRegIndex = usAddress - USER_REG_NET_CFG_START;
        if (read_flag) {
            struct das_net_list_node net;
            memset(&net, 0, sizeof(net));
            das_do_get_net_info(DAS_NET_TYPE_ETH, 0, &net);
            if (!net.DHCP) strcpy(net.GATEWAY, g_net_cfg.gw);
            g_xNetCfgReg.xNetCfg.dhcp = net.DHCP ? 1 : 0;
            strcpy(g_xNetCfgReg.xNetCfg.ipaddr, net.IP);
            strcpy(g_xNetCfgReg.xNetCfg.netmask, net.MASK);
            strcpy(g_xNetCfgReg.xNetCfg.gw, net.GATEWAY);
            strcpy(g_xNetCfgReg.xNetCfg.dns, net.DNS1);
            g_xNetCfgReg.xNetCfg.status = das_string_equals(net.STATUS, DAS_NET_STATUS_UP, 1) ? 1 : 0;
        } else {
            g_xNetCfgReg.xNetCfg.dhcp = (g_net_cfg.dhcp ? 1 : 0);
            memcpy(g_xNetCfgReg.xNetCfg.ipaddr, g_net_cfg.ipaddr, 16);
            memcpy(g_xNetCfgReg.xNetCfg.netmask, g_net_cfg.netmask, 16);
            memcpy(g_xNetCfgReg.xNetCfg.gw, g_net_cfg.gw, 16);
            memcpy(g_xNetCfgReg.xNetCfg.dns, g_net_cfg.dns, 16);
            g_xNetCfgReg.xNetCfg.status = 1;
        }
        xUserReg.xNetCfgReg = g_xNetCfgReg;
        vBigEdianNetCfg(&xUserReg.xNetCfgReg.xNetCfg, &xUserReg.xNetCfgReg.xNetCfg);
        regs = xUserReg.xNetCfgReg.regs;
    } else if (MB_IN_USER_REG(EXT_DATA, usAddress, usNRegs)) {
        rt_enter_critical();
        if (bVarManage_CheckContAddr(usAddress, usNRegs)) {
            iRegIndex = usAddress - USER_REG_EXT_DATA_START;
            regs = &g_xExtDataRegs[0];
        } else {
            eStatus = -1;
        }
        rt_exit_critical();
    } else {
        eStatus = -1;
    }

    if (eStatus == 0) {
        if (read_flag) {
            rt_enter_critical();
            while (usNRegs > 0) {
                *pucRegBuffer++ = (uint8_t)(regs[iRegIndex] & 0xFF);
                *pucRegBuffer++ = (uint8_t)(regs[iRegIndex] >> 8 & 0xFF);
                iRegIndex++;
                usNRegs--;
            }
            rt_exit_critical();
        } else { //MB_REG_WRITE
            rt_enter_critical();
            while (usNRegs > 0) {
                regs[iRegIndex] = *pucRegBuffer++;
                regs[iRegIndex] |= *pucRegBuffer++ << 8;
                iRegIndex++;
                usNRegs--;
            }
            rt_exit_critical();

            if (MB_IN_USER_REG(AI, usAddress, usNRegsBak)) {
                // ro
                eStatus = -1;
            } else if (MB_IN_USER_REG(DI, usAddress, usNRegsBak)) {
                eStatus = -1;
            } else if (MB_IN_USER_REG(DO, usAddress, usNRegsBak)) {
                vLittleEdianDOResult(&xUserReg.xDOResultReg.xDOResult, &xUserReg.xDOResultReg.xDOResult);
                g_xDOResultReg.xDOResult = xUserReg.xDOResultReg.xDOResult;
                // notification DO thread
            } else if (MB_IN_USER_REG(NET_CFG, usAddress, usNRegsBak)) {
                vLittleEdianNetCfg(&xUserReg.xNetCfgReg.xNetCfg, &xUserReg.xNetCfgReg.xNetCfg);
                g_xNetCfgReg.xNetCfg = xUserReg.xNetCfgReg.xNetCfg;

                net_cfg_t bak = g_net_cfg;
                g_net_cfg.dhcp = (g_xNetCfgReg.xNetCfg.dhcp != 0);
                memcpy(g_net_cfg.ipaddr, g_xNetCfgReg.xNetCfg.ipaddr, 16);
                memcpy(g_net_cfg.netmask, g_xNetCfgReg.xNetCfg.netmask, 16);
                memcpy(g_net_cfg.gw, g_xNetCfgReg.xNetCfg.gw, 16);
                memcpy(g_net_cfg.dns, g_xNetCfgReg.xNetCfg.dns, 16);
                if (memcmp(&bak, &g_net_cfg, sizeof(g_net_cfg)) != 0) {
                    // 这里会复位设备
                    vSaveNetCfgToFs();
                }
            } else if (MB_IN_USER_REG(EXT_DATA, usAddress, usNRegsBak)) {
                bVarManage_RefreshExtDataWithModbusSlave( usAddress, usNRegsBak );
            }
        }
    }
    return eStatus;
}

static int __modbus_poll_message(modbus_t *ctx, uint8_t *req)
{
    int rc;
    int header_length = modbus_get_header_length(ctx);
    for (;;) {
        do {
            rc = modbus_receive(ctx, req);
        } while (rc == 0);
        if (rc == -1 && errno != ETIMEDOUT && 
            errno != EMBBADCRC && errno != EMBBADDATA &&
            errno != EMBBADEXC && errno != EMBUNKEXC &&
            errno != EMBMDATA && errno != EMBBADSLAVE) {
            break;
        }

        {
            #define MAX_MESSAGE_LENGTH 515
            modbus_mapping_t *mb_mapping = &g_modbus_map;
            int offset;
            int slave;
            int function;
            uint16_t address;
            uint8_t rsp[MAX_MESSAGE_LENGTH];
            
            offset = header_length;
            slave = req[offset - 1];
            function = req[offset];
            address = (req[offset + 1] << 8) + req[offset + 2];

            /* Data are flushed on illegal number of values errors. */
            switch (function) {
            case MODBUS_FC_READ_HOLDING_REGISTERS:
            case MODBUS_FC_READ_INPUT_REGISTERS: {
                unsigned int is_input = (function == MODBUS_FC_READ_INPUT_REGISTERS);
                int start_registers = is_input ? mb_mapping->start_input_registers : mb_mapping->start_registers;
                int nb_registers = is_input ? mb_mapping->nb_input_registers : mb_mapping->nb_registers;
                uint16_t *tab_registers = is_input ? mb_mapping->tab_input_registers : mb_mapping->tab_registers;
                
                int nb = (req[offset + 3] << 8) + req[offset + 4];
                int mapping_address = address - start_registers;
                
                if (nb < 1 || MODBUS_MAX_READ_REGISTERS < nb) {
                    ;
                } else if (mapping_address < 0 || (mapping_address + nb) > nb_registers) {
                    ;
                } else {
                    __modbus_holding_callback((uint8_t *)&tab_registers[mapping_address], address, nb, 1);
                }
            }
            break;
            case MODBUS_FC_WRITE_SINGLE_REGISTER: {
                int mapping_address = address - mb_mapping->start_registers;
                if (mapping_address < 0 || mapping_address >= mb_mapping->nb_registers) {
                    ;
                } else {
                    __modbus_holding_callback(&req[offset + 3], address, 1, 0);
                }
            }
            break;
            case MODBUS_FC_WRITE_MULTIPLE_REGISTERS: {
                int nb = (req[offset + 3] << 8) + req[offset + 4];
                int nb_write_bytes = req[offset + 5];
                int mapping_address = address - mb_mapping->start_registers;
                if (nb < 1 || MODBUS_MAX_WRITE_REGISTERS < nb ||
                    nb_write_bytes != nb * 2) {
                    ;
                } else if (mapping_address < 0 || (mapping_address + nb) > mb_mapping->nb_registers) {
                    ;
                } else {
                    __modbus_holding_callback(&req[offset + 6], address, nb, 0);
                }
            }
            break;
            case MODBUS_FC_WRITE_AND_READ_REGISTERS: {
                int nb = (req[offset + 3] << 8) + req[offset + 4];
                uint16_t address_write = (req[offset + 5] << 8) + req[offset + 6];
                int nb_write = (req[offset + 7] << 8) + req[offset + 8];
                int nb_write_bytes = req[offset + 9];
                int mapping_address = address - mb_mapping->start_registers;
                int mapping_address_write = address_write - mb_mapping->start_registers;

                if (nb_write < 1 || MODBUS_MAX_WR_WRITE_REGISTERS < nb_write ||
                    nb < 1 || MODBUS_MAX_WR_READ_REGISTERS < nb ||
                    nb_write_bytes != nb_write * 2) {
                    ;
                } else if (mapping_address < 0 || (mapping_address + nb) > mb_mapping->nb_registers ||
                           mapping_address < 0 || (mapping_address_write + nb_write) > mb_mapping->nb_registers) {
                    ;
                } else {
                    __modbus_holding_callback(&req[offset + 10], address_write, nb_write, 0);
                    __modbus_holding_callback((uint8_t *)&mb_mapping->tab_registers[mapping_address], address, nb, 1);
                }
            }
            break;
            default: break;
            }
        }
        
        rc = modbus_reply(ctx, req, rc, &g_modbus_map);
        if (rc == -1) {
            break;
        }
    }
    rt_kprintf("Quit the loop: %s\n", modbus_strerror(errno));
    return 0;
}

