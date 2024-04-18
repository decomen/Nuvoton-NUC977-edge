
#ifndef __TCPIP_CFG_H__
#define __TCPIP_CFG_H__

#include <rtdef.h>

// TCP/IP 类型
typedef enum {
    TCP_IP_TCP, 
    TCP_IP_UDP, 
} tcpip_type_e;

// 通信协议
typedef enum {
    PROTO_MODBUS_TCP    = 0x00,    // modbus TCP
    PROTO_CC_BJDC       = 0x01,    // 北京数据采集协议
    PROTO_MODBUS_RTU_OVER_TCP = 0x02,    // modbus rtu over tcp
    PROTO_HJT212        = 0x03,
    PROTO_DM101         = 0x04,
    
    PROTO_SMF           = 0x05,
    
    PROTO_MQTT          = 0x06,
    
    PROTO_DH            = 0x07,
} proto_tcpip_type_e;

typedef enum {
    TCPIP_CLIENT,           //从
    TCPIP_SERVER            //主
} tcpip_cs_e;

typedef enum {
    TCPIP_STATE_WAIT, 
    TCPIP_STATE_CONNED, 
    TCPIP_STATE_ACCEPT, 
    TCPIP_STATE_CONNING, 
} tcpip_state_e;

typedef enum {
    TCP_IP_M_NORMAL     = 0x00, // 正常通信模式
    TCP_IP_M_XFER       = 0x01, // 转发模式 

    TCP_IP_M_NUM
} tcpip_mode_e;

typedef struct {
    proto_tcpip_type_e  proto_type; //协议
    proto_ms_e          proto_ms;   //主从
    rt_uint8_t          maddress;   // modbus rtu over tcp slave address
} tcpip_noamal_cfg_t;

#include "xfer_cfg.h"

typedef struct {
    rt_bool_t       enable;         // 0:关闭  1:开启
    tcpip_type_e    tcpip_type;     // 模式
    tcpip_cs_e      tcpip_cs;       // client or server
    char            peer[64];       // client时有效, 目标地址
    rt_uint16_t     port;           // 端口号

    rt_uint32_t     interval;       //
    rt_bool_t       keepalive;      //

    tcpip_mode_e    mode;
    union {
        tcpip_noamal_cfg_t  normal;
        xfer_cfg_t          xfer;
    } cfg;
} tcpip_cfg_t;

extern tcpip_cfg_t g_tcpip_cfgs[BOARD_TCPIP_MAX];

rt_err_t tcpip_cfg_init( void );
int is_tcpip_used_gprs(void);

#endif

