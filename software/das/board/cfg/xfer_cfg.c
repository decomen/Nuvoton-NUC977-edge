
#include <board.h>
#include <stdio.h>

xfer_uart_cfg_t g_xfer_uart_cfgs[BOARD_UART_MAX];

const xfer_uart_cfg_t c_xfer_uart_cfg_default = {
    .enable = RT_FALSE,
    .count = 0,
    .addrs = {0}
};

rt_int8_t g_xfer_net_dst_uart_map[BOARD_UART_MAX] = { 0 };
rt_bool_t g_xfer_net_dst_uart_occ[BOARD_UART_MAX];
rt_bool_t g_xfer_net_dst_uart_trt[BOARD_UART_MAX];
rt_bool_t g_xfer_net_dst_uart_dtu[BOARD_UART_MAX];

static void prvSetXferDstUartSet( void )
{
    for( int n = 0; n < BOARD_UART_MAX; n++ ) {
        g_xfer_net_dst_uart_map[n] = -1;
        g_xfer_net_dst_uart_occ[n] = RT_FALSE;
        g_xfer_net_dst_uart_trt[n] = RT_FALSE;
        g_xfer_net_dst_uart_dtu[n] = RT_FALSE;
    }
    for (int n = 0; n < BOARD_TCPIP_MAX; n++) {
        if(g_tcpip_cfgs[n].enable && TCP_IP_M_XFER == g_tcpip_cfgs[n].mode) {
            rt_int8_t dt = -1;
            xfer_dst_uart_cfg *pucfg = RT_NULL;
            if(XFER_M_GW == g_tcpip_cfgs[n].cfg.xfer.mode) {
                dt = g_tcpip_cfgs[n].cfg.xfer.cfg.gw.dst_type;
                pucfg = &g_tcpip_cfgs[n].cfg.xfer.cfg.gw.dst_cfg.uart_cfg;
            } else if(XFER_M_TRT == g_tcpip_cfgs[n].cfg.xfer.mode) {
                dt = g_tcpip_cfgs[n].cfg.xfer.cfg.trt.dst_type;
                pucfg = &g_tcpip_cfgs[n].cfg.xfer.cfg.trt.dst_cfg.uart_cfg;
            }
            
            if (PROTO_DEV_IS_RS(dt)) {
                rt_int8_t instance = nUartGetInstance(dt);
                if (instance >= 0) {
                    g_xfer_net_dst_uart_map[instance] = n;
                    g_xfer_net_dst_uart_occ[instance] = RT_TRUE;

                    // 根据 g_uart_cfgs 设置 232/485
                    if(pucfg) pucfg->type = g_uart_cfgs[instance].uart_type;
                }
            } else if (PROTO_DEV_IS_ZIGBEE(dt)) {
                if((XFER_M_GW == g_tcpip_cfgs[n].cfg.xfer.mode || XFER_M_TRT == g_tcpip_cfgs[n].cfg.xfer.mode) &&
                    ZIGBEE_WORK_COORDINATOR == g_zigbee_cfg.xInfo.WorkMode) {
                    g_xfer_net_dst_uart_map[BOARD_ZGB_UART] = n;
                    g_xfer_net_dst_uart_occ[BOARD_ZGB_UART] = RT_TRUE;
                }
                if(pucfg) pucfg->type = UART_TYPE_ZGB;
            } else if (PROTO_DEV_IS_LORA(dt)) {
                if((XFER_M_GW == g_tcpip_cfgs[n].cfg.xfer.mode || XFER_M_TRT == g_tcpip_cfgs[n].cfg.xfer.mode) &&
                    LORA_WORK_CENTRAL == g_lora_cfg.work_mode) {
                    g_xfer_net_dst_uart_map[BOARD_LORA_UART] = n;
                    g_xfer_net_dst_uart_occ[BOARD_LORA_UART] = RT_TRUE;
                }
                if(pucfg) pucfg->type = BOARD_LORA_UART;
            }
        }
    }
    if(g_zigbee_cfg.xInfo.WorkMode != ZIGBEE_WORK_COORDINATOR) {
        if(ZGB_TM_TRT == g_zigbee_cfg.tmode) {
            if(g_zigbee_cfg.dst_type >= PROTO_DEV_RS1 && g_zigbee_cfg.dst_type <= PROTO_DEV_RS_MAX) {
                rt_int8_t instance = nUartGetInstance(g_zigbee_cfg.dst_type);
                g_xfer_net_dst_uart_map[instance] = -1;
                g_xfer_net_dst_uart_occ[instance] = RT_TRUE;
                g_xfer_net_dst_uart_trt[instance] = RT_TRUE;
            }
        } else if(ZGB_TM_DTU == g_zigbee_cfg.tmode) {
            for(int u = 0; u < BOARD_UART_MAX; u++) {
                if(g_xfer_uart_cfgs[u].enable && g_xfer_uart_cfgs[u].count > 0) {
                    if(u != BOARD_ZGB_UART) {
                        g_xfer_net_dst_uart_map[u] = -1;
                        g_xfer_net_dst_uart_occ[u] = RT_TRUE;
                        g_xfer_net_dst_uart_dtu[u] = RT_TRUE;
                    }
                }
            }
        }
    }
}

