
#ifndef _HOST_CFG_H__
#define _HOST_CFG_H__

#define HOST_NAME_HEAD      "ST-8803"

typedef struct {
    char        szHostName[32];
    char        szId[65];           // has '\0'
    rt_bool_t   bSyncTime;
    rt_int32_t  nTimezone;
    char        szNTP[2][65];
    rt_bool_t   bDebug;
} host_cfg_t;

extern host_cfg_t g_host_cfg;

rt_err_t host_cfg_init( void );

#endif

