
#include <board.h>
#include <stdio.h>
#include <arpa/inet.h>

net_cfg_t g_net_cfg;
const net_cfg_t c_net_cfg_default = {
    .dhcp = RT_FALSE,
    .ipaddr = "192.168.8.10",
    .netmask = "255.255.255.0",
    //.gw = "192.168.1.1",
    //.dns = "209.96.128.86"
    .gw = "192.168.8.1",
    .dns = "8.8.8.8", 
    .adapter = NET_ADAPTER_WIRELESS, 
};

static void prvSetNetCfgDefault(void)
{
    memset(&g_net_cfg, 0, sizeof(g_net_cfg));
    g_net_cfg = c_net_cfg_default;
    /*if (g_xDevInfoReg.xDevInfo.xIp.szIp[0] == 0) {
        g_net_cfg.dhcp = RT_TRUE;
    } else {
        g_net_cfg.dhcp = RT_FALSE;
        sprintf(g_net_cfg.ipaddr, "%u.%u.%u.%u",
                g_xDevInfoReg.xDevInfo.xIp.szIp[0],
                g_xDevInfoReg.xDevInfo.xIp.szIp[1],
                g_xDevInfoReg.xDevInfo.xIp.szIp[2],
                g_xDevInfoReg.xDevInfo.xIp.szIp[3]);
    }*/
}

static void prvReadNetCfgFromFs(void)
{
    if (!board_cfg_read(NET_CFG_NAME, &g_net_cfg, sizeof(g_net_cfg))) {
        prvSetNetCfgDefault();
    }
#if TEST_ON_PC
    g_net_cfg.dhcp = 1;
#endif
}

static void prvSaveNetCfgToFs(void)
{
    if (!board_cfg_write(NET_CFG_NAME, &g_net_cfg, sizeof(g_net_cfg))) {
        ; //prvSetNetCfgDefault();
    }
}

void vSaveNetCfgToFs(void)
{
    prvSaveNetCfgToFs();
    vDoSystemReset();
}

enum e_net_adapter net_adapter_get(void)
{
    int result = das_do_check_enet_link();
    // check 0,  < 0 means error
    if (result == 0) {
        return NET_ADAPTER_WIRELESS;
    }
    return g_net_cfg.adapter;
}

rt_bool_t net_is_link(void)
{
    // check 0,  < 0 means error (if error return link ok)
    return (das_do_check_enet_link() != 0);
}

rt_err_t net_cfg_init(void)
{
    prvReadNetCfgFromFs();
#if !TEST_ON_PC
    net_cfg_reset_net();
#endif
    return RT_EOK;
}

int net_do_add_eth0_route()
{
    rt_kprintf("add eth0 routers\n");
    das_do_del_default_routes();
    char cmd[128] = {0};
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "route add default gw %s dev %s", g_net_cfg.gw, "eth0");
    system(cmd);
    printf("cmd: %s\r\n",cmd); 
}


int net_do_add_ppp0_route()
{
    rt_kprintf("add ppp0 routers\n");
    das_do_del_default_routes();
    char cmd[128] = {0};
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "route add default dev %s", "ppp0");
    system(cmd);
    printf("cmd: %s\r\n",cmd); 
}


int net_do_add_route(const char *dev)
{
    char cmd[128] = {0};
    
    switch (net_adapter_get()) {
    case NET_ADAPTER_WIRED: {
        rt_kprintf("add eth0 routers\n");
        if (das_string_equals(dev, das_do_get_net_driver_name(DAS_NET_TYPE_ETH, 0), 1)) {
            if (!g_net_cfg.dhcp) {
                das_do_del_default_routes();
                memset(cmd, 0, sizeof(cmd));
                sprintf(cmd, "route add default gw %s dev %s", g_net_cfg.gw, dev);
                system(cmd);
                printf("cmd: %s\r\n",cmd);
            }
        }
        break;
    }
    case NET_ADAPTER_WIRELESS: {
        rt_kprintf("add ppp0 routers\n");
        if (das_string_equals(dev, das_do_get_net_driver_name(DAS_NET_TYPE_LTE, 0), 1)) {
            if (!g_net_cfg.dhcp) {
                das_do_del_default_routes();
                memset(cmd, 0, sizeof(cmd));
                sprintf(cmd, "route add default dev %s", dev);
                system(cmd);
                printf("cmd: %s\r\n",cmd);
            }
        }
        break;
    }
    }
}

