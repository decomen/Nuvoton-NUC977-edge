

#include "rtdef.h"
#include <board.h>


static rt_thread_t g_TestTaskThread = NULL;
static rt_thread_t g_TestUartReciveThread = NULL;

static rt_thread_t g_TestUart0Thread = NULL;
static rt_thread_t g_TestUart1Thread = NULL;
static rt_thread_t g_TestUart2Thread = NULL;
static rt_thread_t g_TestUart3Thread = NULL;
static rt_thread_t g_TestRelaysThread = NULL;

queue_test_t *p_com_queue = NULL; 
queue_head_t g_queue_com_head ;
s_TestUartInfo g_xTestUartInfo[HW_MAX];


static int test_serial_helper_select(int fd, int usec)
{
    if (fd >= 0) {
        int s_rc;
        fd_set rset;
        struct timeval tv = { .tv_sec = usec / 1000000, .tv_usec = usec % 1000000 };
        FD_ZERO(&rset); FD_SET(fd, &rset);
        
        while ((s_rc = select(fd + 1, &rset, NULL, NULL, &tv)) == -1) {
            if (errno == EINTR) {
                FD_ZERO(&rset);
                FD_SET(fd, &rset);
            } else {
                return -1;
            }
        }
        return s_rc;
    }
}


static void prvTestUartReciveTask( void* parameter )
{
    mdBYTE buf[512] = {0};
    rt_kprintf("prvTestUartReciveTask \n");

    while(1){
            if(test_serial_helper_select(g_xTestUartInfo[HW_COM].fd, 1000) > 0){
                  int n = read(g_xTestUartInfo[HW_COM].fd, buf, 512);
                  for(int j = 0; j < n ; j++){
                    bQueueWrite(g_xTestUartInfo[HW_COM].queue,buf[j]);
                   // printf("%02x ",buf[j]);
                  }
                 // printf("\n");
                 // printf("\n num: %d, fd: %d\n",HW_COM,g_xTestUartInfo[HW_COM].fd);
            }
        
   }
}

static void prvTestUart0ReciveTask( void* parameter )
{
    int i = 0;
    mdBYTE buf[512] = {0};
    rt_kprintf("prvTestUart0ReciveTask \n");

    while(1){
           // if(test_serial_helper_select(g_xTestUartInfo[HW_UART0].fd, 1000) > 0){
              int n = read(g_xTestUartInfo[HW_UART0].fd, buf, 512);
              for(int j = 0; j < n ; j++){
                bQueueWrite(g_xTestUartInfo[HW_UART0].queue,buf[j]);
               // printf("%02x\n",buf[j]);
              }
             // printf("========num: %d, fd: %d\n",HW_UART0,g_xTestUartInfo[HW_UART0].fd);
              usleep(1000);
           // }
        
   }
}

static void prvTestUart1ReciveTask( void* parameter )
{
    int i = 0;
    mdBYTE buf[512] = {0};
    rt_kprintf("prvTestUart1ReciveTask \n");

    while(1){
           // if(test_serial_helper_select(g_xTestUartInfo[HW_UART1].fd, 1000) > 0){
                  int n = read(g_xTestUartInfo[HW_UART1].fd, buf, 512);
                  for(int j = 0; j < n ; j++){
                    bQueueWrite(g_xTestUartInfo[HW_UART1].queue,buf[j]);
                   // printf("%02x\n",buf[j]);
                  }
                  //printf("=========num: %d, fd: %d\n",HW_UART1,g_xTestUartInfo[HW_UART1].fd);
                  usleep(1000);

           // }
        
     }
}


mdBOOL bRelaysBegain = mdFALSE;


static void prvTestRelaysTask( void* parameter )
{
}


static int led_flag = 0;
void vTestLedToggle()
{
    s_io_t iodata;
    
    if(led_flag == 0){
    	iodata.gpio = 0;
        iodata.dir = 1;
        iodata.val = 0;
        das_do_io_ctrl(UPDATE_LED, &iodata);
        led_flag = 1;
    }else {
        iodata.gpio = 0;
        iodata.dir = 1;
        iodata.val = 1;
        das_do_io_ctrl(UPDATE_LED, &iodata);
        led_flag = 0;
    }
}



int vIsTestModeIo(void)
{
    s_io_t iodata1;
    iodata1.gpio = 0;
    iodata1.dir = 0;
    iodata1.val = -1;
    das_do_io_ctrl(FACTORY_KEY, &iodata1);
    
    printf("iodata1.val: %d\r\n",iodata1.val);
    return(iodata1.val==0);
}

int KeyIoStatus(void)
{
    s_io_t iodata;
    iodata.gpio = 0;
    iodata.dir = 0;
    iodata.val = -1;
    das_do_io_ctrl(FACTORY_KEY, &iodata);
    return (iodata.val);
}




static void prvTestTask( void* parameter );


rt_err_t xTestTaskStart( void )
{

   // g_AIReadThread = rt_thread_create("AI", vAIReadTask, RT_NULL, 0x300, 20, 20);

    if( RT_NULL == g_TestTaskThread ) {
        g_TestTaskThread = rt_thread_create( "TEST", prvTestTask, RT_NULL, 4096, 20, 20 );
        
        if( g_TestTaskThread ) {
            rt_thread_startup( g_TestTaskThread );
            return RT_EOK;
        }
    }

    return RT_ERROR;
}


