
#ifndef __XFER_CFG_H__
#define __XFER_CFG_H__

#include <rtdef.h>
#include <uart_cfg.h>
#include <protomanage.h>

#define XFER_BUF_SIZE       (10240)

typedef enum {
    XFER_M_GW       = 0x00, // 指定协议网关模式 (指定端口:支持zigbee)
    XFER_M_TRT      = 0x01, // 无协议透明传输 (指定端口)

    XFER_M_NUM
} xfer_mode_e;

// 通信协议
typedef enum {
    XFER_PROTO_NONE,            // none
    XFER_PROTO_MODBUS_RTU,      // modbus RTU
    XFER_PROTO_MODBUS_ASCII,    // modbus ASCII
    
} xfer_proto_e;

typedef struct {
    uart_port_cfg_t     cfg;        // 串口配置
    uart_type_e         type;       // 串口类型
} xfer_dst_uart_cfg;

typedef struct {
    uint8_t             idx;        //组号
} xfer_dst_tcpip_cfg;

typedef union {
    xfer_dst_uart_cfg   uart_cfg;   // 串口配置
    xfer_dst_tcpip_cfg  tcpip_cfg;
} xfer_dst_cfg;

// 指定协议网关模式 (指定端口:支持zigbee)
typedef struct {
    xfer_proto_e    proto_type;     // 协议
    eProtoDevId     dst_type;       // 转发端口
    xfer_dst_cfg    dst_cfg;        // 转发端口配置(串口需要该配置)
} xfer_gw_cfg_t;

// 无协议透明传输 (指定端口)
typedef struct {
    eProtoDevId     dst_type;       // 转发端口
    xfer_dst_cfg    dst_cfg;        // 转发端口配置(串口需要该配置)
} xfer_trt_cfg_t;

typedef struct {
    xfer_mode_e     mode;           // 转发模式
    union {
        xfer_gw_cfg_t   gw;
        xfer_trt_cfg_t  trt;
    } cfg;
} xfer_cfg_t;

#define XFER_UART_ADDRS_NUM     (64)

typedef struct {
    rt_bool_t   enable;
    rt_int32_t  count;
    rt_uint8_t  addrs[XFER_UART_ADDRS_NUM];
} xfer_uart_cfg_t;

extern xfer_uart_cfg_t g_xfer_uart_cfgs[];
extern rt_int8_t g_xfer_net_dst_uart_map[];
extern rt_bool_t g_xfer_net_dst_uart_occ[];
extern rt_bool_t g_xfer_net_dst_uart_trt[];
extern rt_bool_t g_xfer_net_dst_uart_dtu[];

rt_err_t xfer_uart_cfg_init( void );
int xfer_get_uart_with_slave_addr(rt_uint8_t slave_addr);

#endif

