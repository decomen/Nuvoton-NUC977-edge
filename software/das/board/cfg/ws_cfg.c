
#include <board.h>

#define _WS_CFG_DEFAULT     { .enable = RT_FALSE, .port_type = WS_PORT_RS1 }

const ws_cfg_t c_ws_cfg = _WS_CFG_DEFAULT;

ws_cfg_t g_ws_cfg = _WS_CFG_DEFAULT;

rt_err_t ws_cfg_init( void )
{
    g_ws_cfg = c_ws_cfg;
    return RT_EOK;
}

// for webserver
DEF_CGI_HANDLER(setWebsocketCfg)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg"); 
    cJSON *pCfg = szJSON ? cJSON_Parse( szJSON ) : RT_NULL;
    if( pCfg ) {
        int en = cJSON_GetInt( pCfg, "en", -1 );
        int pt = cJSON_GetInt( pCfg, "pt", -1 );
        int lsnpt = cJSON_GetInt( pCfg, "lsnpt", -1 );
        int lsnidx = cJSON_GetInt( pCfg, "lsnidx", -1 );
        
        if( en >= 0 ) g_ws_cfg.enable = (en>0);
        if( pt >= 0 && pt <= WS_PORT_LORA ) g_ws_cfg.port_type = pt;
        if( lsnpt >= 1 && lsnpt <= 0xFFFF ) g_ws_cfg.listen_port = lsnpt;
        if( lsnidx >= 1 ) g_ws_cfg.listen_index = lsnidx - 1;

        if( g_host_cfg.bDebug && WS_PORT_SHELL == g_ws_cfg.port_type ) {
            if (ws_console_open() == -1) {
                err = RT_ERROR;
            }
        }
        cJSON_Delete(pCfg);
    } else {
        err = RT_ERROR;
    }

    ws_vm_clear();
    
    WEBS_PRINTF( "{\"ret\":%d}", err);
    WEBS_DONE(200);
}