static void prvSetXferUartCfgsDefault( void )
{
    memset( &g_xfer_uart_cfgs[0], 0, sizeof(g_xfer_uart_cfgs) );
    for( int i = 0; i < BOARD_UART_MAX; i++ ) {
        g_xfer_uart_cfgs[i] = c_xfer_uart_cfg_default;
    }
    prvSetXferDstUartSet();
}

static void prvReadXferUartCfgsFromFs( void )
{
    if( !board_cfg_read( XFER_UART_CFG_NAME, &g_xfer_uart_cfgs[0], sizeof(g_xfer_uart_cfgs) ) ) {
        prvSetXferUartCfgsDefault();
    }
    prvSetXferDstUartSet();
}

static void prvSaveXferUartCfgsToFs( void )
{
    if( !board_cfg_write( XFER_UART_CFG_NAME, &g_xfer_uart_cfgs[0], sizeof(g_xfer_uart_cfgs) ) ) {
        ; //prvSetTcpipCfgsDefault();
    }
    prvSetXferDstUartSet();
}

rt_err_t xfer_uart_cfg_init( void )
{
    prvReadXferUartCfgsFromFs();
    return RT_EOK;
}

int xfer_get_uart_with_slave_addr(rt_uint8_t slave_addr)
{
    for( int n = 0; n < BOARD_UART_MAX; n++ ) {
        if(g_xfer_net_dst_uart_dtu[n]) {
            for(int i = 0; i < g_xfer_uart_cfgs[n].count; i++) {
                if(slave_addr == g_xfer_uart_cfgs[n].addrs[i]) {
                    return n;
                }
            }
        }
    }

    return -1;
}

// for webserver

rt_bool_t setXferUartCfgWithJson(cJSON *pXferUCfg, rt_bool_t save_flag)
{
    rt_bool_t change_flag = RT_FALSE;
    int num = cJSON_GetInt( pXferUCfg, "n", -1 );
    int en = cJSON_GetInt( pXferUCfg, "en", -1 );
    int cnt = cJSON_GetInt( pXferUCfg, "cnt", -1 );
    const char *addrs = cJSON_GetString( pXferUCfg, "addrs", RT_NULL );
    if(num >= 0) {
        int instance = nUartGetInstance(num);
        xfer_uart_cfg_t xbak = g_xfer_uart_cfgs[instance];
        if(en >= 0) g_xfer_uart_cfgs[instance].enable = (en!=0);
        if(cnt >= 0) g_xfer_uart_cfgs[instance].count = cnt;
        if(addrs && strlen(addrs)>0) {
            char *_addrs = rt_strdup(addrs);
            if(_addrs) {
                const char * split = ","; 
                char * p = strtok(_addrs, split);
                int index = 0;
                while(p && index < XFER_UART_ADDRS_NUM) {
                    g_xfer_uart_cfgs[instance].addrs[index++] = atoi(p);
                    p = strtok(NULL, split);
                }
                rt_free(_addrs);
            }
        }
        if( memcmp( &xbak, &g_xfer_uart_cfgs[instance], sizeof(xfer_uart_cfg_t)) != 0 ) {
            change_flag = RT_TRUE;
            if(save_flag) {
                prvSaveXferUartCfgsToFs();
            }
        }
    }

    return change_flag;
}

