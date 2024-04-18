
#include <board.h>
#include <stdio.h>

tcpip_cfg_t g_tcpip_cfgs[BOARD_TCPIP_MAX];
static tcpip_cfg_t s_tcpip_cfgs[BOARD_TCPIP_MAX];
const tcpip_cfg_t c_tcpip_cfg_default = {
    .enable = RT_FALSE,
    .tcpip_type = TCP_IP_TCP,
    .tcpip_cs = TCPIP_CLIENT,
    .peer = "",
    .port = 0,
    .interval = 0,
    .keepalive = RT_TRUE,
    .mode = TCP_IP_M_NORMAL,
    .cfg.normal = {
        .proto_type = PROTO_MODBUS_TCP,
        .proto_ms = PROTO_SLAVE,
        .maddress = 1,
    }
};

static void prvSetTcpipCfgsDefault(void)
{
    memset(&s_tcpip_cfgs[0], 0, sizeof(s_tcpip_cfgs));
    for (int i = 0; i < BOARD_TCPIP_MAX; i++) {
        s_tcpip_cfgs[i] = c_tcpip_cfg_default;
    }
}

static void prvReadTcpipCfgsFromFs(void)
{
    if (!board_cfg_read(TCPIP_CFG_NAME, &s_tcpip_cfgs[0], sizeof(s_tcpip_cfgs))) {
        prvSetTcpipCfgsDefault();
    }
}

void prvSaveTcpipCfgsToFs(void)
{
    if (!board_cfg_write(TCPIP_CFG_NAME, &s_tcpip_cfgs[0], sizeof(s_tcpip_cfgs))) {
        ; //prvSetTcpipCfgsDefault();
    }
}

rt_err_t tcpip_cfg_init(void)
{
    prvReadTcpipCfgsFromFs();
    memcpy(g_tcpip_cfgs, s_tcpip_cfgs, sizeof(s_tcpip_cfgs));
    return RT_EOK;
}

int is_tcpip_used_gprs(void)
{
    int i;
    for (int i = BOARD_ENET_TCPIP_NUM; i < BOARD_TCPIP_MAX; i++) {
        if (s_tcpip_cfgs[i].enable) return 1;
    }
    return 0;
}

