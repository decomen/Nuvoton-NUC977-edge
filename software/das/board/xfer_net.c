
#include "board.h"
#include "net_helper.h"

rt_bool_t xfer_net_open(int n)
{
    rt_kprintf("xfer_net_open : %d\n", n);
    xfer_net_close(n);

    net_open(n, XFER_BUF_SIZE, 0x400, "xf_net");

    return RT_TRUE;
}

void xfer_net_close(int n)
{
    net_close(n);
}

rt_bool_t xfer_net_send(int n, const rt_uint8_t *pucMBTCPFrame, rt_uint16_t usTCPLength)
{
    return (net_send(n, 0, (rt_uint8_t *)pucMBTCPFrame, usTCPLength) > 0);
}

#define ASC_TO_INT(_b)    (((_b)>='0'&&(_b)<='9')?((_b)-'0'):(((_b)>='A'&&(_b)<='F')?((_b)-'A'+0x0A):0))

rt_uint8_t ucMBAsciiLRC(rt_uint8_t *pucFrame, rt_uint16_t usLen)
{
    rt_uint8_t ucLRC = 0;

    // =1 for ':'
    // -4 for ':''\r''\n'
    for (int i = 1; i <= usLen - 3;) {
        rt_uint8_t ucByte0 = pucFrame[i++];
        rt_uint8_t ucByte1 = pucFrame[i++];

        ucByte0 = ASC_TO_INT(ucByte0);
        ucByte1 = ASC_TO_INT(ucByte1);

        ucLRC += (rt_uint8_t)((ucByte0 << 4) | ucByte1);
    }

    /* Return twos complement */
    ucLRC = (rt_uint8_t)(-((rt_uint8_t)ucLRC));
    return ucLRC;
}

rt_uint8_t ucMBAsciiAddr(rt_uint8_t *pucFrame)
{
    rt_uint8_t ucByte0 = pucFrame[1];
    rt_uint8_t ucByte1 = pucFrame[2];

    ucByte0 = ASC_TO_INT(ucByte0);
    ucByte1 = ASC_TO_INT(ucByte1);

    return (rt_uint8_t)((ucByte0 << 4) | ucByte1);
}

