
#ifndef _WS_CFG_H__
#define _WS_CFG_H__

#include <rtdef.h>

typedef enum {
    WS_PORT_SHELL  = 0,      // shell
    WS_PORT_RS1    = 1, 
    WS_PORT_RS2    = 2, 
    WS_PORT_NET    = 3, 
    WS_PORT_ZIGBEE = 4, 
    WS_PORT_GPRS   = 5, 
    WS_PORT_LORA   = 6, 
} ws_port_e;

#define WS_PORT_RS_MIN          (WS_PORT_RS1)
#define WS_PORT_RS_MAX          (WS_PORT_RS2)

#define WS_PORT_TO_DEV(_n)     ((_n) <= WS_PORT_RS_MAX ? ((_n)-(WS_PORT_RS1-PROTO_DEV_RS1)) : (((_n)-(WS_PORT_RS1-PROTO_DEV_RS1)) - 1))
#define DEV_TO_WS_PORT(_n)     ((_n) <= PROTO_DEV_RS_MAX ? ((_n)+(WS_PORT_RS1-PROTO_DEV_RS1)) : ((_n)+(WS_PORT_RS1-PROTO_DEV_RS1) + 1))

//#define WS_PORT_TO_DEV(_n)     ((_n)-(WS_PORT_RS1-PROTO_DEV_RS1))
//#define DEV_TO_WS_PORT(_n)     ((_n)+(WS_PORT_RS1-PROTO_DEV_RS1))

typedef struct {
    rt_uint16_t     port;
} ws_port_net_cfg;

typedef struct {
    rt_uint16_t     port;
} ws_port_gprs_cfg;

typedef union {
    ws_port_net_cfg     net_cfg;
    ws_port_gprs_cfg    gprs_cfg;
} ws_port_cfg;

typedef struct {
    rt_bool_t       enable;
    ws_port_e       port_type;
    rt_uint16_t     listen_port;    // for net/gprs
    rt_uint16_t     listen_index;   // for net/gprs
} ws_cfg_t;

extern ws_cfg_t g_ws_cfg;

rt_err_t ws_cfg_init( void );

#endif