static int ReciveUartData(uint32_t instance , u8 *buf, int length, int read_timeout, int interval_timeout, int total_timeout)
{
	u8 ch = 0;
	u32 begin = rt_tick_get();
	u32 total_begin = rt_tick_get();

	queue_test_t *p_uart = &(g_xTestUartInfo[instance].queue);
	
	u32 index = 0;
	memset(buf, 0, length);

	index = 0;
	while (1) {
		if (bQueueRead((*p_uart), ch) == STATUS_OK) {
			buf[index++] = ch;
			break;
		}
		if (SDCCP_CHECK_TIME_OUT(begin, rt_tick_from_millisecond(read_timeout))) return 0;
	}

	begin = rt_tick_get();

	while (1) {
		if (bQueueRead((*p_uart), ch) == STATUS_OK) {
			begin = rt_tick_get();
			buf[index++] = ch;
			if (index >= length) break;
		}
		if (SDCCP_CHECK_TIME_OUT(begin,  rt_tick_from_millisecond(interval_timeout)) || SDCCP_CHECK_TIME_OUT(begin,  rt_tick_from_millisecond(total_timeout))) return index;
	}
	return index;
}


void vUartSend(uint32_t instance, const char *buf,int len)
{
    //printf("len: %d\n",len);
	write(g_xTestUartInfo[instance].fd, buf,len);
}

static int led_toggle_flag = 1;
static int key_down_flag = 0;

static mdUINT32 start_tick_test_mode = 0;

static void prvTestTask( void* parameter )
{

	rt_kprintf("\r\n\r\n######## enter test mode!!!!!! \n");
	//rt_thread_delay( RT_TICK_PER_SECOND);	

   start_tick_test_mode =   rt_tick_get();

    memset(&g_xTestUartInfo,0,sizeof(g_xTestUartInfo));

    //gAdcCfgPram.ChannelSleepTime = 2;  //测试模式下加快ADC的刷新速度
    //gAdcCfgPram.eMode = ADC_MODE_TEST; //测试模式下,使用理论值计算ADC值

    p_com_queue = &(g_xTestUartInfo[HW_COM].queue);


   /* int fd = -1;
    fd = open(TEST_UART_NAME, O_RDWR|O_NOCTTY|O_NDELAY);
    if(fd <= 0){
       rt_kprintf("open %s failed\r\n",TEST_UART_NAME); 
       return;
    }
    g_com_fd = fd;
    das_do_set_hw_com_info(g_com_fd, 115200, 8, 1, 0);*/


    for(int i = HW_UART0; i < HW_MAX; i++){
          int fd = -1;
          const char *dev_name = das_do_get_uart_driver_name(i);
          if (dev_name && dev_name[0]) {
                if(i < HW_COM){
                    fd = open(dev_name, O_RDWR|O_NOCTTY|O_NDELAY);
                    printf("name: %s, fd: %d\n", dev_name,fd);
                }else {
                    fd = open(TEST_UART_NAME, O_RDWR|O_NOCTTY|O_NDELAY);
                    printf("test uart name: %s, fd: %d\n", TEST_UART_NAME,fd);
                }
          }
          if(fd > 0){
            g_xTestUartInfo[i].fd = fd;
            das_do_set_hw_com_info(fd, 115200, 8, 1, 0);
          }else {
             rt_kprintf("open %s failed\r\n",dev_name); 
             return ;
          }
    }

    if( RT_NULL == g_TestUartReciveThread ) {
        g_TestUartReciveThread = rt_thread_create( "test_uart_recive", prvTestUartReciveTask, RT_NULL, 4096, 20, 20 );
        
        if( g_TestUartReciveThread ) {
            rt_thread_startup( g_TestUartReciveThread );
        }
    }

    if( RT_NULL == g_TestUart0Thread ) {
        g_TestUart0Thread = rt_thread_create( "uart0_recive", prvTestUart0ReciveTask, RT_NULL, 4096, 20, 20 );
        
        if( g_TestUart0Thread ) {
            rt_thread_startup( g_TestUart0Thread );
        }
    }

    if( RT_NULL == g_TestUart1Thread ) {
        g_TestUart1Thread = rt_thread_create( "uart1_recive", prvTestUart1ReciveTask, RT_NULL, 4096, 20, 20 );
        
        if( g_TestUart1Thread ) {
            rt_thread_startup( g_TestUart1Thread );
        }
    }

	 vTestLedToggle();
    
	 int cnt = 0;
	 int relay_cnt = 0;

	 int led_cnt = 0 ;
	    
	S_PACKAGE_HEAD *p_head = NULL;
	vQueueClear(g_queue_com_head);
    for(int i = HW_UART0; i < HW_MAX; i++){
	    vQueueClear(g_xTestUartInfo[i].queue);
    }

	mdBYTE  ch = 0 ;

	while(1){
		
		ch = '\0';
        
		if (bQueueRead((*p_com_queue), ch) == mdTRUE) {

			if ((p_head = try2match_com_head(ch)) != NULL) {
				while (client_event_handler_com(p_head) == SDCCP_RETRY) {
					
				}
			}

		}
		  
        if(led_toggle_flag == 1){
            if(led_cnt++ >= 5){
                led_cnt = 0;
    		    vTestLedToggle();
            }
        }

        rt_thread_delay(20);


        if(rt_tick_get() - start_tick_test_mode > 10000){
            if(KeyIoStatus() == 0){
                key_down_flag = 1;
            }else if(key_down_flag == 1){
                if(led_toggle_flag == 1) led_toggle_flag = 0;
                else if(led_toggle_flag == 0) led_toggle_flag = 1;
                printf("test key down : %d\r\n", led_toggle_flag);
                key_down_flag = 0;
            }else {
                key_down_flag = 0;
            }
        }

	}
}


