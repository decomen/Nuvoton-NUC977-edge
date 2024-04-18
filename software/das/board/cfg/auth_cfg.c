
#include <board.h>

#define _AUTH_DEFAULT       { "admin", "admin", RT_FALSE, RT_FALSE, 0, 0 }

const auth_cfg_t c_auth_default_cfg = _AUTH_DEFAULT;

auth_cfg_t g_auth_cfg = _AUTH_DEFAULT;

static void prvSetAuthCfgDefault( void )
{
    g_auth_cfg = c_auth_default_cfg;
}

static void prvReadAuthCfgFromFs( void )
{
    if( !board_cfg_read( AUTH_CFG_NAME, &g_auth_cfg, sizeof(g_auth_cfg)) ) {
        prvSetAuthCfgDefault();
    }
}

static void prvSaveAuthCfgToFs( void )
{
    if( !board_cfg_write( AUTH_CFG_NAME, &g_auth_cfg, sizeof(g_auth_cfg)) ) {
        ;//prvSetAuthCfgDefault();
    }
}

rt_err_t auth_cfg_init( void )
{
    prvReadAuthCfgFromFs();
    
    return RT_EOK;
}

void jsonAuthCfg(int n, cJSON *pItem)
{
    cJSON_AddStringToObject( pItem, "user", g_auth_cfg.szUser );
    cJSON_AddStringToObject( pItem, "psk", g_auth_cfg.szPsk );
    cJSON_AddNumberToObject( pItem, "ssh", g_auth_cfg.bUseSSH?1:0 );
    cJSON_AddNumberToObject( pItem, "sshcer", g_auth_cfg.bUseSSHCer?1:0 );
    cJSON_AddNumberToObject( pItem, "sp", g_auth_cfg.nSSHPort );
    cJSON_AddNumberToObject( pItem, "t", g_auth_cfg.nTime );
}

// for webserver
DEF_CGI_HANDLER(getAuthCfg)
{
    char* szRetJSON = RT_NULL;
    cJSON *pItem = cJSON_CreateObject();
    if(pItem) {
        cJSON_AddNumberToObject( pItem, "ret", RT_EOK );
        jsonAuthCfg(0, pItem);
        szRetJSON = cJSON_PrintUnformatted( pItem );
        if(szRetJSON) {
            WEBS_PRINTF( szRetJSON );
            rt_free( szRetJSON );
        }
    }
    cJSON_Delete( pItem );

	WEBS_DONE(200);
}

void setAuthCfgWithJson(cJSON *pCfg)
{
    const char *user = cJSON_GetString( pCfg, "user", VAR_NULL );
    const char *psk = cJSON_GetString( pCfg, "psk", VAR_NULL );
    int ssh = cJSON_GetInt( pCfg, "ssh", -1 );
    int sshcer = cJSON_GetInt( pCfg, "sshcer", -1 );
    int sp = cJSON_GetInt( pCfg, "sp", -1 );
    int t = cJSON_GetInt( pCfg, "t", -1 );

    auth_cfg_t cfg_bak = g_auth_cfg;
    if( user && strlen(user) < sizeof(g_auth_cfg.szUser) ) { memset( g_auth_cfg.szUser, 0, sizeof(g_auth_cfg.szUser) ); strcpy( g_auth_cfg.szUser, user); }
    if( psk && strlen(psk) < sizeof(g_auth_cfg.szPsk) ) { memset( g_auth_cfg.szPsk, 0, sizeof(g_auth_cfg.szPsk) ); strcpy( g_auth_cfg.szPsk, psk ); }
    if( ssh >= 0 ) g_auth_cfg.bUseSSH = (ssh!=0);
    if( sshcer >= 0 ) g_auth_cfg.bUseSSHCer = (sshcer!=0);
    if( sp >= 0 ) g_auth_cfg.nSSHPort = sp;
    if( t >= 0 ) g_auth_cfg.nTime = t;
    
    if( memcmp( &cfg_bak, &g_auth_cfg, sizeof(g_auth_cfg) ) != 0 ) {
        webnet_auth_set( g_auth_cfg.szUser, g_auth_cfg.szPsk );
        prvSaveAuthCfgToFs();
    }
}

DEF_CGI_HANDLER(setAuthCfg)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg"); 
    cJSON *pCfg = szJSON ? cJSON_Parse( szJSON ) : RT_NULL;
    if( pCfg ) {
        setAuthCfgWithJson(pCfg);
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);
    
	WEBS_PRINTF( "{\"ret\":%d}", err);
	WEBS_DONE(200);
}

