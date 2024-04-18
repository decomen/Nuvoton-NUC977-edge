
#include "board.h"
#include "bfifo.h"

#define WS_VM_RX_BUFSZ        (4096)
#define WS_VM_TX_BUFSZ        (4096)

static bfifo_t _ws_vm_rx_queue = NULL;
static bfifo_t _ws_vm_tx_rcv_queue = NULL;
static bfifo_t _ws_vm_tx_snd_queue = NULL;
static rt_bool_t _ws_init = RT_FALSE;

void ws_vm_recv_pack( websocket_pack_t *pack )
{
    if( g_ws_cfg.enable ) {
        bfifo_push(_ws_vm_rx_queue, (const unsigned char *)pack->data, pack->len, 0);
    }

    if( WS_PORT_SHELL == g_ws_cfg.port_type ) {
        ws_console_recv_pack(pack->data, pack->len);
    }
}

rt_size_t ws_vm_buflen( void )
{
    return bfifo_count(_ws_vm_rx_queue);
}

int ws_vm_getc( void )
{
    if( !_ws_init ) return -1;
    
    rt_uint8_t ch = 0;
    if(bfifo_pull(_ws_vm_rx_queue, (unsigned char *)&ch, 1, 0) <= 0) {
        return -1;
    }
    
    return ch;
}

rt_bool_t ws_vm_rcv_putc( rt_uint8_t ch )
{
    if( !_ws_init ) return RT_FALSE;
    bfifo_push(_ws_vm_tx_rcv_queue, (const unsigned char *)&ch, 1, 0);
    return RT_TRUE;
}
rt_bool_t ws_vm_snd_putc( rt_uint8_t ch )
{
    if( !_ws_init ) return RT_FALSE;
    bfifo_push(_ws_vm_tx_snd_queue, (const unsigned char *)&ch, 1, 0);
    return RT_TRUE;
}

rt_size_t ws_vm_read( rt_off_t pos, void *buffer, rt_size_t size )
{
    if( !_ws_init ) return 0;
    if( g_ws_cfg.enable ) {
        return bfifo_pull(_ws_vm_rx_queue, (unsigned char *)buffer, size, 0);
    }
    return 0;
}

rt_size_t ws_vm_rcv_write( rt_off_t pos, void *buffer, rt_size_t size )
{
    if( !_ws_init ) return 0;

    if( g_ws_cfg.enable && size > 0 ) {
        return bfifo_push(_ws_vm_tx_rcv_queue, (const unsigned char *)buffer, size, 0);
    }
    return 0;
}

rt_size_t ws_vm_snd_write( rt_off_t pos, void *buffer, rt_size_t size )
{
    if( !_ws_init ) return 0;

    if( g_ws_cfg.enable && size > 0 ) {
        return bfifo_push(_ws_vm_tx_snd_queue, (const unsigned char *)buffer, size, 0);
    }
    return 0;
}

void ws_vm_rx_clear( void )
{
    bfifo_reset( _ws_vm_rx_queue );
}

void ws_vm_tx_clear( void )
{
    bfifo_reset( _ws_vm_tx_rcv_queue );
    bfifo_reset( _ws_vm_tx_snd_queue );
}

void ws_vm_clear( void )
{
    bfifo_reset( _ws_vm_rx_queue );
    bfifo_reset( _ws_vm_tx_rcv_queue );
    bfifo_reset( _ws_vm_tx_snd_queue );
}

static void _ws_vm_trans_rcv_task( void* parameter );
static void _ws_vm_trans_snd_task( void* parameter );

void ws_vm_init( void )
{
    _ws_vm_rx_queue = bfifo_create(WS_VM_RX_BUFSZ);
    _ws_vm_tx_rcv_queue = bfifo_create(WS_VM_TX_BUFSZ);
    _ws_vm_tx_snd_queue = bfifo_create(WS_VM_TX_BUFSZ);
    ws_vm_clear();

    rt_thread_t ws_rcv_thread = rt_thread_create( "wsrcvtsk", _ws_vm_trans_rcv_task, RT_NULL, 0x200, 10, 10 );
    if( ws_rcv_thread != RT_NULL ) {
        rt_thddog_register(ws_rcv_thread, 30);
        rt_thread_startup( ws_rcv_thread );
    }
    rt_thread_t ws_snd_thread = rt_thread_create( "wssndtsk", _ws_vm_trans_snd_task, RT_NULL, 0x200, 10, 10 );
    if( ws_snd_thread != RT_NULL ) {
        rt_thddog_register(ws_snd_thread, 30);
        rt_thread_startup( ws_snd_thread );
    }

    _ws_init = RT_TRUE;
}

static void _ws_vm_trans_rcv_task( void* parameter )
{
    static rt_uint8_t data[2048];

    while(1) {
        if (websocket_ready() && g_ws_cfg.enable) {
            rt_thddog_suspend("bfifo_pull _ws_vm_tx_rcv_queue");
            int n = bfifo_pull(_ws_vm_tx_rcv_queue, (unsigned char *)data, sizeof(data), -1);
            rt_thddog_resume();
            if( n > 0 ) {
                rt_thddog_feed("websocket_send_data");
                websocket_send_data( RT_NULL, (const rt_uint8_t *)data, n, RT_TRUE );
            }
        } else {
            sleep(1);
        }
    }
}

static void _ws_vm_trans_snd_task( void* parameter )
{
    static rt_uint8_t data[2048];

    while(1) {
        if (websocket_ready() && g_ws_cfg.enable) {
            rt_thddog_suspend("bfifo_pull _ws_vm_tx_snd_queue");
            int n = bfifo_pull(_ws_vm_tx_snd_queue, (unsigned char *)data, sizeof(data), -1);
            rt_thddog_resume();
            if( n > 0 ) {
                rt_thddog_feed("websocket_send_data");
                websocket_send_data( RT_NULL, (const rt_uint8_t *)data, n, RT_FALSE );
            }
        } else {
            sleep(1);
        }
    }
}