void __xfer_rcv_frame(int n, rt_uint8_t *frame, int len)
{
    rt_bool_t send_flag = RT_FALSE;
    rt_uint8_t slave_addr = 0;
    ezgb_sn_type_t zgb_sn_type = 0;
    elora_sn_type_t lora_sn_type = 0;
    xfer_cfg_t *xfer = &g_tcpip_cfgs[n].cfg.xfer;
    if(XFER_M_GW == xfer->mode) {
        if (PROTO_DEV_IS_RS(xfer->cfg.gw.dst_type) 
		    || PROTO_DEV_IS_ZIGBEE(xfer->cfg.gw.dst_type) 
			|| PROTO_DEV_IS_LORA(xfer->cfg.gw.dst_type)
            || PROTO_DEV_IS_NET(xfer->cfg.gw.dst_type)
            || PROTO_DEV_IS_GPRS(xfer->cfg.gw.dst_type)) {
            if (XFER_PROTO_MODBUS_RTU == xfer->cfg.gw.proto_type) {
                if (len >= MB_RTU_PDU_SIZE_MIN &&
                    (das_crc16((uint8_t *)&frame[0], len) == 0)) {
                    slave_addr = frame[MB_RTU_PDU_ADDR_OFF];
                    zgb_sn_type = ZGB_SN_T_MODBUS_RTU;
                    lora_sn_type = LORA_SN_T_MODBUS_RTU;
                    send_flag = RT_TRUE;
                }
            } else if (XFER_PROTO_MODBUS_ASCII == xfer->cfg.gw.proto_type) {
                if (len >= MB_ASCII_PDU_SIZE_MIN &&
                    (ucMBAsciiLRC(&frame[0], len) == 0)) {
                    slave_addr = ucMBAsciiAddr(frame);
                    zgb_sn_type = ZGB_SN_T_MODBUS_ASCII;
                    lora_sn_type = LORA_SN_T_MODBUS_ASCII;
                    send_flag = RT_TRUE;
                }
            }
            if (send_flag) {
                if (PROTO_DEV_IS_ZIGBEE(xfer->cfg.gw.dst_type)) {
                    if (slave_addr >= MB_ADDRESS_MIN && slave_addr <= MB_ADDRESS_MAX) {
                        zgb_mnode_t *mnode = zgb_mnode_find_with_addr(zgb_sn_type, slave_addr);
                        if (mnode != RT_NULL && mnode->online) {
                            if (0 == ucZigbeeSetDstAddr(mnode->netid)) {
                                vZigbee_SendBuffer(frame, len);
                            }
                        }
                    }
                } else if (PROTO_DEV_IS_LORA(xfer->cfg.gw.dst_type)) {
                    if (slave_addr >= MB_ADDRESS_MIN && slave_addr <= MB_ADDRESS_MAX) {
                        lora_mnode_t *mnode = lora_mnode_find_with_addr(lora_sn_type, slave_addr);
                        if (mnode != RT_NULL && mnode->online) {
                            if (0 == lora_set_dst_addr(mnode->netid)) {
                                lora_send_buffer(frame, len, LORA_STD_MSG_DATA_TRT);
                            }
                        }
                    }
                } else if (PROTO_DEV_IS_RS(xfer->cfg.gw.dst_type)) {
                    xfer_dst_serial_send(nUartGetInstance(xfer->cfg.gw.dst_type), (const void *)frame, len);
                } else if (PROTO_DEV_IS_NET(xfer->cfg.gw.dst_type)) {
                    int dst_idx = xfer->cfg.gw.dst_cfg.tcpip_cfg.idx;
                    //rt_kprintf("xfer_gprs_to_enet_check 1(%d) = %d\n", n, xfer_gprs_to_enet_check(n));
                    if (xfer_gprs_to_enet_check(n - BOARD_ENET_TCPIP_NUM)) {
                        xfer_net_send(dst_idx, (const rt_uint8_t *)frame, len);
                    }
                } else if (PROTO_DEV_IS_GPRS(xfer->cfg.gw.dst_type)) {
                    int dst_idx = xfer->cfg.gw.dst_cfg.tcpip_cfg.idx + BOARD_ENET_TCPIP_NUM;
                    //rt_kprintf("xfer_enet_to_gprs_check 2(%d) = %d\n", n, xfer_enet_to_gprs_check(n));
                    if (xfer_enet_to_gprs_check(n)) {
                        xfer_net_send(dst_idx, (const rt_uint8_t *)frame, len);
                    }
                }
            }
        }
    } else if(XFER_M_TRT == xfer->mode) {
        if (PROTO_DEV_IS_RS(xfer->cfg.trt.dst_type)) {
            xfer_dst_serial_send(nUartGetInstance(xfer->cfg.trt.dst_type), (const void *)frame, len);
        } else if (PROTO_DEV_IS_ZIGBEE(xfer->cfg.trt.dst_type) && ZIGBEE_WORK_COORDINATOR == g_zigbee_cfg.xInfo.WorkMode) {
            ucZigbeeMode(ZIGBEE_MSG_MODE_BROAD);
            rt_thread_delay(10 * RT_TICK_PER_SECOND / 1000);
            vZigbee_SendBuffer(frame, len);
            rt_thread_delay(100 * RT_TICK_PER_SECOND / 1000);
            ucZigbeeMode(ZIGBEE_MSG_MODE_SINGLE);
        } else if (PROTO_DEV_IS_LORA(xfer->cfg.trt.dst_type) && LORA_WORK_CENTRAL == g_lora_cfg.work_mode) {
            lora_set_broad_mode();
            rt_thread_delay(10 * RT_TICK_PER_SECOND / 1000);
            lora_send_buffer(frame, len, LORA_STD_MSG_DATA_TRT);
            rt_thread_delay(100 * RT_TICK_PER_SECOND / 1000);
            lora_set_single_mode();
        } else if (PROTO_DEV_IS_NET(xfer->cfg.trt.dst_type)) {
            int dst_idx = xfer->cfg.trt.dst_cfg.tcpip_cfg.idx;
            //rt_kprintf("xfer_gprs_to_enet_check 3(%d) = %d\n", n, xfer_gprs_to_enet_check(n));
            if (xfer_gprs_to_enet_check(n - BOARD_ENET_TCPIP_NUM)) {
                xfer_net_send(dst_idx, (const rt_uint8_t *)frame, len);
            }
        } else if (PROTO_DEV_IS_GPRS(xfer->cfg.trt.dst_type)) {
            int dst_idx = xfer->cfg.trt.dst_cfg.tcpip_cfg.idx + BOARD_ENET_TCPIP_NUM;
            //rt_kprintf("xfer_enet_to_gprs_check 4(%d) = %d\n", n, xfer_enet_to_gprs_check(n));
            if (xfer_enet_to_gprs_check(n)) {
                xfer_net_send(dst_idx, (const rt_uint8_t *)frame, len);
            }
        }
    }
}

