#ifndef __OBMODEBUS_HELPER_H__
#define __OBMODEBUS_HELPER_H__

#include "modbus_helper.h"

extern obmodbus_t *g_obmb_ctx_uart[BOARD_UART_MAX];
extern rt_mutex_t g_obmb_mutex_uart[BOARD_UART_MAX];

extern obmodbus_monitor_t g_obmodbus_monitor;

#endif

