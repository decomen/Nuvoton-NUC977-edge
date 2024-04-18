
#include <board.h>

const host_cfg_t c_host_default_cfg = {
    .szHostName = HOST_NAME_HEAD,
    .szId       = "",
    .bSyncTime  = RT_TRUE,
    .nTimezone  = 8*60*60,      //默认8时区
    .szNTP[0]     = "",
    .szNTP[1]     = "",
    .bDebug     = RT_TRUE
};

host_cfg_t g_host_cfg;

static void prvSetHostCfgDefault( void )
{
    g_host_cfg = c_host_default_cfg;
    rt_sprintf( g_host_cfg.szHostName, "%s_%s",
        g_host_cfg.szHostName,
        g_sys_info.SN
    );
    memcpy(g_host_cfg.szId, g_sys_info.DEV_ID, DAS_DEV_ID_LEN);
}

static void prvReadHostCfgFromFs( void )
{
    if( !board_cfg_read( HOST_CFG_NAME, &g_host_cfg, sizeof(g_host_cfg)) ) {
        prvSetHostCfgDefault();
    }
    if (g_host_cfg.szId[0] == '\0') {
        memset(g_host_cfg.szId, 0, sizeof(g_host_cfg.szId));
        memcpy(g_host_cfg.szId, g_sys_info.DEV_ID, DAS_DEV_ID_LEN);
    }
    g_host_cfg.bDebug = RT_TRUE;
}

static void prvSaveHostCfgToFs( void )
{
    if( !board_cfg_write( HOST_CFG_NAME, &g_host_cfg, sizeof(g_host_cfg)) ) {
        ;//prvSetAuthCfgDefault();
    }
    if (g_host_cfg.szId[0] == '\0') {
        memset(g_host_cfg.szId, 0, sizeof(g_host_cfg.szId));
        memcpy(g_host_cfg.szId, g_sys_info.DEV_ID, DAS_DEV_ID_LEN);
    }
}

rt_err_t host_cfg_init( void )
{
    prvReadHostCfgFromFs();

    return RT_EOK;
}

static void jsonHostCfg(int n, cJSON *pItem)
{
    cJSON_AddStringToObject( pItem, "host", g_host_cfg.szHostName );
    cJSON_AddStringToObject( pItem, "id", g_host_cfg.szId );
    cJSON_AddNumberToObject( pItem, "tz", g_host_cfg.nTimezone );
    cJSON_AddNumberToObject( pItem, "sync", g_host_cfg.bSyncTime?1:0 );
    cJSON_AddStringToObject( pItem, "ntp1", g_host_cfg.szNTP[0] );
    cJSON_AddStringToObject( pItem, "ntp2", g_host_cfg.szNTP[1] );
    cJSON_AddNumberToObject( pItem, "dbg", g_host_cfg.bDebug?1:0);
}

void __jsonHostCfg(int n, cJSON *pItem)
{
  //  cJSON_AddStringToObject( pItem, "host", g_host_cfg.szHostName );
   // cJSON_AddStringToObject( pItem, "id", g_host_cfg.szId );
    cJSON_AddNumberToObject( pItem, "tz", g_host_cfg.nTimezone );
    cJSON_AddNumberToObject( pItem, "sync", g_host_cfg.bSyncTime?1:0 );
    cJSON_AddStringToObject( pItem, "ntp1", g_host_cfg.szNTP[0] );
    cJSON_AddStringToObject( pItem, "ntp2", g_host_cfg.szNTP[1] );
    cJSON_AddNumberToObject( pItem, "dbg", g_host_cfg.bDebug?1:0);
}

// for webserver
DEF_CGI_HANDLER(getHostCfg)
{
    char* szRetJSON = RT_NULL;
    cJSON *pItem = cJSON_CreateObject();
    if(pItem) {
        cJSON_AddNumberToObject( pItem, "ret", RT_EOK );
        jsonHostCfg(0, pItem);
        szRetJSON = cJSON_PrintUnformatted( pItem );
        if(szRetJSON) {
            WEBS_PRINTF( szRetJSON );
            rt_free( szRetJSON );
        }
    }
    cJSON_Delete( pItem );

	WEBS_DONE(200);
}

void setHostCfgWithJson(cJSON *pCfg)
{
    const char *host = cJSON_GetString( pCfg, "host", VAR_NULL );
    const char *id = cJSON_GetString( pCfg, "id", "" );
    int sync = cJSON_GetInt( pCfg, "sync", -1 );
    int tz = cJSON_GetInt( pCfg, "tz", 24*60*60 );
    const char *ntp1 = cJSON_GetString( pCfg, "ntp1", VAR_NULL );
    const char *ntp2 = cJSON_GetString( pCfg, "ntp2", VAR_NULL );
    int dbg = cJSON_GetInt( pCfg, "dbg", -1 );

    host_cfg_t cfg_bak = g_host_cfg;
    if( host && strlen(host) < sizeof(g_host_cfg.szHostName) ) { memset( g_host_cfg.szHostName, 0, sizeof(g_host_cfg.szHostName) ); strcpy( g_host_cfg.szHostName, host); }
    if( id && strlen(id) < sizeof(g_host_cfg.szId) ) { memset( g_host_cfg.szId, 0, sizeof(g_host_cfg.szId) ); strcpy( g_host_cfg.szId, id); }
    if( sync >= 0 ) g_host_cfg.bSyncTime = (sync!=0);
    if( tz > -24*60*60 && tz < 24*60*60 ) g_host_cfg.nTimezone = tz;
    if( ntp1 && strlen(ntp1) < sizeof(g_host_cfg.szNTP[0]) ) { memset( g_host_cfg.szNTP[0], 0, sizeof(g_host_cfg.szNTP[0]) ); strcpy( g_host_cfg.szNTP[0], ntp1); }
    if( ntp2 && strlen(ntp2) < sizeof(g_host_cfg.szNTP[1]) ) { memset( g_host_cfg.szNTP[1], 0, sizeof(g_host_cfg.szNTP[1]) ); strcpy( g_host_cfg.szNTP[1], ntp2); }
    if( dbg >= 0 ) g_host_cfg.bDebug = (dbg!=0);

    if( memcmp( &cfg_bak, &g_host_cfg, sizeof(g_host_cfg) ) != 0 ) {
        prvSaveHostCfgToFs();
    }
}

DEF_CGI_HANDLER(setHostCfg)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse( szJSON ) : RT_NULL;
    if( pCfg ) {
        setHostCfgWithJson(pCfg);
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

	WEBS_PRINTF( "{\"ret\":%d}", err);
	WEBS_DONE(200);
}