void set_product_info(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_SET_PRODUCT_INFO_REQUEST request;
    memset(&request,0,sizeof(S_MSG_SET_PRODUCT_INFO_REQUEST));

  //  printf("set_product_info\n");
	
	bb_get_bytes(bb , request.info.xSN.szSN , sizeof(request.info.xSN));
	bb_get_bytes(bb,request.info.xHwVer.hw_ver,sizeof(request.info.xHwVer));
	bb_get_bytes(bb,request.info.xSwVer.sw_ver,sizeof(request.info.xSwVer));
	bb_get_bytes(bb ,request.info.xOEM.szOEM , sizeof(request.info.xOEM));
	bb_get_bytes(bb ,request.info.xIp.szIp , sizeof(request.info.xIp));
	bb_get_bytes(bb ,request.info.xNetMac.mac , sizeof(request.info.xNetMac));
	request.info.usYear = bb_get_short(bb);
	request.info.usMonth = bb_get_short(bb);
	request.info.usDay = bb_get_short(bb);
	request.info.usHour = bb_get_short(bb);
	request.info.usMin = bb_get_short(bb);
	request.info.usSec = bb_get_short(bb);
	bb_get_bytes(bb ,request.info.hwid.hwid , sizeof(request.info.hwid.hwid));
	bb_get_bytes(bb ,request.info.xPN.szPN, sizeof(request.info.xPN));

    /*
       printf("  -> DEV_ID: %s\n", g_sys_info.DEV_ID);
	printf("  -> SN_ID: %s\n", g_sys_info.SN);
	printf("  -> UUID: %s\n", g_sys_info.HW_ID);
	printf("  -> PROD_DATE: %s\n", g_sys_info.PROD_DATE);
	printf("  -> REG_STATUS: %d\n", g_sys_info.REG_STATUS);
	printf("  -> TEST_REMAIN: %d\n", g_sys_info.TEST_REMAIN);
	printf("  -> DEV_MODEL: 0x%x\n", g_sys_info.DEV_MODEL);		
	printf("  -> SYS_DATE : %s\n", g_sys_info.SYS_DATE);
	printf("  -> RUNTIME: %d minutes\n", g_sys_info.RUNTIME);
	printf("  -> DESC: %s\n", g_sys_info.DESC);
	printf("  -> HW_VER: %s\n", g_sys_info.HW_VER);
	printf("  -> SW_VER: %s\n", g_sys_info.SW_VER);
    */


    static char pro_date[64] = {0};
    sprintf(pro_date,"%04d/%02d/%02d-%02d:%02d:%02d",request.info.usYear,request.info.usMonth,request.info.usDay,request.info.usHour,request.info.usMin,request.info.usSec);

    //printf("\nwrite devinfo pn: %s, sn: %s, pro_date: %s\r\n\n", request.info.xPN.szPN,request.info.xSN.szSN, pro_date );

    WriteDevInfo(request.info.xPN.szPN, request.info.xSN.szSN, pro_date);


  //  gen_dev_json_file();

    host_cfg_init();

        
	S_MSG_SET_PRODUCT_INFO_RESPONSE response;
	response.Type = 0;
	response.Value = 0;
	response.RetCode = MSG_TEST_ERROR_OK;

	
	send_base_response(msg, &response);
}


mdBOOL  bIsInitOk = mdFALSE;
mdBOOL  bIsMountOk = mdFALSE;
mdBOOL  bIsMusicListOk = mdFALSE;

/*
#define BOARD_SDCARD_MOUNT  "/media/sdcard"

void vTestSdCard(const S_MSG *msg, byte_buffer_t *bb)
{

    printf("vTestSdCard\r\n");
	S_MSG_TEST_SDCARD_RESPONSE response;
	
	response.Type = 0;
	response.RetCode = MSG_TEST_ERROR_ERR;
	response.Value = 0;
  
	int size = 0;
	if( rt_sd_in() ) {
		response.RetCode = MSG_TEST_ERROR_OK;
	}
	
	send_base_response(msg, &response);
	
}*/

extern mdBOOL bIsGprsInitOk ;

void vTestGprs(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_TEST_GPRS_RESPONSE response;
	
	response.Type = 0;
	response.RetCode = MSG_TEST_ERROR_ERR;
	response.Value = 0;


    if(das_do_is_gprs_up()){
	    response.RetCode = MSG_TEST_ERROR_OK;
    }
	
	send_base_response(msg, &response);
	
}

extern rt_bool_t g_zigbee_init;

void vTestZigbee(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_TEST_ZIGBEE_RESPONSE response;
	
	response.Type = 0;
	response.RetCode = MSG_TEST_ERROR_ERR;

	response.Value = 0;

	if(g_zigbee_init == RT_TRUE){
		response.RetCode = MSG_TEST_ERROR_OK;
		response.Value = (g_zigbee_cfg.usType | (g_zigbee_cfg.usVer << 16));
	}
	
	send_base_response(msg, &response);
	
}


void vTestLora(const S_MSG *msg, byte_buffer_t *bb)
{
    rt_kprintf("test lora\n");
	S_MSG_TEST_ZIGBEE_RESPONSE response;
	
	response.Type = 0;
	response.RetCode = MSG_TEST_ERROR_ERR;

	response.Value = 0;

	if(lora_at_test()){
		response.RetCode = MSG_TEST_ERROR_OK;
		response.Value = 0;
	}
	
	send_base_response(msg, &response);
	
}

static int rtc_post_skip(u32 *diff)
{

	u32 start1 = 0;
	u32 start2 = 0;

	//sec_to_date( my_get_time(), &tm1 );
	time_t timestamp1 = time(NULL);
	time_t timestamp2;

	//RTC_GetTime(&tm1);
	start1 = rt_tick_get();

	while (1) {
		//RTC_GetTime(&tm2);
		timestamp2 = time(NULL);
		start2 = rt_tick_get();

		if (timestamp1 != timestamp2) {
			break;
		}

		if (start2 - start1 > (1500)) { // 1.5s
			break;
		}
	}
	
	if (timestamp2 != timestamp2) {
		*diff = (start2 - start1);
		return 0;
	} else {
		return -1;
	}
}