DEF_CGI_HANDLER(setXferUartCfg)
{
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse( szJSON ) : RT_NULL;
    if( pCfg ) {
        cJSON *pAry = cJSON_GetObjectItem( pCfg, "cfg" );
        if( pAry ) {
            int n = cJSON_GetArraySize( pAry );
            rt_bool_t save_flag = RT_FALSE;
            for( int i = 0; i < n && i < BOARD_UART_MAX; i++ ) {
                cJSON *pXferUCfg = cJSON_GetArrayItem( pAry, i );
                if(pXferUCfg) {
                    if(setXferUartCfgWithJson(pXferUCfg, RT_FALSE)) {
                        save_flag = RT_TRUE;
                    }
                }
            }
            if(save_flag) {
                prvSaveXferUartCfgsToFs();
            }
        }
    }

    cJSON_Delete(pCfg);
    WEBS_PRINTF( "{\"ret\":0}" );
	WEBS_DONE(200);
}

extern void xfer_json_zgb_mnode(cJSON *pItem);

void jsonXferUartCfg( int n, cJSON *pItem )
{
    int instance = nUartGetInstance(n);
    if(n != PROTO_DEV_ZIGBEE && instance >= 0) {
        cJSON_AddNumberToObject(pItem, "n", n);
        cJSON_AddNumberToObject(pItem, "en", g_xfer_uart_cfgs[instance].enable);
        cJSON_AddNumberToObject(pItem, "cnt", g_xfer_uart_cfgs[instance].count);
        if(g_xfer_uart_cfgs[instance].count > 0) {
            char *szlst = rt_calloc(1, g_xfer_uart_cfgs[instance].count * 4 + 12);
            if(szlst) {
                rt_bool_t first = RT_TRUE;
                for(int i = 0; i < g_xfer_uart_cfgs[instance].count; i++) {
                    char str[5] = {0};
                    if (!first) strcat(szlst, ",");
                    first = RT_FALSE;
                    sprintf(str, "%u", g_xfer_uart_cfgs[instance].addrs[i]);
                    strcat(szlst, str);
                }
                cJSON_AddStringToObject(pItem, "addrs", szlst);
                rt_free(szlst);
            }
        } else {
            cJSON_AddStringToObject(pItem, "addrs", "");
        }
    } else if(PROTO_DEV_ZIGBEE == n) { 
        xfer_json_zgb_mnode(pItem);
    }
}

DEF_CGI_HANDLER(getXferUartCfg)
{
    rt_bool_t first = RT_TRUE;
    char *szRetJSON = RT_NULL;
    WEBS_PRINTF( "{\"ret\":0,\"cfg\":[" );
    for( int n = 0; n <= PROTO_DEV_ZIGBEE; n++ ) {
        if (n < BOARD_UART_MAX || n == PROTO_DEV_ZIGBEE) {
            cJSON *pItem = cJSON_CreateObject();
            if(pItem) {
                if( !first ) WEBS_PRINTF( "," );
                first = RT_FALSE;
                jsonXferUartCfg( n, pItem );
                szRetJSON = cJSON_PrintUnformatted( pItem );
                if(szRetJSON) {
                    WEBS_PRINTF( szRetJSON );
                    rt_free( szRetJSON );
                }
            }
            cJSON_Delete( pItem );
        }
    }
    WEBS_PRINTF( "]}" );
    WEBS_DONE(200);
}

