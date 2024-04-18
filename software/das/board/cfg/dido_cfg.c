
#include <board.h>

di_cfg_t g_di_cfgs[DI_CHAN_NUM];
do_cfg_t g_do_cfgs[DO_CHAN_NUM];

static void prvSetDiCfgDefault( void )
{
    memset( &g_di_cfgs[0], 0, sizeof(g_di_cfgs) );
}

static void prvSetDoCfgDefault( void )
{
    memset( &g_do_cfgs[0], 0, sizeof(g_do_cfgs) );
}

static void prvReadDiCfgsFromFs( void )
{
    if( !board_cfg_read( DI_CFG_NAME, &g_di_cfgs[0], sizeof(g_di_cfgs) ) ) {
        prvSetDiCfgDefault();
    }
}

static void prvSaveDiCfgsToFs( void )
{
    if( !board_cfg_write( DI_CFG_NAME, &g_di_cfgs[0], sizeof(g_di_cfgs) ) ) {
        ;//prvSetDiCfgDefault();
    }
}

static void prvReadDoCfgsFromFs( void )
{
    if( !board_cfg_read( DO_CFG_NAME, &g_do_cfgs[0], sizeof(g_do_cfgs) ) ) {
        prvSetDoCfgDefault();
    }
}

static void prvSaveDoCfgsToFs( void )
{
    if( !board_cfg_write( DO_CFG_NAME, &g_do_cfgs[0], sizeof(g_do_cfgs) ) ) {
        ;//prvSetDoCfgDefault();
    }
}

rt_err_t di_cfgs_init( void )
{
    prvReadDiCfgsFromFs();
    return RT_EOK;
}

rt_err_t do_cfgs_init( void )
{
    prvReadDoCfgsFromFs();
    return RT_EOK;
}


// for webserver

void jsonDiCfg( int n, cJSON *pItem )
{
    if( n >= 0 && n < DI_CHAN_NUM ) {
        cJSON_AddNumberToObject(pItem, "n", n);
        cJSON_AddNumberToObject( pItem, "en", g_di_cfgs[n].enable?1:0);
        cJSON_AddNumberToObject( pItem, "in", g_di_cfgs[n].interval );
        cJSON_AddStringToObject( pItem, "exp", g_di_cfgs[n].exp );
    }
}

void jsonDoCfg( int n, cJSON *pItem )
{
    if( n >= 0 && n < DO_CHAN_NUM ) {
        cJSON_AddNumberToObject( pItem, "n", n);
        cJSON_AddStringToObject( pItem, "exp", g_do_cfgs[n].exp );
    }
}

DEF_CGI_HANDLER(getDiCfg)
{
    rt_err_t err = RT_EOK;
    char* szRetJSON = RT_NULL;
    
    WEBS_PRINTF( "{\"ret\":0,\"cfg\":[" );
    
    rt_bool_t first = RT_TRUE;
    for( int n = 0; n < DI_CHAN_NUM; n++ ) {
        cJSON *pItem = cJSON_CreateObject();
        if(pItem) {
            if( !first ) WEBS_PRINTF( "," );
            first = RT_FALSE;
            jsonDiCfg( n, pItem );
            szRetJSON = cJSON_PrintUnformatted( pItem );
            if(szRetJSON) {
                WEBS_PRINTF( szRetJSON );
                rt_free( szRetJSON );
            }
        }
        cJSON_Delete( pItem );
    }
    WEBS_PRINTF( "]}" );

    if( err != RT_EOK ) {
	    WEBS_PRINTF( "{\"ret\":%d}", err);
	}
    WEBS_DONE(200);
}

DEF_CGI_HANDLER(getDoCfg)
{
    rt_err_t err = RT_EOK;
    char* szRetJSON = RT_NULL;
    
    WEBS_PRINTF( "{\"ret\":0,\"cfg\":[" );
    
    rt_bool_t first = RT_TRUE;
    for( int n = 0; n < DO_CHAN_NUM; n++ ) {
        cJSON *pItem = cJSON_CreateObject();
        if(pItem) {
            if( !first ) WEBS_PRINTF( "," );
            first = RT_FALSE;
            jsonDoCfg( n, pItem );
            szRetJSON = cJSON_PrintUnformatted( pItem );
            if(szRetJSON) {
                WEBS_PRINTF( szRetJSON );
                rt_free( szRetJSON );
            }
        }
        cJSON_Delete( pItem );
    }
    WEBS_PRINTF( "]}" );

    if( err != RT_EOK ) {
	    WEBS_PRINTF( "{\"ret\":%d}", err);
	}
    WEBS_DONE(200);
}

void setDiCfgWithJson(cJSON *pDiCfg, rt_bool_t save_flag)
{
    int n = cJSON_GetInt( pDiCfg, "n", -1 );
    int en = cJSON_GetInt( pDiCfg, "en", -1 );
    int in = cJSON_GetInt( pDiCfg, "in", -1 );
    const char *exp = cJSON_GetString( pDiCfg, "exp", RT_NULL );
    if( n >= 0 && n < DI_CHAN_NUM ) {
        if(en >= 0) g_di_cfgs[n].enable = (en!=0);
        if(in >= 0) g_di_cfgs[n].interval = in;
        if(exp && strlen(exp) < DI_EXP_LEN) { memset( g_di_cfgs[n].exp, 0, DI_EXP_LEN); strcpy( g_di_cfgs[n].exp, exp ); }
    }
    if(save_flag) {
        prvSaveDiCfgsToFs();
    }
}

DEF_CGI_HANDLER(setDiCfg)
{
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse( szJSON ) : RT_NULL;
    if( pCfg ) {
        cJSON *pAry = cJSON_GetObjectItem( pCfg, "cfg" );
        if( pAry ) {
            int n = cJSON_GetArraySize( pAry );
            for( int i = 0; i < n && i <= DI_CHAN_NUM; i++ ) {
                cJSON *pDiCfg = cJSON_GetArrayItem( pAry, i );
                if(pDiCfg) {
                    setDiCfgWithJson(pDiCfg, RT_FALSE);
                }
            }
            prvSaveDiCfgsToFs();
        }
    }

    cJSON_Delete(pCfg);
    WEBS_PRINTF( "{\"ret\":0}" );
	WEBS_DONE(200);
}

void setDoCfgWithJson(cJSON *pDoCfg, rt_bool_t save_flag)
{
    int n = cJSON_GetInt( pDoCfg, "n", -1 );
    const char *exp = cJSON_GetString( pDoCfg, "exp", RT_NULL );
    if( n >= 0 && n < DO_CHAN_NUM ) {
        if(exp && strlen(exp) < DO_EXP_LEN) { memset( g_do_cfgs[n].exp, 0, DO_EXP_LEN); strcpy( g_do_cfgs[n].exp, exp ); }
    }
    if(save_flag) {
        prvSaveDoCfgsToFs();
    }
}

DEF_CGI_HANDLER(setDoCfg)
{
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse( szJSON ) : RT_NULL;
    if( pCfg ) {
        cJSON *pAry = cJSON_GetObjectItem( pCfg, "cfg" );
        if( pAry ) {
            int n = cJSON_GetArraySize( pAry );
            for( int i = 0; i < n && i <= DI_CHAN_NUM; i++ ) {
                cJSON *pDoCfg = cJSON_GetArrayItem( pAry, i );
                if(pDoCfg) {
                    setDoCfgWithJson(pDoCfg, RT_FALSE);
                }
            }
            prvSaveDoCfgsToFs();
        }
    }

    cJSON_Delete(pCfg);
    WEBS_PRINTF( "{\"ret\":0}" );
	WEBS_DONE(200);
}