// for webserver
void setTcpipCfgWithJson(cJSON *pCfg)
{
    int n = cJSON_GetInt(pCfg, "n", -1);
    int en = cJSON_GetInt(pCfg, "en", -1);
    int tt = cJSON_GetInt(pCfg, "tt", -1);
    int cs = cJSON_GetInt(pCfg, "cs", -1);
    const char *pe = cJSON_GetString(pCfg, "pe", VAR_NULL);
    int po = cJSON_GetInt(pCfg, "po", -1);
    int it = cJSON_GetInt(pCfg, "it", -1);
    int kl = cJSON_GetInt(pCfg, "kl", -1);
    int md = cJSON_GetInt(pCfg, "md", -1);

    if (n >= 0 && n < BOARD_TCPIP_MAX && md >= TCP_IP_M_NORMAL && md < TCP_IP_M_NUM) {
        tcpip_cfg_t xBak = s_tcpip_cfgs[n];
        s_tcpip_cfgs[n].mode = md;
        if (en >= 0) s_tcpip_cfgs[n].enable = (en != 0 ? VAR_TRUE : VAR_FALSE);
        if (tt >= 0) s_tcpip_cfgs[n].tcpip_type = tt;
        if (cs >= 0) s_tcpip_cfgs[n].tcpip_cs = cs;
        if (pe && strlen(pe) < sizeof(s_tcpip_cfgs[n].peer)) {
            memset(s_tcpip_cfgs[n].peer, 0, sizeof(s_tcpip_cfgs[n].peer)); strcpy(s_tcpip_cfgs[n].peer, pe);
        }
        if (po >= 0 && po <= 65535) s_tcpip_cfgs[n].port = po;
        if (it >= 0) s_tcpip_cfgs[n].interval = it;
        if (kl >= 0) s_tcpip_cfgs[n].keepalive = (kl != 0);
        
        if(TCP_IP_M_NORMAL == md) {
            cJSON *normal = cJSON_GetObjectItem(pCfg, "normal");
            if(normal) {
            
                int pt = cJSON_GetInt(normal, "pt", -1);
                int ms = cJSON_GetInt(normal, "ms", -1);
                int mad = cJSON_GetInt(normal, "mad", -1);
                
                if (pt >= 0) s_tcpip_cfgs[n].cfg.normal.proto_type = pt;
                if (ms >= 0) s_tcpip_cfgs[n].cfg.normal.proto_ms = ms;
                if (mad >= MB_ADDRESS_MIN && mad <= MB_ADDRESS_MAX) s_tcpip_cfgs[n].cfg.normal.maddress = mad;
            }
        } else if(TCP_IP_M_XFER == md) {
            cJSON *xfer = cJSON_GetObjectItem(pCfg, "xfer");
            if(xfer) {
                xfer_dst_uart_cfg *pucfg = RT_NULL;
                xfer_dst_tcpip_cfg *ptcfg = RT_NULL;
                int xfer_md = cJSON_GetInt(xfer, "md", -1);
                if (xfer_md >= XFER_M_GW && xfer_md < XFER_M_NUM) {
                    s_tcpip_cfgs[n].cfg.xfer.mode = xfer_md;
                    int xfer_pt = cJSON_GetInt( xfer, "pt", -1 );
                    int xfer_dt = cJSON_GetInt( xfer, "dt", -1 );
                    int tidx = cJSON_GetInt( xfer, "tidx", -1 );
                    int ubr = cJSON_GetInt( xfer, "ubr", -1 );
                    int udb = cJSON_GetInt( xfer, "udb", -1 );
                    int usb = cJSON_GetInt( xfer, "usb", -1 );
                    int upy = cJSON_GetInt( xfer, "upy", -1 );
                    
                    if(XFER_M_GW == xfer_md) {
                        if(xfer_pt >= 0) s_tcpip_cfgs[n].cfg.xfer.cfg.gw.proto_type = xfer_pt;
                        if(xfer_dt >= 0) s_tcpip_cfgs[n].cfg.xfer.cfg.gw.dst_type = xfer_dt;
                        if (PROTO_DEV_IS_RS(xfer_dt)) {
                            pucfg = &s_tcpip_cfgs[n].cfg.xfer.cfg.gw.dst_cfg.uart_cfg;
                        } else if (PROTO_DEV_IS_NET(xfer_dt) 
                                || PROTO_DEV_IS_GPRS(xfer_dt)) {
                            ptcfg = &s_tcpip_cfgs[n].cfg.xfer.cfg.gw.dst_cfg.tcpip_cfg;
                        }
                    } else if(XFER_M_TRT == xfer_md) {
                        if(xfer_dt >= 0) s_tcpip_cfgs[n].cfg.xfer.cfg.trt.dst_type = xfer_dt;
                        if (PROTO_DEV_IS_RS(xfer_dt)) {
                            pucfg = &s_tcpip_cfgs[n].cfg.xfer.cfg.trt.dst_cfg.uart_cfg;
                        } else if (PROTO_DEV_IS_NET(xfer_dt) 
                                || PROTO_DEV_IS_GPRS(xfer_dt)) {
                            ptcfg = &s_tcpip_cfgs[n].cfg.xfer.cfg.trt.dst_cfg.tcpip_cfg;
                        }
                    }
                    
                    if(ptcfg) {
                        if (tidx >= 0) ptcfg->idx = tidx;
                    }
                    if(pucfg) {
                        pucfg->type = g_uart_cfgs[nUartGetInstance(xfer_dt)].uart_type;
                        if( ubr >= 0 ) pucfg->cfg.baud_rate = ubr;
                        if( udb >= 0 ) pucfg->cfg.data_bits = udb;
                        if( usb >= 0 ) pucfg->cfg.stop_bits = usb;
                        if( upy >= 0 ) pucfg->cfg.parity    = upy;
                    }
                }
            }
        }

        if (memcmp(&xBak, &s_tcpip_cfgs[n], sizeof(tcpip_cfg_t)) != 0) {

            // 以太网, 阻塞方式重新配置需要重启

            // GPRS 可以即时生效

            // add by jay 2017/04/16 去除即时生效, 配置完需要重启
            prvSaveTcpipCfgsToFs();
        }
    }
}

DEF_CGI_HANDLER(setTcpipCfg)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        setTcpipCfgWithJson(pCfg);
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}

