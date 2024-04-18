
#include <board.h>

analog_cfg_t g_analog_cfgs[ADC_CHANNEL_NUM];

static void prvDefalutAnalogCfgs( void )
{
    memset( &g_analog_cfgs[0], 0, sizeof(g_analog_cfgs) );
    for( int i = 0; i < ADC_CHANNEL_NUM; i++ ) {
        g_analog_cfgs[i].enable = RT_FALSE;
        g_analog_cfgs[i].range = Range_4_20MA;
        g_analog_cfgs[i].unit = Unit_Eng;
    }
}

static void prvReadAnalogCfgsFromFs( void )
{
    if( !board_cfg_read( ANALOG_CFG_NAME, &g_analog_cfgs[0], sizeof(g_analog_cfgs)) ) {
        prvDefalutAnalogCfgs();
    }
}

static void prvSaveAnalogCfgsToFs( void )
{
    if( !board_cfg_write( ANALOG_CFG_NAME, &g_analog_cfgs[0], sizeof(g_analog_cfgs)) ) {
        ;
    }
}

rt_err_t analog_cfgs_init( void )
{
    prvReadAnalogCfgsFromFs();
    return RT_EOK;
}


// for webserver
extern var_double_t g_dAIStorage_xx[ADC_CHANNEL_NUM];
extern int g_nAIStorageCnt[ADC_CHANNEL_NUM];
extern rt_tick_t g_dAI_last_storage_tick[ADC_CHANNEL_NUM];

extern var_double_t g_dAIHourStorage_xx[ADC_CHANNEL_NUM];
extern int g_nAIHourStorageCnt[ADC_CHANNEL_NUM];
extern rt_tick_t g_dAIHour_last_storage_tick[ADC_CHANNEL_NUM];

void setAnalogCfgWithJson(cJSON *pCfg)
{
    int n = cJSON_GetInt( pCfg, "n", -1 );
    
    if( n >= ADC_CHANNEL_0 && n < ADC_CHANNEL_NUM ) {
        int en = cJSON_GetInt( pCfg, "en", -1 );
        int it = cJSON_GetInt( pCfg, "it", -1 );
        int rg = cJSON_GetInt( pCfg, "rg", -1 );
        int ut = cJSON_GetInt( pCfg, "ut", -1 );
        float ei = (float)cJSON_GetDouble( pCfg, "ei", g_analog_cfgs[n].ext_range_min );
        float ea = (float)cJSON_GetDouble( pCfg, "ea", g_analog_cfgs[n].ext_range_max );
        float ec = (float)cJSON_GetDouble( pCfg, "ec", g_analog_cfgs[n].ext_corr.factor );
    
        analog_cfg_t xBak = g_analog_cfgs[n];
        
        if( en >= 0 ) g_analog_cfgs[n].enable = (en!=0);
        if( it >= 0 ) g_analog_cfgs[n].interval = rg;
        if( rg >= 0 ) g_analog_cfgs[n].range = rg;
        if( ut >= 0 ) g_analog_cfgs[n].unit = ut;
        g_analog_cfgs[n].ext_range_min = ei;
        g_analog_cfgs[n].ext_range_max = ea;
        g_analog_cfgs[n].ext_corr.factor = ec;

        if( memcmp( &xBak, &g_analog_cfgs[n], sizeof(analog_cfg_t)) != 0 ) {
            g_dAI_last_storage_tick[n] = rt_tick_get();
            g_nAIStorageCnt[n] = 0;
            g_dAIStorage_xx[n] = 0;

            g_dAIHour_last_storage_tick[n] = rt_tick_get();
            g_nAIHourStorageCnt[n] = 0;
            g_dAIHourStorage_xx[n] = 0;
            prvSaveAnalogCfgsToFs();
        }
    }
}

DEF_CGI_HANDLER(setAnalogCfg)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse( szJSON ) : RT_NULL;
    if( pCfg ) {
        setAnalogCfgWithJson(pCfg);
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);
    
	WEBS_PRINTF( "{\"ret\":%d}", err);
    WEBS_DONE(200);
}

extern AIResult_t g_AIEngUnitResult;
extern AIResult_t g_AIMeasResult;
void jsonAnalogCfg( int n, cJSON *pItem )
{
    if( n >= ADC_CHANNEL_0 && n < ADC_CHANNEL_NUM ) {
        cJSON_AddNumberToObject( pItem, "n", n);
        cJSON_AddNumberToObject( pItem, "en", g_analog_cfgs[n].enable?1:0);
        cJSON_AddNumberToObject( pItem, "it", g_analog_cfgs[n].interval );
        cJSON_AddNumberToObject( pItem, "rg", g_analog_cfgs[n].range );
        cJSON_AddNumberToObject( pItem, "ut", g_analog_cfgs[n].unit );
        cJSON_AddNumberToObject( pItem, "ei", g_analog_cfgs[n].ext_range_min );
        cJSON_AddNumberToObject( pItem, "ea", g_analog_cfgs[n].ext_range_max );
        cJSON_AddNumberToObject( pItem, "ec", g_analog_cfgs[n].ext_corr.factor );
        cJSON_AddNumberToObject( pItem, "mv", g_AIMeasResult.fAI_xx[n] );
        cJSON_AddNumberToObject( pItem, "eg", g_AIEngUnitResult.fAI_xx[n] );
    }
}

DEF_CGI_HANDLER(getAnalogCfg)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg"); 
    cJSON *pCfg = szJSON ? cJSON_Parse( szJSON ) : RT_NULL;
    char* szRetJSON = RT_NULL;
    
    if( pCfg ) {
        int all = cJSON_GetInt( pCfg, "all", 0 );
        if( all ) {
            WEBS_PRINTF( "{\"ret\":0,\"list\":[" );
            
            rt_bool_t first = RT_TRUE;
            for( int n = ADC_CHANNEL_0; n < ADC_CHANNEL_NUM; n++ ) {
                cJSON *pItem = cJSON_CreateObject();
                if(pItem) {
                    if( !first ) WEBS_PRINTF( "," );
                    first = RT_FALSE;
                    jsonAnalogCfg( n, pItem );
                    szRetJSON = cJSON_PrintUnformatted( pItem );
                    if(szRetJSON) {
                        WEBS_PRINTF( szRetJSON );
                        rt_free( szRetJSON );
                    }
                }
                cJSON_Delete( pItem );
            }
            WEBS_PRINTF( "]}" );
        } else {
            int n = cJSON_GetInt( pCfg, "n", -1 );
            if( n >= ADC_CHANNEL_0 && n < ADC_CHANNEL_NUM ) {
                cJSON *pItem = cJSON_CreateObject();
                if(pItem) {
                    cJSON_AddNumberToObject( pItem, "ret", RT_EOK );
                    jsonAnalogCfg( n, pItem );
                    szRetJSON = cJSON_PrintUnformatted( pItem );
                    if(szRetJSON) {
                        WEBS_PRINTF( szRetJSON );
                        rt_free( szRetJSON );
                    }
                }
                cJSON_Delete( pItem );
            }
        }
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    if( err != RT_EOK ) {
	    WEBS_PRINTF( "{\"ret\":%d}", err);
	}
    WEBS_DONE(200);
}


