
#ifndef __WS_VM_H__
#define __WS_VM_H__

#include <rtdef.h>
#include <websocket.h>

void ws_vm_recv_pack( websocket_pack_t *pack );
rt_size_t ws_vm_buflen( void );
// 需要外部 rt_hw_interrupt_disable
int ws_vm_getc( void );
// 需要外部 rt_hw_interrupt_disable
rt_bool_t ws_vm_rcv_putc( rt_uint8_t ch );
rt_bool_t ws_vm_snd_putc( rt_uint8_t ch );
rt_size_t ws_vm_read( rt_off_t pos, void *buffer, rt_size_t size );
rt_size_t ws_vm_rcv_write( rt_off_t pos, void *buffer, rt_size_t size );
rt_size_t ws_vm_snd_write( rt_off_t pos, void *buffer, rt_size_t size );
void ws_vm_rx_clear( void );
void ws_vm_tx_clear( void );
void ws_vm_clear( void );
void ws_vm_init( void );

#endif