rt_err_t net_cfg_reset_net(void)
{
    struct das_net_list_node net;
    memset(&net, 0, sizeof(net));
    das_strcpy_s(net.TYPE, DAS_NET_TYPE_ETH);
    net.INDEX = 0;
    net.DHCP = g_net_cfg.dhcp ? 1 : 0;
    das_strcpy_s(net.IP, g_net_cfg.ipaddr);
    das_strcpy_s(net.MASK, g_net_cfg.netmask);
    das_strcpy_s(net.GATEWAY, g_net_cfg.gw);
    das_strcpy_s(net.DNS1, g_net_cfg.dns);
    das_do_set_net_info(&net);

    return -RT_EOK;
}


void jsonNetCfg(int n, cJSON *pItem)
{
    char netstatus[512] = {0};

    cJSON_AddNumberToObject(pItem, "dh", g_net_cfg.dhcp);
    cJSON_AddStringToObject(pItem, "ip", g_net_cfg.ipaddr);
    cJSON_AddStringToObject(pItem, "mask", g_net_cfg.netmask);
    cJSON_AddStringToObject(pItem, "gw", g_net_cfg.gw);
    cJSON_AddStringToObject(pItem, "dns", g_net_cfg.dns);

    {
        struct das_net_list_node net;
        char *p_status = netstatus;
        const char *interface_name = das_do_get_net_driver_name(DAS_NET_TYPE_ETH, 0);
        memset(&net, 0, sizeof(net));
        das_do_get_net_info(DAS_NET_TYPE_ETH, 0, &net);
        if (!net.DHCP) strcpy(net.GATEWAY, g_net_cfg.gw);
        p_status += sprintf(p_status,
                 "INTERFACE: %s<br />"
                 "MAC &nbsp;: %s<br />"
                 "FLAGS : %s %s<br />"
                 "I P &nbsp;: %s<br />",
                 interface_name,
                 net.MAC,
                 net.STATUS, net.DHCP ? "DHCP" : "",
                 net.IP
                );

        p_status += sprintf(p_status, "G W &nbsp;: %s", net.GATEWAY);
        p_status += sprintf(p_status, "<br />MASK&nbsp;: %s", net.MASK);
        p_status += sprintf(p_status, "<br />DNS1&nbsp;: %s", net.DNS1);
        p_status += sprintf(p_status, "<br />DNS2&nbsp;: %s", net.DNS2);
        p_status += sprintf(p_status, "<br />");
        cJSON_AddStringToObject(pItem, "status", netstatus);
    }
}

static void jsonNetInfo(cJSON *pItem)
{
    struct das_net_list_node net;
    memset(&net, 0, sizeof(net));
    das_do_get_net_info(DAS_NET_TYPE_ETH, 0, &net);
    if (!net.DHCP) strcpy(net.GATEWAY, g_net_cfg.gw);
    cJSON_AddNumberToObject(pItem, "dh", net.DHCP ? 1 : 0);
    cJSON_AddStringToObject(pItem, "mac", net.MAC);
    cJSON_AddStringToObject(pItem, "ip", net.IP);
    cJSON_AddStringToObject(pItem, "mask", net.MASK);
    cJSON_AddStringToObject(pItem, "gw", net.GATEWAY);
    cJSON_AddStringToObject(pItem, "d1", net.DNS1);
    cJSON_AddStringToObject(pItem, "d2", net.DNS2);
    cJSON_AddStringToObject(pItem, "lk", net.STATUS);
    
    cJSON_AddNumberToObject(pItem, "link", net_is_link() ? 1 : 0);
    cJSON_AddNumberToObject(pItem, "adpt", net_adapter_get());
}

// for webserver
DEF_CGI_HANDLER(getNetInfo)
{
    char *szRetJSON = RT_NULL;
    cJSON *pItem = cJSON_CreateObject();
    if(pItem) {
        cJSON_AddNumberToObject(pItem, "ret", RT_EOK);
        jsonNetInfo(pItem);
        szRetJSON = cJSON_PrintUnformatted(pItem);
        if(szRetJSON) {
            WEBS_PRINTF( szRetJSON );
            rt_free( szRetJSON );
        }
    }
    cJSON_Delete( pItem );

    WEBS_DONE(200);
}

