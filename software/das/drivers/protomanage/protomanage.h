#ifndef __PROTO_MANAGE_H__
#define __PROTO_MANAGE_H__

typedef enum {
    PROTO_SLAVE,            // SLAVE
    PROTO_MASTER,           // MASTER
} proto_ms_e;

typedef enum {
    PROTO_DEV_RS1       = 0, 
    PROTO_DEV_RS2       = 1, 
    PROTO_DEV_NET       = 2, 
    PROTO_DEV_ZIGBEE    = 3, 
    PROTO_DEV_GPRS      = 4, 
    PROTO_DEV_LORA      = 5, 



    PROTO_DEV_RTU_SELF      = 100,       // 本机驱动
    PROTO_DEV_RTU_SELF_MID  = 200,
} eProtoDevId;

#define PROTO_DEV_RS_MAX                (PROTO_DEV_RS2)
#define PROTO_DEV_IS_RS(_d)             ((_d) >= PROTO_DEV_RS1 && (_d)<=PROTO_DEV_RS_MAX)
#define PROTO_DEV_IS_NET(_d)            ((_d)==PROTO_DEV_NET)
#define PROTO_DEV_IS_ZIGBEE(_d)         ((_d)==PROTO_DEV_ZIGBEE)
#define PROTO_DEV_IS_LORA(_d)           ((_d)==PROTO_DEV_LORA)
#define PROTO_DEV_IS_GPRS(_d)           ((_d)==PROTO_DEV_GPRS)
#define PROTO_DEV_IS_RTU_SELF(_d)       ((_d)==PROTO_DEV_RTU_SELF)
#define PROTO_DEV_IS_RTU_SELF_MID(_d)   ((_d)==PROTO_DEV_RTU_SELF_MID)

#define PROTO_DEV_CHECK(_d)             ((_d)<=PROTO_DEV_GPRS || (_d)==PROTO_DEV_RTU_SELF_MID)

#define PROTO_DEV_NET_TYPE(_n)          ((_n)<BOARD_ENET_TCPIP_NUM?PROTO_DEV_NET:PROTO_DEV_GPRS)
#define PROTO_DEV_NET_INDEX(_n)         ((_n)<BOARD_ENET_TCPIP_NUM?(_n):((_n) - BOARD_ENET_TCPIP_NUM))

#endif



