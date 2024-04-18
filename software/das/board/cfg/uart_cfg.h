
#ifndef __UART_CFG_H__
#define __UART_CFG_H__

#include <rtdef.h>
#include <stdint.h>
#include <protomanage.h>

// 串口类型
typedef enum {
    UART_TYPE_232,
    UART_TYPE_485,
    UART_TYPE_ZGB,
    UART_TYPE_GPRS,
    UART_TYPE_LORA
} uart_type_e;

// 通信协议
typedef enum {
    PROTO_MODBUS_RTU        = 0,    // modbus RTU
    PROTO_MODBUS_ASCII      = 1,    // modbus ASCII
    PROTO_DLT645            = 2,    // dlt645 电表 2007
    PROTO_DLT645_1997       = 3,    // dlt645 电表 1997
    PROTO_DUST              = 4,    // 粉尘协议
    
    PROTO_OBMODBUS_RTU      = 5,    // ob modbus RTU
    
    PROTO_MBUS603          = 6,    // PROTO_MBUS603

    PROTO_LUA = 20,             // lua
} proto_uart_type_e;

typedef struct {
    rt_uint32_t baud_rate;
    rt_uint8_t  data_bits   :4;
    rt_uint8_t  stop_bits   :2;
    rt_uint8_t  parity      :2;
} uart_port_cfg_t;

typedef struct {
    uart_port_cfg_t     port_cfg;

    uart_type_e         uart_type;
    proto_uart_type_e   proto_type;
    proto_ms_e          proto_ms;
    rt_uint8_t          slave_addr;     //从机modbus时有效, 从机地址
    rt_uint32_t         interval;

    char                lua_proto[32];
} uart_cfg_t;

#define DEFAULT_UART_CFG(_type) { 115200, 8, 1, 0, _type, PROTO_MODBUS_RTU, PROTO_SLAVE, 0, 1000, "" }

extern uart_cfg_t g_uart_cfgs[];

rt_err_t uart_cfg_init( void );
rt_int8_t nUartGetIndex(rt_int8_t instance);
rt_int8_t nUartGetInstance( rt_int8_t index );

#endif