DEF_CGI_HANDLER(getNetCfg)
{
    char *szRetJSON = RT_NULL;
    cJSON *pItem = cJSON_CreateObject();
    if(pItem) {
        cJSON_AddNumberToObject(pItem, "ret", RT_EOK);
        jsonNetCfg(0, pItem);
        szRetJSON = cJSON_PrintUnformatted(pItem);
        if(szRetJSON) {
            WEBS_PRINTF( szRetJSON );
            rt_free( szRetJSON );
        }
    }
    cJSON_Delete( pItem );

    WEBS_DONE(200);
}

void setNetCfgWithJson(cJSON *pCfg)
{
    int dh = cJSON_GetInt(pCfg, "dh", -1);
    const char *ip = cJSON_GetString(pCfg, "ip", VAR_NULL);
    const char *mask = cJSON_GetString(pCfg, "mask", VAR_NULL);
    const char *gw = cJSON_GetString(pCfg, "gw", VAR_NULL);
    const char *dns = cJSON_GetString(pCfg, "dns", VAR_NULL);
    int adpt = cJSON_GetInt(pCfg, "adpt", -1);

    net_cfg_t net_cfg_bak = g_net_cfg;

    if (dh >= 0) g_net_cfg.dhcp = (dh != 0 ? VAR_TRUE : VAR_FALSE);
    if (ip && strlen(ip) < 16) {
        memset(g_net_cfg.ipaddr, 0, 16); strcpy(g_net_cfg.ipaddr, ip);
    }
    if (mask && strlen(mask) < 16) {
        memset(g_net_cfg.netmask, 0, 16); strcpy(g_net_cfg.netmask, mask);
    }
    if (gw && strlen(gw) < 16) {
        memset(g_net_cfg.gw, 0, 16); strcpy(g_net_cfg.gw, gw);
    }
    if (dns && strlen(dns) < 16) {
        memset(g_net_cfg.dns, 0, 16); strcpy(g_net_cfg.dns, dns);
    }
    if (adpt >= 0) g_net_cfg.adapter = adpt;

    if (memcmp(&net_cfg_bak, &g_net_cfg, sizeof(g_net_cfg)) != 0) {
        prvSaveNetCfgToFs();
        net_cfg_reset_net();
        if (net_cfg_bak.adapter != g_net_cfg.adapter && g_net_cfg.adapter == NET_ADAPTER_WIRELESS) {
           // das_do_del_route(das_do_get_net_driver_name(DAS_NET_TYPE_LTE, 0));
           // net_do_add_route(das_do_get_net_driver_name(DAS_NET_TYPE_LTE, 0));
        }
    }
}

DEF_CGI_HANDLER(setNetCfg)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        setNetCfgWithJson(pCfg);
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}

#if NET_HAS_GPRS
extern void jsonGPRSState( cJSON *pItem );
#endif
extern void jsonTcpipState(int n, cJSON *pItem);

DEF_CGI_HANDLER(getAllNetInfo)
{
    char *szRetJSON = RT_NULL;
    WEBS_PRINTF("{\"ret\":0,\"netinfo\":");
    {
        cJSON *pItem = cJSON_CreateObject();
        if(pItem) {
            jsonNetInfo(pItem);
            szRetJSON = cJSON_PrintUnformatted(pItem);
            if(szRetJSON) {
                WEBS_PRINTF( szRetJSON );
                rt_free( szRetJSON );
            }
        }
        cJSON_Delete( pItem );
    }
#if NET_HAS_GPRS
    WEBS_PRINTF(",\"gprsinfo\":");
    {
        cJSON *pItem = cJSON_CreateObject();
        if(pItem) {
            jsonGPRSState(pItem);
            szRetJSON = cJSON_PrintUnformatted(pItem);
            if(szRetJSON) {
                WEBS_PRINTF( szRetJSON );
                rt_free( szRetJSON );
            }
        }
        cJSON_Delete( pItem );
    }
#endif
    WEBS_PRINTF(",\"tcpipinfo\":[");
    rt_bool_t first = RT_TRUE;
    for (int n = 0; n < BOARD_TCPIP_MAX; n++) {
        cJSON *pItem = cJSON_CreateObject();
        if(pItem) {
            if (!first) WEBS_PRINTF(",");
            first = RT_FALSE;
            jsonTcpipState(n, pItem);
            szRetJSON = cJSON_PrintUnformatted(pItem);
            if(szRetJSON) {
                WEBS_PRINTF( szRetJSON );
                rt_free( szRetJSON );
            }
        }
        cJSON_Delete( pItem );
    }
    WEBS_PRINTF("]}");
    WEBS_DONE(200);
}

