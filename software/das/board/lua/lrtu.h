#ifndef __LUA_PROTO_H__
#define __LUA_PROTO_H__

#include "rtdef.h"

typedef enum {
    LUA_RTU_DOEXP,      // 执行表达式
    LUA_RTU_DATA,       // 数据
} lua_rtu_msgtype_e;

// 通过 malloc 分配 buf 空间大小
typedef struct {
    rt_uint8_t      msg_type;
    rt_uint8_t      dev_type;
    rt_uint8_t      dev_num;
    rt_uint16_t     size;
    rt_uint16_t     n;
    rt_uint8_t      buf[1];
} lua_rtu_queue_t;

void lua_rtu_doexp(char *exp);
void lua_rtu_recvdata(lua_rtu_queue_t *queue);

#endif

