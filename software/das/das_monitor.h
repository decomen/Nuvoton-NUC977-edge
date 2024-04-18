#ifndef __DAS_MONITOR_H__
#define __DAS_MONITOR_H__

#define MONITOR_FILE_TOTAL_SIZE     (4 * 1024 * 1024)

#define MONITOR_UART_LINE_LEN       (512)
#define MONITOR_UART_FIFO_LEN       (100 * 1024)
#define MONITOR_UART_FILE_SIZE      (512 * 1024)

typedef union {
    uint8_t     uart_enable[BOARD_UART_MAX];
    uint8_t     tcpip_enable[BOARD_TCPIP_MAX];
} monitor_cfg_t;

int monitor_init(void);
void monitor_cfg_set_default(void);
void monitor_cfg_read_from_fs(void);
void monitor_cfg_save_to_fs(void);
bool monitor_uart_recv_data(int com, uint8_t *data, int d_len);
bool monitor_uart_send_data(int com, uint8_t *data, int d_len);

#endif
