
#ifndef _LORA_CFG_H__
#define _LORA_CFG_H__

typedef enum {
    LORA_TM_GW       = 0x00, // 网关模式 (采集变量采集)
    LORA_TM_TRT      = 0x01, // 无协议透明传输 (指定目标设备)
    LORA_TM_DTU      = 0x02, // 组网转发模式 (转发到串口)

    LORA_TM_NUM
} lora_tmode_e;

typedef struct {
    lora_dev_info       info;
    LORA_WORK_TYPE_E    work_mode;
    uint32_t            dst_addr;
    rt_uint32_t         learnstep;
    rt_uint8_t          slave_addr;
    int                 proto_type;
    lora_tmode_e        tmode;
    eProtoDevId         dst_type;       // 转发端口
    xfer_dst_cfg        dst_cfg;        // 转发端口配置(串口需要该配置)
    int                 interval;       // 采集间隔
} lora_cfg_t;

extern const lora_cfg_t c_lora_default_cfg;
extern lora_cfg_t g_lora_cfg;

rt_err_t lora_cfg_init( void );

#endif


