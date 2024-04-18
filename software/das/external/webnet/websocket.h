
#ifndef __WEBSOCKET_H__
#define __WEBSOCKET_H__

#include "rtdef.h"

typedef enum {
    WEB_SK_CLOSED, 
    WEB_SK_HAND, 
    WEB_SK_CONNECTED,
} websocket_state_e;

typedef struct {
    rt_bool_t on;
    websocket_state_e   state;
} websocket_t;

typedef struct {
    rt_uint8_t          op      :4;
    rt_uint8_t          rsv     :3;
    rt_uint8_t          fin     :1;

    rt_uint8_t          len     :7;
    rt_uint8_t          mask    :1;
} websocket_head_t;

typedef struct {
    rt_uint8_t          rcv     :1;
    rt_uint8_t          rsv     :7;
} websocket_top_t;

typedef struct {
    websocket_head_t    head;
    
    rt_uint32_t         len;        // 长度
    rt_uint8_t          mask;       // 是否使用掩码
    rt_uint8_t          masks[4];   // 掩码
    rt_uint8_t          *data;      // 数据
} websocket_pack_t;

void websocket_set( struct webnet_session *session );
rt_bool_t websocket_ready( void );
void websocket_close( struct webnet_session *session );
void websocket_handler_data( struct webnet_session *session, const rt_uint8_t *data, rt_size_t len );
void websocket_send_data( struct webnet_session *session, const rt_uint8_t *data, rt_size_t len, rt_bool_t isrcv );
void websocket_send_string( struct webnet_session *session, const char *str );
void websocket_send_pong( struct webnet_session *session );

void ws_console_recv_pack(void *buffer, int size);
void ws_console_init( void );
int ws_console_open( void );
void ws_console_close( void );

#endif

