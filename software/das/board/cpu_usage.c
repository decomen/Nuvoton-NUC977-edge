
#include <board.h>

cpu_usage_t g_xCpuUsage;

void cpu_flash_usage_refresh( void )
{
    das_do_get_system_resource(&g_xCpuUsage);
}

DEF_CGI_HANDLER(getCpuUsage)
{
    char* szRetJSON = RT_NULL;
    cJSON *pItem = cJSON_CreateObject();
    if(pItem) {
        cJSON_AddNumberToObject( pItem, "ret", RT_EOK );
        
        cJSON_AddNumberToObject( pItem, "cpu", g_xCpuUsage.cpu );
        cJSON_AddNumberToObject( pItem, "ms", g_xCpuUsage.mem_all );
        cJSON_AddNumberToObject( pItem, "ma", g_xCpuUsage.mem_max_use );
        cJSON_AddNumberToObject( pItem, "mu", g_xCpuUsage.mem_now_use );
        cJSON_AddNumberToObject( pItem, "fs", g_xCpuUsage.flash_size );
        cJSON_AddNumberToObject( pItem, "fu", g_xCpuUsage.flash_use );
        cJSON_AddNumberToObject( pItem, "ss", g_xCpuUsage.sd_size );
        cJSON_AddNumberToObject( pItem, "su", g_xCpuUsage.sd_use );
        
        szRetJSON = cJSON_PrintUnformatted( pItem );
        if(szRetJSON) {
            WEBS_PRINTF( szRetJSON );
            rt_free( szRetJSON );
        }
    }
    cJSON_Delete( pItem );

	WEBS_DONE(200);
}
