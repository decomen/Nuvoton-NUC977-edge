#ifndef __DM_101_H__
#define __DM_101_H__

#include "varmanage.h"

#define DM101_INI_CFG_PATH_PREFIX         BOARD_CFG_PATH"rtu_dm101_"
#define DM101_FIFO_SIZE         (4096)      //fifo size
#define DM101_PARSE_STACK       (2048)      //解析任务内存占用

rt_bool_t dm101_open(rt_uint8_t index);
void dm101_close(rt_uint8_t index);
void dm101_startwork(rt_uint8_t index);
void dm101_exitwork(rt_uint8_t index);
rt_err_t dm101_put_bytes(rt_uint8_t index, rt_uint8_t *buffer, rt_uint16_t len);

typedef enum
{
    E_DM101_BEGAIN = 0x00,
    E_DM101_SERVER_FIND,
    E_DM101_REPORT_INFO,
    E_DM101_SET_TIME,
    E_DM101_REPORT_DATA,    
    
}eDm101InitStatus;

typedef struct
{
  rt_tick_t dm101_lastheart;
  rt_tick_t dm101_lastrecv;
  eDm101InitStatus eInitStatus;
} s_Dm101Status_t;

extern s_Dm101Status_t g_xDm101Status[BOARD_TCPIP_MAX];


#endif

