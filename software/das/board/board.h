/*
 * File      : board.h
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 *
 */

// <<< Use Configuration Wizard in Context Menu >>>
#ifndef __BOARD_H__
#define __BOARD_H__

#define BOARD_USE_LORA          (1)

#define REG_TEST_OVER_TIME      (90*24*60*60)    //单位:秒

#define DAS_VER_VERCODE     (122)

#define HW_VER_VERCODE      g_sys_ver.HW_VER

#define SW_VER_MAJOR        (g_sys_ver.SW_VER / 100)
#define SW_VER_MINOR        (g_sys_ver.SW_VER % 100)
#define SW_VER_VERCODE      (g_sys_ver.SW_VER)

#define PRODUCT_MODEL       DM_A401F_PRODUCT_NAME

#define HTTP_SERVER_HOST    "http://api.fitepc.cn"
#define _STR(_str)          (_str)?(_str):""
#define HEX_CH_TO_NUM(c)	(c<='9'?(c-'0'):(c-'A'+10))

#define USE_DEV_BSP         0

#define USE_REBOOT_DAY 0
#define USR_TEST_CODE
#define SX1278_TEST (0)

#include "threaddog.h"

#if 1
#define rt_thddog_register(_rt, _sec)   (_rt)->user_data = (void *)threaddog_register((_rt)->name, _sec, (const void *)(_rt))
#define rt_thddog_unregister(_rt)       threaddog_unregister((thddog_t *)(_rt)->user_data)
#define rt_thddog_unreg_inthd()         rt_thddog_unregister(rt_thread_self())
#define rt_thddog_feed(_desc)           threaddog_feed((thddog_t *)rt_thread_self()->user_data, _desc)
#define rt_thddog_suspend(_desc)        threaddog_suspend((thddog_t *)rt_thread_self()->user_data, _desc)
#define rt_thddog_resume()              threaddog_resume((thddog_t *)rt_thread_self()->user_data, RT_NULL)
#define rt_thddog_exit()                threaddog_exit((thddog_t *)rt_thread_self()->user_data, RT_NULL)
#else
#define rt_thddog_register(_rt, _sec)   
#define rt_thddog_unregister(_rt)       
#define rt_thddog_unreg_inthd()         
#define rt_thddog_feed(_desc)           
#define rt_thddog_suspend(_desc)        
#define rt_thddog_resume()              
#define rt_thddog_exit()                
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

#include <assert.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <netdb.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <semaphore.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <signal.h>
#include <inttypes.h>
#include <sys/sysinfo.h>

#include "rtdef.h"

#include "elog.h"

#include "das_os.h"
#include "bfifo.h"
#include "das_util.h"
#include "das_docking.h"
#include "das_monitor.h"

#include "cpu_usage.h"

#include "protomanage.h"
#include "varmanage.h"
#include "rule.h"

#include "cJSON.h"

#include "queue.h"
#include "rtcrc32.h"
#include "rtu_reg.h"

#include "board_cfg.h"

#include "httpclient.h"
#include "http.h"

#include "zigbee.h"
#include "zgb_std.h"
#include "gprs_helper.h"

#include "lora.h"
#include "lora_std.h"

#include "module.h"
#include "host_cfg.h"
#include "uart_cfg.h"
#include "auth_cfg.h"
#include "dev_cfg.h"
#include "net_cfg.h"
#include "tcpip_cfg.h"
#include "gprs_cfg.h"
#include "zigbee_cfg.h"
#include "lora_cfg.h"
#include "xfer_cfg.h"
#include "xfer_helper.h"
#include "ws_cfg.h"
#include "ws_vm.h"

#include "sdccp_net.h"
#include "sdccp_serial.h"
#include "serial_helper.h"

#include "user_mb_app.h"
//#include "lua.h"
//#include "lauxlib.h"
//#include "lualib.h"

#include "cc_bjdc.h"
#include "iniparser.h"
#include "ez_exp.h"
#include "storage.h"
#include "storage_log.h"

#include "nv.h"
#include "mdtypedef.h"
#include "dev_helper.h"
#include "bytebuffer.h"
#include "sdccp.h"
#include "queue.h"
#include "sdccp_test.h"
#include "server_com.h"
#include "test.h"
#include "mdcrc8.h"
#include "mdcrc32.h"


//#define USR_TEST_CODE

#define BOARD_CREAT_NAME(_s_,_fmt,...) char _s_[RT_NAME_MAX+1]; snprintf( _s_, RT_NAME_MAX+1, _fmt, __VA_ARGS__ );

extern struct das_system_info g_sys_info;
extern struct das_system_ver g_sys_ver;

#define BOARD_UART_0_TYPE           UART_TYPE_485
#define BOARD_UART_1_TYPE           UART_TYPE_485
#define BOARD_UART_2_TYPE           UART_TYPE_GPRS
#define BOARD_UART_3_TYPE           UART_TYPE_ZGB
#define BOARD_UART_4_TYPE           UART_TYPE_LORA

#define BOARD_GPRS_UART             2
#define BOARD_ZGB_UART              3
#define BOARD_LORA_UART             4

typedef enum
{
    E_4G_EC20,      //4G网络
    E_NBIOT_BC26,   //NBIOT
    E_AIR720,       //合宙4G模块
    E_GPRS_M26,     //2G网络
    E_4G_EC200S,      //4G模块低成本方案
}eCELLNetTYPE_t;

//移远4G ttyUSB2 -> AT
//M26 2G模块 ttyS6

#include "uart_cfg.h"

typedef struct {
    uart_type_e eUart0Type;
    uart_type_e eUart1Type;
} s_Rs232_Rs485_Stat;

void vTestRs232Rs485(s_Rs232_Rs485_Stat *pState);

extern eCELLNetTYPE_t g_xCellNetType;
eCELLNetTYPE_t vCheckCellNetType(void);


void rt_hw_board_init(void);

#define DEF_CGI_HANDLER(_func)      void _func( struct webnet_session * session )
#define CGI_GET_ARG(_tag)           webnet_request_get_query( session->request, _tag )
#define WEBS_PRINTF(_fmt,...)       webnet_session_printf( session, _fmt, ##__VA_ARGS__ )
#define WEBS_DONE(_code)            session->request->result_code = (_code)

void vDoSystemReset(void);
void vDoSystemReboot(void);

#endif

// <<< Use Configuration Wizard in Context Menu >>>
