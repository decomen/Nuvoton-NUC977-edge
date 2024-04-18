#ifndef __MODEBUS_HELPER_H__
#define __MODEBUS_HELPER_H__

typedef struct _modbus_user_data {
    int port_type;
    int net_port;
    int index;
} modbus_user_data_t;

extern modbus_t *g_mb_ctx_uart[BOARD_UART_MAX];
extern rt_mutex_t g_mb_mutex_uart[BOARD_UART_MAX];

extern modbus_t *g_mb_ctx_tcp[BOARD_TCPIP_MAX];
extern rt_mutex_t g_mb_mutex_tcp[BOARD_TCPIP_MAX];

extern modbus_mapping_t g_modbus_map;
extern modbus_user_data_t g_modbus_rtu_user_data[BOARD_UART_MAX];
extern modbus_user_data_t g_modbus_tcp_user_data[BOARD_TCPIP_MAX];
extern modbus_monitor_t g_modbus_monitor;
extern uint16_t g_modbus_regs[];

#endif