void jsonTcpipCfg(int n, cJSON *pItem)
{
    if (n >= 0 && n < BOARD_TCPIP_MAX) {
        cJSON_AddNumberToObject(pItem, "n", n);
        cJSON_AddNumberToObject(pItem, "en", s_tcpip_cfgs[n].enable);
        cJSON_AddNumberToObject(pItem, "md", s_tcpip_cfgs[n].mode);
        cJSON_AddNumberToObject(pItem, "tt", s_tcpip_cfgs[n].tcpip_type);
        cJSON_AddNumberToObject(pItem, "cs", s_tcpip_cfgs[n].tcpip_cs);
        cJSON_AddStringToObject(pItem, "pe", s_tcpip_cfgs[n].peer);
        cJSON_AddNumberToObject(pItem, "po", s_tcpip_cfgs[n].port);
        cJSON_AddNumberToObject(pItem, "it", s_tcpip_cfgs[n].interval);
        cJSON_AddNumberToObject(pItem, "kl", s_tcpip_cfgs[n].keepalive ? 1 : 0);

        cJSON *cfg = cJSON_CreateObject();
        if(TCP_IP_M_NORMAL == s_tcpip_cfgs[n].mode) {
            cJSON_AddItemToObject(pItem, "normal", cfg);
            cJSON_AddNumberToObject(cfg, "pt", s_tcpip_cfgs[n].cfg.normal.proto_type);
            cJSON_AddNumberToObject(cfg, "ms", s_tcpip_cfgs[n].cfg.normal.proto_ms);
            cJSON_AddNumberToObject(cfg, "mad", s_tcpip_cfgs[n].cfg.normal.maddress);
        } else if(TCP_IP_M_XFER == s_tcpip_cfgs[n].mode) {
            xfer_dst_uart_cfg *pucfg = RT_NULL;
            xfer_dst_tcpip_cfg *ptcfg = RT_NULL;
            cJSON_AddItemToObject(pItem, "xfer", cfg);
            cJSON_AddNumberToObject(cfg, "md", s_tcpip_cfgs[n].cfg.xfer.mode);
            if(XFER_M_GW == s_tcpip_cfgs[n].cfg.xfer.mode) {
                cJSON_AddNumberToObject(cfg, "pt", s_tcpip_cfgs[n].cfg.xfer.cfg.gw.proto_type);
                cJSON_AddNumberToObject(cfg, "dt", s_tcpip_cfgs[n].cfg.xfer.cfg.gw.dst_type);
                if (PROTO_DEV_IS_RS(s_tcpip_cfgs[n].cfg.xfer.cfg.gw.dst_type) 
                    || PROTO_DEV_IS_ZIGBEE(s_tcpip_cfgs[n].cfg.xfer.cfg.gw.dst_type)) {
                    pucfg = &s_tcpip_cfgs[n].cfg.xfer.cfg.gw.dst_cfg.uart_cfg;
                } else if (PROTO_DEV_IS_NET(s_tcpip_cfgs[n].cfg.xfer.cfg.gw.dst_type) 
                        || PROTO_DEV_IS_GPRS(s_tcpip_cfgs[n].cfg.xfer.cfg.gw.dst_type)) {
                    ptcfg = &s_tcpip_cfgs[n].cfg.xfer.cfg.gw.dst_cfg.tcpip_cfg;
                }
            } else if(XFER_M_TRT == s_tcpip_cfgs[n].cfg.xfer.mode) {
                cJSON_AddNumberToObject(cfg, "dt", s_tcpip_cfgs[n].cfg.xfer.cfg.trt.dst_type);
                if (PROTO_DEV_IS_RS(s_tcpip_cfgs[n].cfg.xfer.cfg.trt.dst_type) 
                    || PROTO_DEV_IS_ZIGBEE(s_tcpip_cfgs[n].cfg.xfer.cfg.trt.dst_type)) {
                    pucfg = &s_tcpip_cfgs[n].cfg.xfer.cfg.trt.dst_cfg.uart_cfg;
                } else if (PROTO_DEV_IS_NET(s_tcpip_cfgs[n].cfg.xfer.cfg.trt.dst_type) 
                        || PROTO_DEV_IS_GPRS(s_tcpip_cfgs[n].cfg.xfer.cfg.trt.dst_type)) {
                    ptcfg = &s_tcpip_cfgs[n].cfg.xfer.cfg.trt.dst_cfg.tcpip_cfg;
                }
            }

            if(ptcfg) {
                cJSON_AddNumberToObject(cfg, "tidx", ptcfg->idx);
            }
            if(pucfg) {
                cJSON_AddNumberToObject(cfg, "uty", pucfg->type);
                cJSON_AddNumberToObject(cfg, "ubr", pucfg->cfg.baud_rate);
                cJSON_AddNumberToObject(cfg, "udb", pucfg->cfg.data_bits);
                cJSON_AddNumberToObject(cfg, "usb", pucfg->cfg.stop_bits);
                cJSON_AddNumberToObject(cfg, "upy", pucfg->cfg.parity);
            }
        }
    }
}