void vTestRtc(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_TEST_RTC_RESPONSE response;
	u32 diff;
	u32 i;

	response.Type = 0;
	response.RetCode = MSG_TEST_ERROR_RTC;
	response.Value = 0;
/*
	for (i = 0; i < 1; i++) {
		if (rtc_post_skip(&diff) == 0) {
			break;
		}
	}

	if (i >= 2) {
		goto _END_;
	}

	for (i = 0; i < 2; i++) {
		if (rtc_post_skip(&diff) != 0) {
			break;
		}

		if ((diff < (950)) || (diff > (1050)) ) {
			break;
		}
	}*/

    u32 start1 = 0;
	u32 start2 = 0;

	time_t timestamp1 = time(NULL);
	time_t timestamp2;

	start1 = rt_tick_get();

	while (1) {
        
		timestamp2 = time(NULL);
		start2 = rt_tick_get();

		if (start2 - start1 > (1500)) { // 1.5s
			break;
		}
        if(timestamp2 != timestamp1) break;
        
	}

	if(timestamp2 != timestamp1) {
		response.RetCode = MSG_TEST_ERROR_OK;
	}

_END_:
	send_base_response(msg, &response);
}


void vTestSpiFlash(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_TEST_SPIFLASH_RESPONSE response;
	
	response.Type = 0;
	response.RetCode = MSG_TEST_ERROR_OK;
	response.Value = 0;

	send_base_response(msg, &response);
}


#define PHY_BMCR                    (0x00) /* Basic Control */
#define PHY_BMCR_RESET              (0x8000)
#define PHY_PHYIDR1                 (0x02) /* PHY Identifer 1 */
#define ENET_DELAY_MS(_ms)          rt_thread_delay( RT_TICK_PER_SECOND * (_ms) / 1000 )

void vTestNet(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_TEST_NET_RESPONSE response;
	
	response.Type = 0;
	response.RetCode = MSG_TEST_ERROR_ERR;
	response.Value = 0;

    if(das_do_is_enet_up()){
       response.RetCode = MSG_TEST_ERROR_OK; 
    }

	send_base_response(msg, &response);
}

/*
static eInOut_stat_t sta;
static eInOut_stat_t sta_bak;
static mdBOOL bTTLInputChang(eTTL_Input_Chanel_t chan)
{
	vTTLInputputGet(chan, &sta);
	if(sta != sta_bak){
		sta_bak = sta;
		return mdTRUE;
	}
	return mdFALSE;
}
*/
static unsigned int cnt = 0;
#define CNT_TTL_MIN (10)

void vTestTTLInput(const S_MSG *msg, byte_buffer_t *bb)
{
	/*S_MSG_TEST_TTL_INPUT_RESPONSE response;
	S_MSG_TEST_TTL_INPUT_REQUEST  request;
	//vInOutInit();
	
	request.Type = bb_get(bb);
	
	response.Type = request.Type;
	response.Value = 0;
	response.RetCode = MSG_TEST_ERROR_ERR;

	switch(request.Type){
		case TTLInput_GET_1:{
			 cnt = 0;
			int begin = rt_tick_get();
			while(1){
				if(bTTLInputChang(TTL_INPUT_1)){
					cnt++;
				}
				if(SDCCP_CHECK_TIME_OUT(begin, rt_tick_from_millisecond(500))) break;
				//rt_thread_delay( RT_TICK_PER_SECOND/1000);
				
			}
			if(cnt >= CNT_TTL_MIN){
				response.RetCode = MSG_TEST_ERROR_OK;
			}
			break;
		}
		case TTLInput_GET_2:{
			 cnt = 0;
			int begin = rt_tick_get();
			while(1){
				if(bTTLInputChang(TTL_INPUT_2)){
					cnt++;
				}
				if(SDCCP_CHECK_TIME_OUT(begin, rt_tick_from_millisecond(500))) break;
				//rt_thread_delay( RT_TICK_PER_SECOND/1000);
				
			}
			if(cnt >= CNT_TTL_MIN){
				response.RetCode = MSG_TEST_ERROR_OK;
			}
			break;
		}
		case TTLInput_GET_3:{
			 cnt = 0;
			int begin = rt_tick_get();
			while(1){
				if(bTTLInputChang(TTL_INPUT_3)){
					cnt++;
				}
				if(SDCCP_CHECK_TIME_OUT(begin, rt_tick_from_millisecond(500))) break;
				//rt_thread_delay( RT_TICK_PER_SECOND/1000);
				
			}
			if(cnt >= CNT_TTL_MIN){
				response.RetCode = MSG_TEST_ERROR_OK;
			}
			break;
		}
		case TTLInput_GET_4:{
			 cnt = 0;
			int begin = rt_tick_get();
			while(1){
				if(bTTLInputChang(TTL_INPUT_4)){
					cnt++;
				}
				if(SDCCP_CHECK_TIME_OUT(begin, rt_tick_from_millisecond(500))) break;
				//rt_thread_delay( RT_TICK_PER_SECOND/1000);
				
			}
			if(cnt >= CNT_TTL_MIN){
				response.RetCode = MSG_TEST_ERROR_OK;
			}
			break;
		}
        case TTLInput_GET_5:{
			 cnt = 0;
			int begin = rt_tick_get();
			while(1){
				if(bTTLInputChang(TTL_INPUT_5)){
					cnt++;
				}
				if(SDCCP_CHECK_TIME_OUT(begin, rt_tick_from_millisecond(500))) break;
				//rt_thread_delay( RT_TICK_PER_SECOND/1000);
				
			}
			if(cnt >= CNT_TTL_MIN){
				response.RetCode = MSG_TEST_ERROR_OK;
			}
			break;
		}
        case TTLInput_GET_6:{
			 cnt = 0;
			int begin = rt_tick_get();
			while(1){
				if(bTTLInputChang(TTL_INPUT_6)){
					cnt++;
				}
				if(SDCCP_CHECK_TIME_OUT(begin, rt_tick_from_millisecond(500))) break;
				//rt_thread_delay( RT_TICK_PER_SECOND/1000);
				
			}
			if(cnt >= CNT_TTL_MIN){
				response.RetCode = MSG_TEST_ERROR_OK;
			}
			break;
		}
        case TTLInput_GET_7:{
			 cnt = 0;
			int begin = rt_tick_get();
			while(1){
				if(bTTLInputChang(TTL_INPUT_7)){
					cnt++;
				}
				if(SDCCP_CHECK_TIME_OUT(begin, rt_tick_from_millisecond(500))) break;
				//rt_thread_delay( RT_TICK_PER_SECOND/1000);
				
			}
			if(cnt >= CNT_TTL_MIN){
				response.RetCode = MSG_TEST_ERROR_OK;
			}
			break;
		}
        case TTLInput_GET_8:{
			 cnt = 0;
			int begin = rt_tick_get();
			while(1){
				if(bTTLInputChang(TTL_INPUT_8)){
					cnt++;
				}
				if(SDCCP_CHECK_TIME_OUT(begin, rt_tick_from_millisecond(500))) break;
				//rt_thread_delay( RT_TICK_PER_SECOND/1000);
				
			}
			if(cnt >= CNT_TTL_MIN){
				response.RetCode = MSG_TEST_ERROR_OK;
			}
			break;
		}
		default:
			response.RetCode = MSG_TEST_ERROR_UNKNOWN;
		break;
		
	}
	send_base_response(msg, &response);*/
}



