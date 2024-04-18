#ifndef __SERIAL_HELPER_H__
#define __SERIAL_HELPER_H__

typedef struct serial {
    int             fd;
    uart_port_cfg_t cfg;
} serial_t ;

void serial_helper_init(void);
void serial_helper_open(int port);
void serial_helper_cfg(int port, uart_port_cfg_t *cfg);
void serial_helper_close(int port);
void serial_helper_tx_enable(int port);
void serial_helper_rx_enable(int port);
int serial_helper_send(int port, const void *data, rt_size_t size);
int serial_helper_select(int port, int usec);
int serial_helper_recv(int port, void *data, rt_size_t size);
int serial_helper_is_open(int port);

extern serial_t *g_serials[BOARD_UART_MAX];

#endif