DEF_CGI_HANDLER(getTcpipCfg)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    char *szRetJSON = RT_NULL;

    if (pCfg) {
        int all = cJSON_GetInt(pCfg, "all", 0);
        if (all) {
            WEBS_PRINTF("{\"ret\":0,\"list\":[");
            rt_bool_t first = RT_TRUE;
            for (int n = 0; n < BOARD_TCPIP_MAX; n++) {
                cJSON *pItem = cJSON_CreateObject();
                if(pItem) {
                    if (!first) WEBS_PRINTF(",");
                    first = RT_FALSE;
                    jsonTcpipCfg(n, pItem);
                    szRetJSON = cJSON_PrintUnformatted(pItem);
                    if(szRetJSON) {
                        WEBS_PRINTF(szRetJSON);
                        rt_free(szRetJSON);
                    }
                }
                cJSON_Delete(pItem);
            }
            WEBS_PRINTF("]}");
        } else {
            int n = cJSON_GetInt(pCfg, "n", -1);
            if (n >= 0 && n < BOARD_TCPIP_MAX) {
                cJSON *pItem = cJSON_CreateObject();
                if(pItem) {
                    cJSON_AddNumberToObject(pItem, "ret", RT_EOK);
                    jsonTcpipCfg(n, pItem);
                    szRetJSON = cJSON_PrintUnformatted(pItem);
                    if(szRetJSON) {
                        WEBS_PRINTF(szRetJSON);
                        rt_free(szRetJSON);
                    }
                }
                cJSON_Delete(pItem);
            }
        }
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    if (err != RT_EOK) {
        WEBS_PRINTF("{\"ret\":%d}", err);
    }
    WEBS_DONE(200);
}

void jsonTcpipState(int n, cJSON *pItem)
{
    if (n >= 0 && n < BOARD_TCPIP_MAX) {
        cJSON *list = cJSON_CreateArray();
        cJSON_AddNumberToObject(pItem, "n", n);
        if (list) {
            tcpip_state_t *ts = g_tcpip_states[n];
            while (ts) {
                cJSON *node = cJSON_CreateObject();
                if (node && ts->s >= 0) {
                    cJSON_AddNumberToObject(node, "st", ts->eState);
                    cJSON_AddStringToObject(node, "rip", ts->szRemIP);
                    cJSON_AddNumberToObject(node, "rpt", ts->usRemPort);
                    cJSON_AddStringToObject(node, "lip", ts->szLocIP);
                    cJSON_AddNumberToObject(node, "lpt", ts->usLocPort);
                    cJSON_AddNumberToObject(node, "tc", ts->ulConnTime);
                    cJSON_AddNumberToObject(node, "tn", (rt_millisecond_from_tick(rt_tick_get())) / 1000);
                    cJSON_AddItemToArray(list, node);
                }
                ts = ts->next;
            }
            cJSON_AddItemToObject(pItem, "list", list);
        }
    }
}

DEF_CGI_HANDLER(getTcpipState)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    char *szRetJSON = RT_NULL;

    WEBS_PRINTF("{\"ret\":0,\"states\":[");
    rt_bool_t first = RT_TRUE;
    for (int n = 0; n < BOARD_TCPIP_MAX; n++) {
        cJSON *pItem = cJSON_CreateObject();
        if(pItem) {
            if (!first) WEBS_PRINTF(",");
            first = RT_FALSE;
            jsonTcpipState(n, pItem);
            szRetJSON = cJSON_PrintUnformatted(pItem);
            if(szRetJSON) {
                WEBS_PRINTF(szRetJSON);
                rt_free(szRetJSON);
            }
        }
        cJSON_Delete(pItem);
    }
    WEBS_PRINTF("]}");
    cJSON_Delete(pCfg);

    if (err != RT_EOK) {
        WEBS_PRINTF("{\"ret\":%d}", err);
    }
    WEBS_DONE(200);
}