/*
static eInOut_stat_t sta_on;
static eInOut_stat_t sta_bak_on;
static mdBOOL bOnOffInputChang(eOnOff_Input_Chanel_t chan)
{
	vOnOffInputGet(chan, &sta_on);
	if(sta_on != sta_bak_on){
		sta_bak_on = sta_on;
		return mdTRUE;
	}
	return mdFALSE;
}
*/
/*

void vTestOnOffInput(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_TEST_ON_OFF_RESPONSE response;
	S_MSG_TEST_ON_OFF_REQUEST  request;

	request.Type = bb_get(bb);
	
	response.Type = request.Type;
	response.Value = 0;
	response.RetCode = MSG_TEST_ERROR_ERR;
	
	switch(request.Type){
		case ONOFF_1:{
			int cnt = 0;
			int begin = rt_tick_get();
			while(1){
				if(bOnOffInputChang(ONOFF_INPUT_1)){
					cnt++;
				}
				if(SDCCP_CHECK_TIME_OUT(begin, rt_tick_from_millisecond(500))) break;
				//rt_thread_delay( RT_TICK_PER_SECOND/1000);
				
			}
			if(cnt >= CNT_TTL_MIN){
				response.RetCode = MSG_TEST_ERROR_OK;
			}
			break;
		}
		case ONOFF_2:{
			int cnt = 0;
			int begin = rt_tick_get();
			while(1){
				if(bOnOffInputChang(ONOFF_INPUT_2)){
					cnt++;
				}
				if(SDCCP_CHECK_TIME_OUT(begin, rt_tick_from_millisecond(500))) break;
				//rt_thread_delay( RT_TICK_PER_SECOND/1000);
				
			}
			if(cnt >= CNT_TTL_MIN){
				response.RetCode = MSG_TEST_ERROR_OK;
			}
			break;
		}
		case ONOFF_3:{
			int cnt = 0;
			int begin = rt_tick_get();
			while(1){
				if(bOnOffInputChang(ONOFF_INPUT_3)){
					cnt++;
				}
				if(SDCCP_CHECK_TIME_OUT(begin, rt_tick_from_millisecond(500))) break;
				//rt_thread_delay( RT_TICK_PER_SECOND/1000);
				
			}
			if(cnt >= CNT_TTL_MIN){
				response.RetCode = MSG_TEST_ERROR_OK;
			}
			break;
		}
		case ONOFF_4:{
			int cnt = 0;
			int begin = rt_tick_get();
			while(1){
				if(bOnOffInputChang(ONOFF_INPUT_4)){
					cnt++;
				}
				if(SDCCP_CHECK_TIME_OUT(begin, rt_tick_from_millisecond(500))) break;
				//rt_thread_delay( RT_TICK_PER_SECOND/1000);
				
			}
			if(cnt >= CNT_TTL_MIN){
				response.RetCode = MSG_TEST_ERROR_OK;
			}
			break;
		}
		default:
			response.RetCode = MSG_TEST_ERROR_UNKNOWN;
		break;
		
	}

	
	
	send_base_response(msg, &response);
}

*/
/*
extern AIResult_t g_AIEngUnitResult;
extern AIResult_t g_AIMeasResult;
*/

