/*
 * File      : das.c
 */

#include <board.h>
#include "mdtypedef.h"
#include "mxml.h"
#include "net_helper.h"

mdBOOL g_bIsTestMode = mdFALSE;

struct das_system_info g_sys_info;
struct das_system_ver g_sys_ver;

rt_thread_t g_AIReadThread;
rt_thread_t g_DIReadThread;
rt_thread_t g_DOWriteThread;

extern mdBOOL g_bIsTestMode;

static void __doSystemReset(void *arg)
{
    sleep(5);
    NVIC_SystemReset();
}

void vDoSystemReset(void)
{
    rt_thread_t th = rt_thread_create("sys_reset", __doSystemReset, RT_NULL, 0x300, 20, 20);
    if (th) {
        rt_thread_startup(th);
    } else {
        NVIC_SystemReset();
    }
}

static void __doSystemReboot(void *arg)
{
    sleep(5);
    NVIC_SystemReboot();
}

void vDoSystemReboot(void)
{
    rt_thread_t th = rt_thread_create("sys_reboot", __doSystemReboot, RT_NULL, 0x300, 20, 20);
    if (th) {
        rt_thread_startup(th);
    } else {
        NVIC_SystemReboot();
    }
}

static int reboot_flag = 0;

static void __thread_idle(void *arg)
{
    rt_tick_t regtick = 0;
    extern rt_tick_t g_ulLastBusyTick;
    int key_tick = 0;

    while (1) {
        rt_tick_t tickNow = rt_tick_get();
        cpu_flash_usage_refresh();

        /*if (tickNow - g_ulLastBusyTick >= rt_tick_from_millisecond(1000)) {
            g_ulLastBusyTick = tickNow;
            rt_pin_write(BOARD_GPIO_NET_LED, PIN_HIGH);
        }*/

        if(!g_bIsTestMode){
            if(KeyIoStatus() == 0){
		        printf("key down!\r\n");
                //key_down = 1;
                if((rt_tick_get() - key_tick) >=  rt_tick_from_millisecond(10000)){
                   printf("\n==========key down 10 s, factory reset!!!!!!\n\n");
                   for(int i = 0 ; i < 6; i++){
                        vTestLedToggle();
                        usleep(100000);
                   }
                   board_cfg_del_all();
                   vDoSystemReset();  
                   key_tick = rt_tick_get();
                }
            }else {
                key_tick = rt_tick_get();
            }
        }

        if (tickNow - regtick >= rt_tick_from_millisecond(30000)) {
            if (!g_isreg) {
                reg_testdo(rt_millisecond_from_tick(tickNow - regtick) / 1000);
                if (reg_testover()) {
                    rt_kprintf("test time over\n");
                    if (
#ifdef USR_TEST_CODE
                        !g_bIsTestMode &&
#endif
                        !g_istestover_poweron) {
                        rt_kprintf("reset system\n");
                        NVIC_SystemReset();
                    }
                }
            }
            regtick = tickNow;
        }
        sleep(2);
#if USE_REBOOT_DAY

          time_t t = time(0);
          struct tm tm;
          das_localtime_r(&t, &tm);
          rt_kprintf("#### current time: %04d/%02d/%02d %02d:%02d:%02d\r\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
          if( (tm.tm_hour == 14) && (tm.tm_min == 0) ){
                if(reboot_flag == 0){
                    reboot_flag = 1;
                    vDoSystemReboot();
                }
          }

#endif
        
    }
}

void rt_thread_idle_init(void)
{
    rt_thread_t th = rt_thread_create("idle", __thread_idle, RT_NULL, 0x300, 20, 20);
    if (th) {
        rt_thread_startup(th);
    }
}

void rt_init_zigbee_thread_entry(void *parameter);
void rt_gprs_thread_entry(void *parameter);

void rt_lora_thread_entry(void *parameter)
{
     lora_init(1);
}


void rt_init_thread_entry(void *parameter)
{
    my_system("rm /tmp/test.log -rf");
#ifdef USR_TEST_CODE

   // rt_pin_mode(BOARD_GPIO_ZIGBEE_SLEEP, PIN_MODE_INPUT_PULLUP);

	if (vIsTestModeIo() == 1) {
		g_bIsTestMode = mdTRUE;
		g_istestover_poweron = mdFALSE;
    	xTestTaskStart();
        my_system("echo 'enter test mode' > /tmp/test.log");
	}
	
#endif

    das_set_time(das_get_time(), 28800);

    cpu_flash_usage_refresh();

    board_cfg_init();
    // 必须先 storage_cfg_init
    storage_cfg_init();
    vStorageInit();
    storage_log_init();

    ft_http_init();

    vDevCfgInit();      // 放在最前面
    host_cfg_init();     // 放在第二
    rule_init();
    reg_init();
    auth_cfg_init();
    net_cfg_init();
    tcpip_cfg_init();
#if BOARD_USE_LORA
    lora_cfg_init();
#endif
    uart_cfg_init();
    gprs_cfg_init();
    xfer_uart_cfg_init();
    vVarManage_ExtDataInit();
    rt_thddog_feed("");

    monitor_init();
    serial_helper_init();

#if BOARD_USE_LORA && !TEST_ON_PC
    __lora_reinit(1);
    if(g_bIsTestMode){
        rt_thread_t init_lora_thread = rt_thread_create("lora", rt_lora_thread_entry, RT_NULL, 512, 20, 20);

        if (init_lora_thread != RT_NULL) {
            rt_thddog_register(init_lora_thread, 120);
            rt_thread_startup(init_lora_thread);
        }
        
    } else {
        lora_init(1);
    }
#endif

    if (!g_istestover_poweron && !g_bIsTestMode ) {

    }

    if (!g_istestover_poweron && !g_bIsTestMode) {
        for (int port = 0; port < BOARD_UART_MAX; port++) {
            if ((UART_TYPE_232 == g_uart_cfgs[port].uart_type
                 || UART_TYPE_485 == g_uart_cfgs[port].uart_type
#if BOARD_USE_LORA
                 || UART_TYPE_LORA == g_uart_cfgs[port].uart_type
#endif
                 ) &&
                (PROTO_MODBUS_RTU == g_uart_cfgs[port].proto_type ||
                 PROTO_MODBUS_ASCII == g_uart_cfgs[port].proto_type)
               ) {

                if (!g_xfer_net_dst_uart_occ[port]) {
                    if (PROTO_SLAVE == g_uart_cfgs[port].proto_ms) {
                        xMBRTU_ASCIISlavePollReStart(port, (PROTO_MODBUS_RTU == g_uart_cfgs[port].proto_type) ? (MB_RTU) : (MB_ASCII));
                    } else if (PROTO_MASTER == g_uart_cfgs[port].proto_ms) {
                        xMBRTU_ASCIIMasterPollReStart(port, (PROTO_MODBUS_RTU == g_uart_cfgs[port].proto_type) ? (MB_RTU) : (MB_ASCII));
                    }
                }
            }
        }
        for (int port = 0; port < BOARD_UART_MAX; port++) {
            if ((UART_TYPE_232 == g_uart_cfgs[port].uart_type
                 || UART_TYPE_485 == g_uart_cfgs[port].uart_type
                 ) &&
                (PROTO_OBMODBUS_RTU == g_uart_cfgs[port].proto_type)
               ) {

                if (!g_xfer_net_dst_uart_occ[port]) {
                    if (PROTO_MASTER == g_uart_cfgs[port].proto_ms) {
                        xOBMBRTU_ASCIIMasterPollReStart(port, MB_RTU);
                    }
                }
            }
        }
        sdccp_serial_openall();
    }

    
    if (!g_istestover_poweron) {
        varmanage_start();

        {
            /*rt_thread_t init_zgb_thread = rt_thread_create("initzgb", rt_init_zigbee_thread_entry, RT_NULL, 512, 20, 20);

            if (init_zgb_thread != RT_NULL) {
                rt_thddog_register(init_zgb_thread, 30);
                rt_thread_startup(init_zgb_thread);
            }*/
        }
        {
            rt_thread_t init_gprs_thread = rt_thread_create("gprs", rt_gprs_thread_entry, RT_NULL, 512, 20, 20);

            if (init_gprs_thread != RT_NULL) {
                rt_thddog_register(init_gprs_thread, 120);
                rt_thread_startup(init_gprs_thread);
            }
        }
    }

    /*extern void lua_rtu_init(void);
    lua_rtu_init();*/

    // 先初始化串口

    if(!g_bIsTestMode) {
        xfer_helper_serial_init();
    }

    //while (!das_do_is_enet_up()) {
      //  rt_thread_delay(200);
    //}

    if(!das_do_is_enet_up()){
        sleep(5);
    }
    
    rt_kprintf("webnet_init\n");
    webnet_init();

    if (!g_istestover_poweron && !g_bIsTestMode ) {
        xfer_helper_enet_init();

        for (int n = 0; n < BOARD_ENET_TCPIP_NUM; n++) {
            if (g_tcpip_cfgs[n].enable && 
                TCP_IP_M_NORMAL == g_tcpip_cfgs[n].mode &&
                (TCP_IP_TCP == g_tcpip_cfgs[n].tcpip_type ||
                 TCP_IP_UDP == g_tcpip_cfgs[n].tcpip_type)) {
                if(PROTO_MODBUS_TCP == g_tcpip_cfgs[n].cfg.normal.proto_type) {
                    if (PROTO_SLAVE == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                        xMBTCPSlavePollReStart(n);
                    } else if (PROTO_MASTER == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                        xMBTCPMasterPollReStart(n);
                    }
                } else if(PROTO_MODBUS_RTU_OVER_TCP == g_tcpip_cfgs[n].cfg.normal.proto_type) {
                    if (PROTO_SLAVE == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                        xMBRTU_OverTCPSlavePollReStart(n);
                    } else if (PROTO_MASTER == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                        xMBRTU_OverTCPMasterPollReStart(n);
                    }
                }
            }
        }

        for (int n = 0; n < BOARD_ENET_TCPIP_NUM; n++) {
            if (g_tcpip_cfgs[n].enable &&
                TCP_IP_M_NORMAL == g_tcpip_cfgs[n].mode &&
                TCP_IP_TCP == g_tcpip_cfgs[n].tcpip_type &&
                //TCPIP_CLIENT == g_tcpip_cfgs[n].tcpip_cs &&
                (
                    PROTO_CC_BJDC == g_tcpip_cfgs[n].cfg.normal.proto_type ||
                    PROTO_HJT212 == g_tcpip_cfgs[n].cfg.normal.proto_type ||
                    PROTO_DM101 == g_tcpip_cfgs[n].cfg.normal.proto_type ||
                    PROTO_SMF == g_tcpip_cfgs[n].cfg.normal.proto_type ||
                    PROTO_MQTT == g_tcpip_cfgs[n].cfg.normal.proto_type ||
                    PROTO_DH == g_tcpip_cfgs[n].cfg.normal.proto_type
                )) {
                cc_net_open(n);
            }
        }
    }

    /*while (!reg_hclient_query()) {
        rt_thddog_feed("redo reg_hclient_query");
        rt_thread_delay(rt_tick_from_millisecond(2000));
    }
    //elog_v("test", "this is a test log!");

    /*for(int i = 0; i < 100; i++) {
        elog_e("test", "testaaaaaaaaaaaaaa----%d", i);
        rt_thddog_feed("elog_e test");
        rt_thread_delay(10);
    }*/

    rt_thddog_unreg_inthd();
}

rt_bool_t g_zigbee_init = RT_FALSE;
rt_bool_t g_lora_init = RT_FALSE;

static uint32_t  last_sync_time_1 = 0;


void rt_gprs_thread_entry(void *parameter)
{
    gprs_do_init();

    while (g_gprs_work_cfg.eWMode != GPRS_WM_SHUTDOWN) {
        if (!das_do_is_gprs_up()) {
            if (is_tcpip_used_gprs()) {
               // rt_kprintf("开始重新ppp拨号\r\n");
                //gprs_do_reinit(0);
            }
        }
        sleep(5);
        rt_thddog_feed("");
        if (das_do_is_gprs_up()) {
            das_ntpd_try_sync();
			rt_thddog_feed("das_ntpd_try_sync");
            //printf("\n############# sync hw clock: %d, %u\n",das_time_is_sync() , (das_sys_time() - last_sync_time_1));
            if(das_time_is_sync()){
                 if ( (das_get_time() - last_sync_time_1) >= 24 * 60 * 60 ){
                    //printf("\n############# sync hw clock\n");
                    system("hwclock -w -u");
                    last_sync_time_1 = das_get_time();
                    printf("\n############# sync hw clock\n");
                 }
            }
        }
    }
    
    rt_thddog_unreg_inthd();
}

void rt_elog_init(void);

int rt_application_init()
{
    my_system("debug off");
    ez_exp_init();
    das_sys_time_init();
    rt_elog_init();
    net_init_all();
    rt_thread_idle_init();
    rt_init_thread_entry(NULL);
    return 0;
}

/*
void HardFault_Handler(void)
{
    list_mem();
    list_thread();
    while (1);
}*/

static void elog_user_assert_hook(const char* ex, const char* func, size_t line);

void rt_elog_init(void)
{
    /* initialize EasyFlash and EasyLogger */
    if ((elog_init() == ELOG_NO_ERR)) {
        /* set enabled format */
        elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL & ~ELOG_FMT_P_INFO);
        elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
        elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
        elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
        elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_ALL & ~(ELOG_FMT_FUNC | ELOG_FMT_P_INFO));
        elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_ALL & ~(ELOG_FMT_FUNC | ELOG_FMT_P_INFO));
        /* set EasyLogger assert hook */
        elog_assert_set_hook(elog_user_assert_hook);
        /* start EasyLogger */
        elog_start();
    } else {
        /* initialize fail and switch to fault status */
    }
}

static void elog_user_assert_hook(const char* ex, const char* func, size_t line)
{
#ifdef ELOG_ASYNC_OUTPUT_ENABLE
    /* disable async output */
    elog_async_enabled(false);
#endif

    /* disable logger output lock */
    elog_output_lock_enabled(false);
    /* output logger assert information */
    elog_a("elog", "(%s) has assert failed at %s:%d.", ex, func, line);
    while(1);
}

/*@}*/

