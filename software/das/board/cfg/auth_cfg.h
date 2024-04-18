
#ifndef _AUTH_CFG_H__
#define _AUTH_CFG_H__

typedef struct {
    char szUser[32];
    char szPsk[128];
    rt_bool_t bUseSSH;
    rt_bool_t bUseSSHCer;
    rt_base_t nSSHPort;
    rt_uint32_t nTime;
} auth_cfg_t;

extern auth_cfg_t g_auth_cfg;

rt_err_t auth_cfg_init( void );

#endif