void vTestGetAdc(const S_MSG *msg, byte_buffer_t *bb)
{
  //  printf("vTestGetAdc\r\n");
	/*S_MSG_GET_TEST_ADC_REQUEST    request;
	S_MSG_TEST_GET_ADC_RESPONSE   response;

	double  EngUnit = 0.0f;
	double  Measure = 0.0f;

    request.Type =     bb_get(bb);
	response.RetCode = MSG_TEST_ERROR_ERR;

	gAdcCfgPram.eMode = ADC_MODE_TEST;
	gAdcCfgPram.ChannelSleepTime = 1;

#if 1
    const rt_uint8_t chan_tbl[] = { 6, 7, 0, 1, 2, 3, 4, 5 };
    s_CorrectionFactor_t factor = {.factor = 0};
    s_AdcValue_t xAdcVal;
    AIResult_t AIEngUnitResult;
    AIResult_t AIMeasResult;
    memset(&AIEngUnitResult,0,sizeof(AIResult_t));
    memset(&AIMeasResult,0,sizeof(AIResult_t));


    u32 begin = rt_tick_get();
    int cnt = 0;

    do{
        for(ADC_CHANNEL_E chan = ADC_CHANNEL_0; chan <= ADC_CHANNEL_7; chan++){
            vGetAdcValueTest(chan, Range_0_20MA, Range_4_20MA, 0, 0, factor, &xAdcVal);
            int index = chan_tbl[chan];
            AIEngUnitResult.fAI_xx[index] = (float)xAdcVal.usEngUnit;
            AIMeasResult.fAI_xx[index]  = xAdcVal.fMeasure;
         //   usleep(100);
        }
        if (SDCCP_CHECK_TIME_OUT(begin, 250)) {
			break;
		}
    }while(1);

    begin = rt_tick_get();
    do{
        for(ADC_CHANNEL_E chan = ADC_CHANNEL_0; chan <= ADC_CHANNEL_7; chan++){
            vGetAdcValueTest(chan, Range_0_20MA, Range_4_20MA, 0, 0, factor, &xAdcVal);
            int index = chan_tbl[chan];
            AIEngUnitResult.fAI_xx[index] = (float)xAdcVal.usEngUnit;
            AIMeasResult.fAI_xx[index]  = xAdcVal.fMeasure;
         //   usleep(100);
        }
        if (SDCCP_CHECK_TIME_OUT(begin, 100)) {
			break;
		}
    }while(1);

    memset(&AIEngUnitResult,0,sizeof(AIResult_t));
    memset(&AIMeasResult,0,sizeof(AIResult_t));
    for(ADC_CHANNEL_E chan = ADC_CHANNEL_0; chan <= ADC_CHANNEL_7; chan++){
        vGetAdcValueTest(chan, Range_0_20MA, Range_4_20MA, 0, 0, factor, &xAdcVal);
        int index = chan_tbl[chan];
        AIEngUnitResult.fAI_xx[index] = (float)xAdcVal.usEngUnit;
        AIMeasResult.fAI_xx[index]  = xAdcVal.fMeasure;
    }
    
	EngUnit =   AIEngUnitResult.fAI_xx[request.Type];
    Measure =   AIMeasResult.fAI_xx[request.Type];
#else 
    rt_thread_delay(250);
    EngUnit =   g_AIEngUnitResult.fAI_xx[request.Type];
    Measure =   g_AIMeasResult.fAI_xx[request.Type];
#endif
	
	response.xAdcInfo.usChannel = request.Type;
	response.xAdcInfo.usRange =  Range_0_20MA;
	response.xAdcInfo.usEngVal = (unsigned int)EngUnit;

	//获取0量程时的ADC值
	if(EngUnit < 5000){ //表示是0量程
		//float factor = Measure;
		//gCalEntryBak[request.Type].factor = factor;
	}
	
	if(Measure < 0.0f){
		Measure = 0.0f;
	}
	response.xAdcInfo.usMeasureVal = (mdUINT32)(Measure * 1000);
	response.RetCode = MSG_TEST_ERROR_OK;

	send_adc_get_response(msg, &response);*/
}



//获取校验过后的ADC值

void vTestGetTestAdcValue(const S_MSG *msg, byte_buffer_t *bb)
{
	/*S_MSG_GET_TEST_ADC_REQUEST    request;
	S_MSG_TEST_GET_ADC_RESPONSE   response;
	
	request.Type =   bb_get(bb);

	response.RetCode = MSG_TEST_ERROR_ERR;

    if(request.Type == ADC_CHANNEL_0){
        vAdcCalCfgInit();
    }

	 gAdcCfgPram.eMode = ADC_MODE_CALC;


     
	const rt_uint8_t chan_tbl[] = { 6, 7, 0, 1, 2, 3, 4, 5 };

    s_CorrectionFactor_t factor = {.factor = 0};
    s_AdcValue_t xAdcVal;
    AIResult_t AIEngUnitResult;
    AIResult_t AIMeasResult;
    memset(&AIEngUnitResult,0,sizeof(AIResult_t));
    memset(&AIMeasResult,0,sizeof(AIResult_t));

 //   double EngVal = 0;
//    double MesureVal = 0;
    //das_delay(250000);

    u32 begin = rt_tick_get();
    int cnt = 0;

    do{
        for(ADC_CHANNEL_E chan = ADC_CHANNEL_0; chan <= ADC_CHANNEL_7; chan++){
            vGetAdcValueCal(chan, Range_0_20MA, Range_4_20MA, 0, 0, factor, &xAdcVal);
            int index = chan_tbl[chan];
            AIEngUnitResult.fAI_xx[index] = (float)xAdcVal.usEngUnit;
            AIMeasResult.fAI_xx[index]  = xAdcVal.fMeasure;
         //   usleep(100);
        }
        if (SDCCP_CHECK_TIME_OUT(begin, 250)) {
			break;
		}
    }while(1);

    begin = rt_tick_get();

     do{
        for(ADC_CHANNEL_E chan = ADC_CHANNEL_0; chan <= ADC_CHANNEL_7; chan++){
            vGetAdcValueCal(chan, Range_0_20MA, Range_4_20MA, 0, 0, factor, &xAdcVal);
            int index = chan_tbl[chan];
            AIEngUnitResult.fAI_xx[index] = (float)xAdcVal.usEngUnit;
            AIMeasResult.fAI_xx[index]  = xAdcVal.fMeasure;
         //   usleep(100);
        }
        if (SDCCP_CHECK_TIME_OUT(begin, 100)) {
			break;
		}
    }while(1);
        

    for(ADC_CHANNEL_E chan = ADC_CHANNEL_0; chan <= ADC_CHANNEL_7; chan++){
        vGetAdcValueCal(chan, Range_0_20MA, Range_4_20MA, 0, 0, factor, &xAdcVal);
        int index = chan_tbl[chan];
        AIEngUnitResult.fAI_xx[index] = (float)xAdcVal.usEngUnit;
        AIMeasResult.fAI_xx[index]  = xAdcVal.fMeasure;
    }


	
	//vGetAdcTestValueInfo(chan, Range_0_20MA, Range_4_20MA, 0, 100, factor, &xAdcVal);
	xAdcVal.usEngUnit =   AIEngUnitResult.fAI_xx[request.Type];
    xAdcVal.fMeasure =    AIMeasResult.fAI_xx[request.Type];

	s_CalEntry_t CalEntry;
	memset(&CalEntry,0,sizeof(s_CalEntry_t));
    vGetCalValue(request.Type, &CalEntry);
	
	response.xAdcInfo.usChannel = CalEntry.xMin.usAdcValue;   		      //表示当前通道校准的最小值
	response.xAdcInfo.usRange =  CalEntry.xMiddle.usAdcValue;    	      //表示当前通道校准的中间值
	response.xAdcInfo.usEngVal = CalEntry.xMax.usAdcValue; 				  //表示当前通道校准的最大值
	response.xAdcInfo.usMeasureVal = (mdUINT32)(xAdcVal.fMeasure * 1000); //当前通道测量的实测值

	response.RetCode = MSG_TEST_ERROR_OK;	
	send_adc_get_response(msg, &response);*/
}

