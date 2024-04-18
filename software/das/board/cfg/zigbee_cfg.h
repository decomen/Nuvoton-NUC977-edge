
#ifndef _ZIGBEE_CFG_H__
#define _ZIGBEE_CFG_H__

typedef enum {
    ZGB_TM_GW       = 0x00, // 网关模式 (采集变量采集)
    ZGB_TM_TRT      = 0x01, // 无协议透明传输 (指定目标设备)
    ZGB_TM_DTU      = 0x02, // 组网转发模式 (转发到串口)

    ZGB_TM_NUM
} zgb_tmode_e;

typedef struct {
    ZIGBEE_DEV_INFO_T   xInfo;
    UCHAR               ucState;    //只读
    USHORT              usType;     //只读
    USHORT              usVer;      //只读
    rt_uint32_t         ulLearnStep;
    rt_uint8_t          btSlaveAddr;
    int                 nProtoType;
    zgb_tmode_e         tmode;
    eProtoDevId         dst_type;       // 转发端口
    xfer_dst_cfg        dst_cfg;        // 转发端口配置(串口需要该配置)
} zigbee_cfg_t;

extern zigbee_cfg_t g_zigbee_cfg;

rt_err_t zigbee_cfg_init( void );

#endif

