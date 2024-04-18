
#ifndef __RTU_REG_H__
#define __RTU_REG_H__

#include "rtdef.h"

// 该结构存在ROM中,不可清空
// code 可在必要时修改
typedef struct {
    rt_uint32_t     test_time;      // 运行时长,试用模式下增加
    rt_uint32_t     code;           // 随机码
} reg_info_t;

// 该结构存在SPI Flash中,恢复出厂可清空
typedef struct {
    rt_uint8_t      key[128+1];
    rt_uint32_t     reg_time;
} reg_t;

void reg_init( void );
rt_bool_t reg_check( const char *key );
rt_bool_t reg_reg( const char *key );
int reg_regetid( uint32_t code, rt_uint8_t id[40] );
rt_bool_t reg_testover( void );
void reg_testdo( rt_time_t sec );

extern reg_t g_reg;
extern reg_info_t g_reg_info;
extern rt_uint8_t g_regid[];
extern rt_bool_t g_isreg;
extern rt_bool_t g_istestover_poweron;

#endif

