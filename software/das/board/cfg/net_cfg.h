
#ifndef __NET_CFG_H__
#define __NET_CFG_H__

#include <rtdef.h>

enum e_net_adapter {
    NET_ADAPTER_WIRED = 0,          //本地网络
    NET_ADAPTER_WIRELESS = 1,       //GPRS/LTE
};

typedef struct {
    rt_bool_t dhcp;         // 0 关, 1 开
    char ipaddr[16];
    char netmask[16];
    char gw[16];
    char dns[16];
    
    enum e_net_adapter adapter;
} net_cfg_t;

extern net_cfg_t g_net_cfg;

void vSaveNetCfgToFs( void );
rt_err_t net_cfg_init( void );
rt_err_t net_cfg_reset_net(void);
rt_err_t net_cfg_readd_dns(void);
rt_bool_t net_is_link(void);
enum e_net_adapter net_adapter_get(void);

#endif

