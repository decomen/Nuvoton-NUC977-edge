
#ifndef __GPRS_CFG_H__
#define __GPRS_CFG_H__

#include <rtdef.h>

typedef enum {
    GPRS_WM_FULL        ,   //全速
    GPRS_WM_LOW_POWER   ,   //低电流
    GPRS_WM_SLEEP       ,   //休眠
    GPRS_WM_SHUTDOWN    ,   //关机不使用

    GPRS_WM_MAX
} eGPRS_WorkMode;

typedef enum {
    GPRS_OM_AUTO    ,   //自动
    GPRS_OM_NEED    ,   //按需
    GPRS_OM_MANUAL  ,   //手动

    GPRS_OM_MAX
} eGPRS_OpenMode;

typedef struct {
    char szAPN[16];
    char szUser[16];
    char szPsk[16];
    char szAPNNo[22];
    char szMsgNo[22];
} gprs_net_cfg_t;

typedef struct {
    eGPRS_WorkMode  eWMode;         //工作模式
    eGPRS_OpenMode  eOMode;         //激活方式
    rt_uint8_t      btDebugLvl;     //调试等级
    char            szSIMNo[12];    //SIM卡号码 11位
    rt_uint32_t     ulInterval;     //数据帧时间
    rt_uint8_t      btRegLen;
    rt_uint8_t      btRegBuf[32];   //注册包 <= 64 byte
    rt_uint8_t      btHeartLen;
    rt_uint8_t      btHeartBuf[32]; //心跳包 <= 64 byte
    rt_uint32_t     ulRetry;        //0表示不重试
} gprs_work_cfg_t;

extern gprs_net_cfg_t g_gprs_net_cfg;
extern gprs_work_cfg_t g_gprs_work_cfg;

rt_err_t gprs_cfg_init( void );

#endif