/*
enum{
	CHECK_VAL_0_MA = 0x00,
	CHECK_VAL_10_MA,
	CHECK_VAL_20_MA,
};
*/
void vTestSetCheckDefault(const S_MSG *msg, byte_buffer_t *bb)
{
	/*S_MSG_TEST_NET_RESPONSE response;
	
	response.Type = 0;
	response.RetCode = MSG_TEST_ERROR_ERR;
	response.Value = 0;

	SetAdcCalCfgDefault();*/
	//SaveAdcCalInfoToFs();
	//send_base_response(msg, &response);
}

//#define ADC_CAL_RANG (1000)

//extern const s_CalEntry_t gDefaultCalEntry[ADC_CHANNEL_NUM];
void vTestSetAdc(const S_MSG *msg, byte_buffer_t *bb)
{

	/*S_MSG_SET_TEST_SET_ADC_REQUEST  request;
	S_MSG_TEST_SET_ADC_RESPONSE response;
	
	request.xAdcInfo.usChannel =   bb_get_int(bb);
	request.xAdcInfo.usRange =     bb_get_int(bb);
	request.xAdcInfo.usEngVal =    bb_get_int(bb);
	request.xAdcInfo.usMeasureVal = bb_get_int(bb);
	
	s_CalEntry_t  xCalEntry;
	ADC_CHANNEL_E chann = request.xAdcInfo.usChannel;
	eCalCountType_t eCalType;


	if(request.xAdcInfo.usRange == CHECK_VAL_0_MA){
		xCalEntry.xMin.fMeterValue = 4.0f;
		eCalType = SET_MIN;
		xCalEntry.xMin.usAdcValue = request.xAdcInfo.usEngVal;
	}else if(request.xAdcInfo.usRange == CHECK_VAL_10_MA){
		xCalEntry.xMiddle.fMeterValue = 10.0f;
		eCalType = SET_MIDDLE;
		xCalEntry.xMiddle.usAdcValue = request.xAdcInfo.usEngVal;
	}else if(request.xAdcInfo.usRange == CHECK_VAL_20_MA){
		xCalEntry.xMax.fMeterValue = 18.0f;
		eCalType = SET_MAX;
		xCalEntry.xMax.usAdcValue = request.xAdcInfo.usEngVal;
	}

	vSetCalValue(chann, eCalType,&xCalEntry);
    
	response.Type = 0;
	response.Value = 0;
	response.RetCode = MSG_TEST_ERROR_OK;


	if(request.xAdcInfo.usRange == CHECK_VAL_20_MA && chann == ADC_CHANNEL_7){
		mdBOOL set_ok = mdTRUE;
		for(int i = 0 ; i < 8 ;i++){
			if((gCalEntryBak[i].xMin.usAdcValue <= (gDefaultCalEntry[i].xMin.usAdcValue - ADC_CAL_RANG)) || (gCalEntryBak[i].xMin.usAdcValue >= (gDefaultCalEntry[i].xMin.usAdcValue + ADC_CAL_RANG))){
				set_ok = mdFALSE;
				break;
			}
			if((gCalEntryBak[i].xMiddle.usAdcValue <= (gDefaultCalEntry[i].xMiddle.usAdcValue - ADC_CAL_RANG)) || (gCalEntryBak[i].xMiddle.usAdcValue >= (gDefaultCalEntry[i].xMiddle.usAdcValue + ADC_CAL_RANG))){
				set_ok = mdFALSE;
				break;
			}
			if((gCalEntryBak[i].xMax.usAdcValue <= (gDefaultCalEntry[i].xMax.usAdcValue - ADC_CAL_RANG)) || (gCalEntryBak[i].xMax.usAdcValue >= (gDefaultCalEntry[i].xMax.usAdcValue + ADC_CAL_RANG))){
				set_ok = mdFALSE;
			}
		}
		if(set_ok != mdTRUE){
			response.RetCode = MSG_TEST_ERROR_ERR;
		}else {
			SaveAdcCalInfoToFs();
		}
	}
	
	send_base_response(msg, &response);*/
}



