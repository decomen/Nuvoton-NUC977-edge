/*
 * File      : net_broadcast.c
 
 * Change Logs:
 * Date           Author       Notes
 * 2016/06/12      Jay               
 */

#include <board.h>

static rt_thread_t s_xNetBroadcastThread;

void vNetBroadcastTask( void* parameter )
{
    while(1) {
        
    }
    s_xNetBroadcastThread = RT_NULL;
}

rt_err_t xNetBroadcastReStart( void )
{
    vNetBroadcastStop();

    if( RT_NULL == s_xNetBroadcastThread ) {
        s_xNetBroadcastThread = rt_thread_create( "netbroad", vNetBroadcastTask, RT_NULL, 1024, RT_THREAD_PRIORITY_MAX - 4, 50 );
        
        if( s_xNetBroadcastThread ) {
            rt_thread_startup( s_xNetBroadcastThread );
            return RT_EOK;
        }
    }

    return RT_ERROR;
}

void vNetBroadcastStop( void )
{
    if( s_xNetBroadcastThread ) {
        if( RT_EOK == rt_thread_delete( s_xNetBroadcastThread ) ) {
            s_xNetBroadcastThread = RT_NULL;
        }
    }
}

