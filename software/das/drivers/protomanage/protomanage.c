
#include <board.h>

DEF_CGI_HANDLER(getProtoManage)
{
    rt_err_t err = RT_EOK;
    rt_bool_t first = RT_TRUE;

    first = RT_TRUE;
    // uart
    WEBS_PRINTF( "{\"ret\":0,\"rs\":[" );
    for( int n = 0; n < 4; n++ ) {
        rt_int8_t instance = nUartGetInstance( n );
        if( instance >= 0 ) {
            if( PROTO_MASTER == g_uart_cfgs[instance].proto_ms ) {
                cJSON *pItem = cJSON_CreateObject();
                if(pItem) {
                    if( !first ) WEBS_PRINTF( "," );
                    first = RT_FALSE;
                    cJSON_AddNumberToObject( pItem, "id", PROTO_DEV_RS1 + n );
                    cJSON_AddNumberToObject( pItem, "po", g_uart_cfgs[instance].proto_type );
                    char *szRetJSON = cJSON_PrintUnformatted( pItem );
                    if(szRetJSON) {
                        WEBS_PRINTF( szRetJSON );
                        rt_free( szRetJSON );
                    }
                }
                cJSON_Delete( pItem );
            }
        }
    }
    WEBS_PRINTF( "]}" );

    // net gprs zigbee

    if( err != RT_EOK ) {
	    WEBS_PRINTF( "{\"ret\":%d}", err);
	}
	WEBS_DONE(200);
}