//按顺序依次是 UART4 UART5 UART0
void vTestUart(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_TEST_UART_RESPONSE response;
	S_MSG_TEST_UART_REQUEST  request;

	request.Type = bb_get(bb);
	
	response.Type =  request.Type;
	response.Value = 0;
	response.RetCode = MSG_TEST_ERROR_ERR;

	short rtype = (request.Type&0x0f);
	short cType = ((request.Type>>4)&0x0f);

   //vQueueClear( g_xTestUartInfo[rtype].queue);
   //printf("vTestUart\n");

	switch(rtype){
		case UART_GET_0:{
			if(cType == UART_RECIVE){
				byte buf[256] = {0};
				int index = 0;
				if((index = ReciveUartData(HW_UART0, buf, sizeof(buf), 200, 200, 500)) > 0 ){
                    int i;
					//printf("recive data:%d\n",index);
					for(i = 0; i < 20;i++){
                       				// printf("%02x ",buf[i]);
						if(buf[i] != i) break;
					}
                   			 printf("\n");

					if(i >= 20){
						response.RetCode = MSG_TEST_ERROR_OK;
					}
				}
else {
					printf("recive data error\n");
				}
				send_base_response(msg, &response);
				
				vQueueClear(g_xTestUartInfo[HW_UART0].queue);
			
				
			}else if(cType == UART_SEND){
				char buf[20] = {0};
				for(int i = 0; i < 20; i++){
					buf[i] = i;
				}
				vUartSend(HW_UART0, buf, 20);
                rt_thread_delay( 100 );
				send_base_response(msg, &response);
			}
			break;
		}
		case UART_GET_1:{
			if(cType == UART_RECIVE){
				byte buf[256] = {0};
				int index = 0;
				if((index = ReciveUartData(HW_UART1, buf, sizeof(buf), 200, 200, 500)) > 0){
					//printf("recive data:%d\n",index);
                    int i;
					for( i = 0; i < 20; i++){
                        //printf("%02x ",buf[i]);
						if(buf[i] != i) break;
					}
                   // printf("\n");
					if(i >= 20){
						response.RetCode = MSG_TEST_ERROR_OK;
					}
				}
else {
					printf("recive data error\n");
				}
				send_base_response(msg, &response);
				vQueueClear(g_xTestUartInfo[HW_UART1].queue);
			}else if(cType == UART_SEND){
				/*for(int i = 0; i < 20; i++){
					vUartSend(HW_UART1, (char*)&i, 1);
				}
*/
				char buf[20] = {0};
				for(int i = 0; i < 20; i++){
					buf[i] = i;
				}
				vUartSend(HW_UART1, buf, 20);
                		rt_thread_delay( 100 );
				send_base_response(msg, &response);
			}
			break;
		}
		/*case UART_GET_2:{
			if(cType == UART_RECIVE){
				byte buf[256] = {0};
				if(ReciveUartData(HW_UART2, buf, sizeof(buf), 200, 200, 500) > 0){
                    int i;
					for( i = 0; i < 20;i++){
						if(buf[i] != i) break;
					}
					if(i >= 20){
						response.RetCode = MSG_TEST_ERROR_OK;
					}
				}
				send_base_response(msg, &response);
				vQueueClear(g_xTestUartInfo[HW_UART2].queue);
			}else if(cType == UART_SEND){
				for(int i = 0; i < 20; i++){
					vUartSend(HW_UART2, (char*)&i, 1);
				}
                                rt_thread_delay( 100 );
				send_base_response(msg, &response);
			}
			break;
		}*/
		/*
        case UART_GET_3:{
			if(cType == UART_RECIVE){
				byte buf[256] = {0};
				if(ReciveUartData(HW_UART3, buf, sizeof(buf), 200, 200, 500) > 0){
                    int i;
					for( i = 0; i < 20;i++){
						if(buf[i] != i) break;
					}
					if(i >= 20){
						response.RetCode = MSG_TEST_ERROR_OK;
					}
				}
				send_base_response(msg, &response);
				vQueueClear(g_xTestUartInfo[HW_UART3].queue);
			}else if(cType == UART_SEND){
				for(int i = 0; i < 20; i++){
					vUartSend(HW_UART3, (char*)&i, 1);
				}
                rt_thread_delay( 100 );
				send_base_response(msg, &response);
			}
			break;
		}*/
	}

	
	//send_base_response(msg, &response);
	
}




void vTestGetCalValue(const S_MSG *msg, byte_buffer_t *bb)
{
	/*S_MSG_TEST_RTC_RESPONSE response;
	
	response.Type = 0;
	response.RetCode = MSG_TEST_ERROR_OK;
	response.Value = 0;

	vAdcCalCfgInit();

	int i = 0;
	for(i = 0 ; i< 8; i++){
		if(gCalEntry[i].xMax.usAdcValue == gDefaultCalEntry[i].xMax.usAdcValue){
			response.RetCode = MSG_TEST_ERROR_ERR;
			break;
		}
	}
	
	send_base_response(msg, &response);*/
}


void vTestRelays(const S_MSG *msg, byte_buffer_t *bb)
{
	/*S_MSG_TEST_TTL_OUTPUT_RELAY_RESPONSE response;
	S_MSG_TEST_TTL_OUTPUT_RELAY_REQUEST  request;

	request.Type = bb_get(bb);

	if(request.Type == 1){
		bRelaysBegain = mdTRUE;
	}else {
		bRelaysBegain = mdFALSE;
	}

	response.RetCode = MSG_TEST_ERROR_OK;
	send_base_response(msg, &response);*/
}




