#include <board.h>
#include <in_out.h>

void vInOutInit(void)
{
}

void vRelaysOutputConfig(eRelays_OutPut_Chanel_t chan, eInOut_stat_t sta)
{
    if(sta == PIN_RESET) {
        das_do_set_do_state(DAS_DIDO_TYPE_RELAY, chan, 0);
    } else {
        das_do_set_do_state(DAS_DIDO_TYPE_RELAY, chan, 1);
    }
}

void vRelaysOutputGet(eRelays_OutPut_Chanel_t chan, eInOut_stat_t *sta)
{
    int state = das_do_get_do_state(DAS_DIDO_TYPE_RELAY, chan);
    *sta = (state == 0 ? PIN_RESET : PIN_SET);
}

void vRelaysOutputToggle(eRelays_OutPut_Chanel_t chan)
{
    eInOut_stat_t xStat;
    vRelaysOutputGet( chan , &xStat) ;
    if (xStat == PIN_SET) {
        vRelaysOutputConfig(chan, PIN_RESET);
    } else {
        vRelaysOutputConfig(chan, PIN_SET);
    }
}

void vTTLOutputConfig(eTTL_Output_Chanel_t chan, eInOut_stat_t sta)
{
    if (sta == PIN_RESET) {
        das_do_set_do_state(DAS_DIDO_TYPE_TTL, chan, 0);
    } else {
        das_do_set_do_state(DAS_DIDO_TYPE_TTL, chan, 1);
    }
}

void vTTLOutputGet(eTTL_Output_Chanel_t chan, eInOut_stat_t *sta)
{
    int state = das_do_get_do_state(DAS_DIDO_TYPE_TTL, chan);
    *sta = (state == 0 ? PIN_RESET : PIN_SET);
}

void vTTLOutputToggle(eTTL_Output_Chanel_t chan)
{
    eInOut_stat_t xStat;
    vTTLOutputGet(chan , &xStat) ;
    if (xStat == PIN_SET){
        vTTLOutputConfig(chan, PIN_RESET);
    } else {
        vTTLOutputConfig(chan, PIN_SET);
    }
}

void vTTLInputputGet(eTTL_Input_Chanel_t chan, eInOut_stat_t *sta)
{
    int state = das_do_get_di_state(chan);
    *sta = ((state <= 0) ? PIN_RESET : PIN_SET);
}

DEF_CGI_HANDLER(getDiValue)
{
    rt_bool_t first = RT_TRUE;

    first = RT_TRUE;
    WEBS_PRINTF( "{\"ret\":0,\"di\":[" );
    for( int i = 0; i < 8; i++ ) {
        cJSON *pItem = cJSON_CreateObject();
        if(pItem) {
            if( !first ) WEBS_PRINTF( "," );
            first = RT_FALSE;
            cJSON_AddNumberToObject( pItem, "va", (g_xDIResultReg.xDIResult.usDI_xx[i] == (PIN_RESET ? 0 : 1)) );
            char *szRetJSON = cJSON_PrintUnformatted( pItem );
            if(szRetJSON) {
                WEBS_PRINTF( szRetJSON );
                rt_free( szRetJSON );
            }
        }
        cJSON_Delete( pItem );
    }
    WEBS_PRINTF( "]}" );

	WEBS_DONE(200);
}

DEF_CGI_HANDLER(setDoValue)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse( szJSON ) : RT_NULL;
    if( pCfg ) {
        cJSON *pAry = cJSON_GetObjectItem( pCfg, "do" );
        if( pAry ) {
            int n = cJSON_GetArraySize( pAry );
            for( int i = 0; i < n; i++ ) {
                cJSON *pDO = cJSON_GetArrayItem( pAry, i );
                if(pDO) {
                    int n = cJSON_GetInt( pDO, "n", -1 );
                    int va = cJSON_GetInt( pDO, "va", -1 );
                    if( n >= 0 && n < DO_CHAN_NUM && va >= 0 ) {
                        g_xDOResultReg.xDOResult.usDO_xx[n] = (va!=0?PIN_SET:PIN_RESET);
                        if (i < RELAYS_OUTPUT_NUM) {
                            das_do_set_do_state(DAS_DIDO_TYPE_RELAY, i, va);
                        } else {
                            das_do_set_do_state(DAS_DIDO_TYPE_TTL, i - RELAYS_OUTPUT_NUM, va);
                        }
                        if (g_do_cfgs[n].exp[0]) {
                            evaluate(g_do_cfgs[n].exp, NULL, NULL);
                        }
                    }
                }
            }
        }
    }

    cJSON_Delete(pCfg);
    WEBS_PRINTF( "{\"ret\":0}" );
    WEBS_DONE(200);
}

DEF_CGI_HANDLER(getDoValue)
{
    rt_bool_t first = RT_TRUE;

    first = RT_TRUE;
    WEBS_PRINTF( "{\"ret\":0,\"do\":[" );
    for( int i = 0; i < DO_CHAN_NUM; i++ ) {
        cJSON *pItem = cJSON_CreateObject();
        if(pItem) {
            if( !first ) WEBS_PRINTF( "," );
            first = RT_FALSE;
            if (i < RELAYS_OUTPUT_NUM) {
                cJSON_AddNumberToObject( pItem, "va", das_do_get_do_state(DAS_DIDO_TYPE_RELAY, i));
            } else {
                cJSON_AddNumberToObject( pItem, "va", das_do_get_do_state(DAS_DIDO_TYPE_TTL, i - RELAYS_OUTPUT_NUM));
            }
            char *szRetJSON = cJSON_PrintUnformatted( pItem );
            if(szRetJSON) {
                WEBS_PRINTF( szRetJSON );
                rt_free( szRetJSON );
            }
        }
        cJSON_Delete( pItem );
    }
    WEBS_PRINTF( "]}" );

	WEBS_DONE(200);
}

