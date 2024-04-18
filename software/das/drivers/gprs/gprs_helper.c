#include <board.h>
#include <stdio.h>
#include <time.h>
#include "mdtypedef.h"

#define DM_GPRS_CTL     _IO('G',100)
#define GPRS_RESET      0
#define GPRS_POWER      1
#define GPRS_TERM_ON    2

static pthread_mutex_t gprs_mutex;

rt_bool_t bIsGprsInitOk = RT_FALSE;

GPRS_NState_t g_xGPRS_NState;
GPRS_COPN_t g_xGPRS_COPN;
UGPRS_SMOND_t g_xGPRS_SMOND;
GPRS_COPS_t g_xGPRS_COPS;
GPRS_SEND_ACK_t g_xGPRS_SACK;

rt_base_t g_nGPRS_CSQ = UINT32_MAX;
rt_base_t g_nGPRS_CREG = UINT32_MAX;
rt_base_t g_nGPRS_MUX = -1;

static serial_t *s_gprs_serial = RT_NULL;
static rt_thread_t s_gprs_parse_thread = NULL;
static rt_bool_t _gprs_net_time_flag = RT_FALSE;

typedef struct {
    const char *szCode;
    rt_uint8_t const btLen;
} GPRSCode_t;

typedef struct {
    const char *szEvt;
    rt_uint8_t const btLen;
} GPRSEvt_t;

static const GPRSCode_t c_xGPRSCodeList[GPRS_CODE_MAX] = {
    [GPRS_CODE_OK]                  = { "OK", 2 },
    [GPRS_CODE_ERROR]               = { "ERROR", 5 },
};

// 这个值根据下面的内容调整
#define GPRS_EVT_SZ_MIN     (2)
static const GPRSEvt_t c_xGPRS_EvtList[GPRS_EVT_MAX] = {
    [GPRS_EVT_COPS]         = { "+COPS: ", 7 },
    [GPRS_EVT_CSQ]          = { "+CSQ: ", 6 },
    [GPRS_EVT_CREG]         = { "+CGREG: ", 8},
    [GPRS_EVT_MUX]         =  { "+QIMUX: ", 8},
    [GPRS_EVT_NWTIME]       = { "+CCLK: ", 7 },

    [GPRS_EVT_SEDN_ACK]    = {"+QISACK: ",9},

};

#define RX_BUFFER_SIZE      (1400)

static rt_uint8_t s_btRxBuffer[RX_BUFFER_SIZE];

static rt_bool_t s_bRcvCode[GPRS_CODE_MAX];

static rt_bool_t s_bRcvEvt[GPRS_EVT_MAX];
static rt_bool_t s_bIsEvtParse[GPRS_EVT_MAX];
static rt_uint32_t s_ulRcvEvtTick[GPRS_EVT_MAX];

static rt_base_t s_nRxOffset = 0;

static void prvClearRcvBuffer(void);
static void prvClearRcvCode(void);
static void prvClearRcvEvt(void);
static rt_bool_t prvIsRcvAnyEvt(void);
static rt_bool_t prvIsInParseAnyEvt(void);
static void prvParseEvt(void);
static void prvParseCOPS(void);
static void prvParseSMOND(void);
static void prvParseCSQ(void);
static void prvParseCREG(void);
static void prvParseMux(void);
static void prvParseSISO(void);
static void prvParseSendAck(void);

static void prvParseNWTIME(void);

static void prvParseCode(void);

static rt_bool_t prvWaitEvt(eGPRS_Evt eEvt, rt_bool_t bCheckErr, rt_base_t nMs);
static rt_bool_t prvWaitResult(rt_base_t nMs);

static void prvSendCmd(const char *szCmd);
static void prvGPRS_Reset(void);

static int __gprs_power_ctrl(int val)
{
    s_io_t iodata = {
    	.gpio = GPRS_POWER,
    	.dir = 1,
    	.val = val,
    };
    das_do_io_ctrl(DM_GPRS_CTL, &iodata);
    return 0;
}

static int __gprs_reset_ctrl(int val)
{
    s_io_t iodata = {
    	.gpio = GPRS_RESET,
    	.dir = 1,
    	.val = val,
    };
    das_do_io_ctrl(DM_GPRS_CTL, &iodata);
    return 0;
}

static int __gprs_term_ctrl(int val)
{
    s_io_t iodata = {
    	.gpio = GPRS_TERM_ON,
    	.dir = 1,
    	.val = val,
    };
    das_do_io_ctrl(DM_GPRS_CTL, &iodata);
    return 0;
}

// AT 指令适当延时
#define GPRS_AT_DELAY()         rt_thread_delay( RT_TICK_PER_SECOND / 10 )

static void __gprsparse_thread(void *parameter)
{
    rt_uint32_t xRcvEvt = 0;
    rt_uint8_t buffer[1024];

    while (!das_check_process("pppd")) {
        rt_thddog_suspend("serial_helper_select");
        int s_rc = serial_helper_select(BOARD_GPRS_UART, -1);
        int nRead = serial_helper_recv(BOARD_GPRS_UART, (void *)buffer, sizeof(buffer));
        rt_thddog_resume();
        if (nRead <= 0) break;
        /*for(int i = 0 ; i < nRead;i++){
            printf("%c",buffer[i]);
        }
        printf("\n");*/
        for (int i = 0; i < nRead; i++) {
            s_btRxBuffer[s_nRxOffset++] = buffer[i];
            if (s_nRxOffset >= RX_BUFFER_SIZE) {
                s_nRxOffset = 0;
            }

            if (!prvIsRcvAnyEvt() && s_nRxOffset >= GPRS_EVT_SZ_MIN) {
                prvParseEvt();
            }


            if (s_bRcvEvt[GPRS_EVT_COPS] && !s_bIsEvtParse[GPRS_EVT_COPS]) {
                prvParseCOPS();
            }


            if (s_bRcvEvt[GPRS_EVT_CSQ] && !s_bIsEvtParse[GPRS_EVT_CSQ]) {
                prvParseCSQ();
            }

            if (s_bRcvEvt[GPRS_EVT_CREG] && !s_bIsEvtParse[GPRS_EVT_CREG]) {
                prvParseCREG();
            }

            if (s_bRcvEvt[GPRS_EVT_SEDN_ACK] && !s_bIsEvtParse[GPRS_EVT_SEDN_ACK]) {
                prvParseSendAck();
            }

            if (s_bRcvEvt[GPRS_EVT_MUX] && !s_bIsEvtParse[GPRS_EVT_MUX]) {
                prvParseMux();
            }

            if (s_bRcvEvt[GPRS_EVT_NWTIME] && !s_bIsEvtParse[GPRS_EVT_NWTIME]) {
                prvParseNWTIME();
            }

            if (!prvIsRcvAnyEvt()) {

                // 匹配到\r\n
                if (s_nRxOffset > 1 && '\r' == s_btRxBuffer[s_nRxOffset - 2] && '\n' == s_btRxBuffer[s_nRxOffset - 1]) {
                    if (s_nRxOffset >= 2) {
                        prvParseCode();
                        s_nRxOffset = 0;
                    }
                }
            }
        }
    }
    rt_thddog_exit();
}

static rt_bool_t prvIsRcvAnyEvt(void)
{
    for (int i = 0; i < GPRS_EVT_MAX; i++) {
        if (s_bRcvEvt[i]) {
            return RT_TRUE;
        }
    }
    return RT_FALSE;
}

static rt_bool_t prvIsInParseAnyEvt(void)
{
    for (int i = 0; i < GPRS_EVT_MAX; i++) {
        if (s_bIsEvtParse[i]) {
            return RT_TRUE;
        }
    }
    return RT_FALSE;
}


static void prvParseEvt(void)
{
    for (int i = 0; i < GPRS_EVT_MAX; i++) {
        rt_uint8_t btLen = c_xGPRS_EvtList[i].btLen;

        if(btLen == s_nRxOffset){
            int Ret = strncmp((char const *)s_btRxBuffer, c_xGPRS_EvtList[i].szEvt, btLen);
            if(Ret == 0) {
                if (!s_bRcvEvt[i]) {
                    s_bRcvEvt[i] = RT_TRUE;
                    s_bIsEvtParse[i] = RT_FALSE;
                }
                s_ulRcvEvtTick[i] = rt_tick_get();
                break;
            }
        }
    }
}

static void prvClearEvt(rt_uint8_t btCode)
{
    if (btCode < GPRS_EVT_MAX) {
        s_bRcvEvt[btCode] = RT_FALSE;
        s_bIsEvtParse[btCode] = RT_FALSE;
    }
}

static rt_bool_t prvParseTimeout(int index, int ms)
{
    if (rt_tick_get() - s_ulRcvEvtTick[index] > rt_tick_from_millisecond(ms)) {
        s_bRcvEvt[index] = RT_FALSE;
        s_bIsEvtParse[index] = RT_FALSE;
        s_nRxOffset = 0;
        return RT_TRUE;
    }
    return RT_FALSE;
}

static void prvParseNWTIME(void)
{
    rt_base_t year=0, mon=0, day=0, hour=0, min=0, sec=0, tz=0, dst=0;
    if (s_nRxOffset > c_xGPRS_EvtList[GPRS_EVT_NWTIME].btLen &&
        '\r' == s_btRxBuffer[s_nRxOffset - 2] && '\n' == s_btRxBuffer[s_nRxOffset - 1]) {
        s_btRxBuffer[s_nRxOffset] = '\0';
        if( sscanf((const char *)s_btRxBuffer, "^NWTIME: \"%d/%d/%d,%d:%d:%d+%d,%d\"\r\n", \
                &year, &mon, &day, &hour, &min, &sec, &tz, &dst) >= 8) 
        {
            rt_kprintf("GPRS NetTime:%04d/%02d/%02d,%02d:%02d:%02d+%d,%d", 
                2000+year, mon, day, hour, min, sec, tz, dst);
            {
                struct tm tm = {
                    .tm_year = year + 100, 
                    .tm_mon = mon - 1, 
                    .tm_mday = day, 
                    .tm_hour = hour, 
                    .tm_min = min, 
                    .tm_sec = sec, 
                    .tm_isdst = dst
                };
                das_set_time((uint32_t)mktime(&tm), tz);
                my_system("hwclock -w -u");
            }
            //set_date(2000+year, mon, day);
            //set_time(hour, min, sec);
            _gprs_net_time_flag = RT_TRUE;
        } else if( sscanf((const char *)s_btRxBuffer, "^NWTIME: \"%d/%d/%d,%d:%d:%d-%d,%d\"\r\n", \
                &year, &mon, &day, &hour, &min, &sec, &tz, &dst) >= 8) 
        {
            rt_kprintf("GPRS NetTime:%04d/%02d/%02d,%02d:%02d:%02d-%d,%d", 
                2000+year, mon, day, hour, min, sec, tz, dst);
            {
                struct tm tm = {
                    .tm_year = year + 100, 
                    .tm_mon = mon - 1, 
                    .tm_mday = day, 
                    .tm_hour = hour, 
                    .tm_min = min, 
                    .tm_sec = sec, 
                    .tm_isdst = dst
                };
                das_set_time((uint32_t)mktime(&tm), -tz);
                my_system("hwclock -w -u");
            }
            _gprs_net_time_flag = RT_TRUE;
        }
        //rt_kprintf( "%s\n", g_xGPRS_COPN.alphan );
        s_bRcvEvt[GPRS_EVT_NWTIME] = RT_FALSE;
        s_bIsEvtParse[GPRS_EVT_NWTIME] = RT_TRUE;
        s_nRxOffset = 0;
        return;
    } else {
        prvParseTimeout(GPRS_EVT_NWTIME, 200);
    }
}


static void prvParseSendAck(void)
{
    if (s_nRxOffset > c_xGPRS_EvtList[GPRS_EVT_SEDN_ACK].btLen &&
        '\r' == s_btRxBuffer[s_nRxOffset - 2] && '\n' == s_btRxBuffer[s_nRxOffset - 1]) {
        s_btRxBuffer[s_nRxOffset] = '\0';

		memset(&g_xGPRS_SACK,0,sizeof(g_xGPRS_SACK));
		sscanf(s_btRxBuffer, "+QISACK: %d,%d,%d", &g_xGPRS_SACK.sent,&g_xGPRS_SACK.acked,&g_xGPRS_SACK.nAcked);
	
        s_bRcvEvt[GPRS_EVT_SEDN_ACK] = RT_FALSE;
        s_bIsEvtParse[GPRS_EVT_SEDN_ACK] = RT_TRUE;
        s_nRxOffset = 0;
        return;
    } else {
        prvParseTimeout(GPRS_EVT_SEDN_ACK, 200);
    }
}


static void prvParseCOPS(void)
{
    if (s_nRxOffset > c_xGPRS_EvtList[GPRS_EVT_COPS].btLen &&
        '\r' == s_btRxBuffer[s_nRxOffset - 2] && '\n' == s_btRxBuffer[s_nRxOffset - 1]) {
        s_btRxBuffer[s_nRxOffset] = '\0';

		
        g_xGPRS_COPS.nCount = 1;
        memset(g_xGPRS_COPS.xCOPS[g_xGPRS_COPS.nCount].salphan, 0, 16);
		int j = 0;
        int flag = 0;
		for(int i = 0 ; i < strlen(s_btRxBuffer) && j < 16;i++){
			if(s_btRxBuffer[i] != '"' && s_btRxBuffer[i-1] == '"'){
				//g_xGPRS_COPS.xCOPS[g_xGPRS_COPS.nCount].salphan[j] = s_btRxBuffer[i];
                flag = 1;
			}
           
			if(s_btRxBuffer[i] == '"' && s_btRxBuffer[i-1] != '"' && flag == 1){
				break;
			}
             if(flag == 1){
              g_xGPRS_COPS.xCOPS[0].alphan[j++] = s_btRxBuffer[i];
            }
		}
        s_bRcvEvt[GPRS_EVT_COPS] = RT_FALSE;
        s_bIsEvtParse[GPRS_EVT_COPS] = RT_TRUE;
        s_nRxOffset = 0;
        return;
    } else {
        prvParseTimeout(GPRS_EVT_COPS, 200);
    }
}



static void prvParseCSQ(void)
{
    rt_base_t tmp;

    if (s_nRxOffset > c_xGPRS_EvtList[GPRS_EVT_CSQ].btLen &&
        '\r' == s_btRxBuffer[s_nRxOffset - 2] && '\n' == s_btRxBuffer[s_nRxOffset - 1]) {
        s_btRxBuffer[s_nRxOffset] = '\0';
        sscanf((const char *)s_btRxBuffer, "+CSQ: %d,%d\r\n", &g_nGPRS_CSQ, &tmp);
        s_bRcvEvt[GPRS_EVT_CSQ] = RT_FALSE;
        s_bIsEvtParse[GPRS_EVT_CSQ] = RT_TRUE;
        s_nRxOffset = 0;
        return;
    } else {
        prvParseTimeout(GPRS_EVT_CSQ, 200);
    }
}

static void prvParseCREG(void)
{
    rt_base_t tmp;

    if (s_nRxOffset > c_xGPRS_EvtList[GPRS_EVT_CREG].btLen &&
        '\r' == s_btRxBuffer[s_nRxOffset - 2] && '\n' == s_btRxBuffer[s_nRxOffset - 1]) {
        s_btRxBuffer[s_nRxOffset] = '\0';
        sscanf((const char *)s_btRxBuffer, "+CGREG: %d,%d\r\n", &tmp, &g_nGPRS_CREG);
        s_bRcvEvt[GPRS_EVT_CREG] = RT_FALSE;
        s_bIsEvtParse[GPRS_EVT_CREG] =  RT_TRUE;
        s_nRxOffset = 0;
        return;
    } else {
        prvParseTimeout(GPRS_EVT_CREG, 200);
    }
}


static void prvParseMux(void)
{
    rt_base_t tmp;

    if (s_nRxOffset > c_xGPRS_EvtList[GPRS_EVT_MUX].btLen &&
        '\r' == s_btRxBuffer[s_nRxOffset - 2] && '\n' == s_btRxBuffer[s_nRxOffset - 1]) {
        s_btRxBuffer[s_nRxOffset] = '\0';
        sscanf((const char *)s_btRxBuffer, "+QIMUX: %d\r\n", &g_nGPRS_MUX);
        s_bRcvEvt[GPRS_EVT_MUX] = RT_FALSE;
        s_bIsEvtParse[GPRS_EVT_MUX] =  RT_TRUE;
        s_nRxOffset = 0;
        return;
    } else {
        prvParseTimeout(GPRS_EVT_MUX, 200);
    }
}

static void prvParseCode(void)
{
    rt_base_t nIndex = s_nRxOffset - 2;

    for (int i = 0; i < GPRS_CODE_MAX; i++) {
        rt_uint8_t btLen = c_xGPRSCodeList[i].btLen;

        if (nIndex >= btLen &&
            0 == strncmp(
                         (char const *)&s_btRxBuffer[nIndex - btLen],
                         c_xGPRSCodeList[i].szCode,
                         btLen)
            ) {
            s_bRcvCode[i] = RT_TRUE;
            break;
        }
    }
}

static void prvClearRcvBuffer(void)
{
    s_nRxOffset = 0;
}

static void prvClearRcvCode(void)
{
    for (int i = 0; i < GPRS_CODE_MAX; i++) {
        s_bRcvCode[i] = RT_FALSE;
    }
}

static void prvClearRcvEvt(void)
{
    for (int i = 0; i < GPRS_EVT_MAX; i++) {
        s_bRcvEvt[i] = RT_FALSE;
        s_bIsEvtParse[i] = RT_FALSE;
        s_ulRcvEvtTick[i] = rt_tick_get();
    }
}

void prvSendBuffer(const void *pBuffer, rt_size_t uSize)
{
    prvClearRcvCode();
    serial_helper_send(BOARD_GPRS_UART, (const void *)pBuffer, uSize);
}

void prvSendCmd(const char *szCmd)
{
    prvClearRcvCode();
    serial_helper_send(BOARD_GPRS_UART, (const void *)szCmd, strlen(szCmd));
}

void vInitGPRSStatus(void)
{
    prvClearRcvCode();
    prvClearRcvEvt();
    prvClearRcvBuffer();
}

static int wait_cnt = 0;

static int __gprs_wait_pppd_up(int timeout_sec)
{
    while (timeout_sec--) {
        rt_thread_delay(1000);
        if (das_do_is_gprs_up()) break;
        if(wait_cnt++ >= 5){
            wait_cnt = 0;
            rt_kprintf("#### wait ppp0 up\r\n");
        }
    }
    return das_do_is_gprs_up();
}


rt_err_t gprs_do_init(void)
{
    //my_system("killall pppd");
    
    if (rt_mutex_init(&gprs_mutex, "gprs_mutex", RT_IPC_FLAG_FIFO) != RT_EOK) {
        rt_kprintf("init gprs_mutex failed\n");

        return -RT_ERROR;
    }

#if !TEST_ON_PC
    if(g_gprs_work_cfg.eWMode != GPRS_WM_SHUTDOWN){
        //gprs_do_reinit(1);
        __gprs_wait_pppd_up(-1);
    }
#endif
    
    if (g_gprs_work_cfg.eWMode != GPRS_WM_SHUTDOWN) {
        bIsGprsInitOk = mdTRUE;
        xfer_helper_gprs_init();
        //拨号成功后重启一下狗洞

       // my_system("/usr/fs/etc/init.d/S96dtunnel.sh restart");
        
        for (int n = BOARD_ENET_TCPIP_NUM; n < BOARD_TCPIP_MAX; n++) {
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
            if (g_tcpip_cfgs[n].enable &&
                TCP_IP_M_NORMAL == g_tcpip_cfgs[n].mode &&
                TCP_IP_TCP == g_tcpip_cfgs[n].tcpip_type &&
                TCPIP_CLIENT == g_tcpip_cfgs[n].tcpip_cs &&
                (
                    PROTO_CC_BJDC == g_tcpip_cfgs[n].cfg.normal.proto_type ||
                    PROTO_HJT212 == g_tcpip_cfgs[n].cfg.normal.proto_type ||
                    PROTO_DM101 == g_tcpip_cfgs[n].cfg.normal.proto_type ||
                    PROTO_MQTT == g_tcpip_cfgs[n].cfg.normal.proto_type
                )) {
                cc_net_open(n);
            }
        }
    }

    return -RT_EOK;
}



rt_err_t gprs_do_reinit(int shutdown_if_init_error)
{

    int start_cnt = 5;
    rt_bool_t bAllInitOk = RT_FALSE;
    rt_bool_t bHasGPRSFlag = RT_FALSE;
    
_start:
    my_system("killall pppd");
    __gprs_power_ctrl(0);
    __gprs_term_ctrl(0);
    __gprs_reset_ctrl(0);
     vGPRS_PowerUp();
     //rt_thread_delay(1 * RT_TICK_PER_SECOND);

     g_xCellNetType = vCheckCellNetType();

 //打开GPRS电源，判断当前接的是哪种模块
   if( (g_xCellNetType == E_4G_EC20) || (g_xCellNetType == E_4G_EC200S) || (g_xCellNetType == E_AIR720) ){
        rt_kprintf("检测到4G模块\r\n");
        if(access("/dev/ttyUSB2", R_OK) != 0){
            rt_kprintf("未检测到ttyUSB2节点\r\n");
            vGPRS_PowerDown();
            rt_thread_delay(RT_TICK_PER_SECOND / 10);
            vGPRS_PowerUp();
            rt_thread_delay(RT_TICK_PER_SECOND / 10);
             __gprs_term_ctrl(1);
            rt_thread_delay(2.0 * RT_TICK_PER_SECOND);
            __gprs_term_ctrl(0);
            rt_kprintf("GPRS wait 30s check network type!\r\n");
            for(int i = 0 ; i < 30; i++){
                rt_thread_delay(1 * RT_TICK_PER_SECOND);
                if(access("/dev/ttyUSB2", R_OK) == 0){
                    rt_thread_delay(2 * RT_TICK_PER_SECOND);
		            printf("检测到了ttyusb2\n");
                    break;
                }
            }
        }
   }else {
        rt_thread_delay(2 * RT_TICK_PER_SECOND); 
   }

   if(g_xCellNetType == E_4G_EC20){
     rt_kprintf("\n============= 4G MODULE EC20 IS CHECK!\n");
   } else if(g_xCellNetType == E_GPRS_M26){
     rt_kprintf("\n============= 2G MODULE OR NO MODULE M26 IS CHECK!\n");
   } else if(g_xCellNetType == E_NBIOT_BC26){
     rt_kprintf("\n============= NB-IOT MODULE BC95 IS CHECK!\n");
   }else if(g_xCellNetType == E_AIR720){
     rt_kprintf("\n合宙AIR720\n");
   }

   if( (g_xCellNetType == E_4G_EC20) || (g_xCellNetType == E_AIR720) || (g_xCellNetType == E_4G_EC200S) ){
        if(access("/dev/ttyUSB2", R_OK) != 0){
            if(start_cnt-- <= 0){
                g_gprs_work_cfg.eWMode = GPRS_WM_SHUTDOWN;
                bIsGprsInitOk = RT_FALSE;
                rt_kprintf("多次检测，未识别到4G模块，直接关机\r\n");
                return 0;
            }
            rt_kprintf("未检测到ttyUSB2节点,复位一下模块\r\n");
            vGPRS_HWRest();
            goto _start;
        }else {
            rt_kprintf("检测到ttyUSB2节点\r\n");
        }
   }

   int retry = 3;
   
_retry:

    serial_helper_open(BOARD_GPRS_UART);
    s_gprs_serial = g_serials[BOARD_GPRS_UART];
    s_gprs_parse_thread = rt_thread_create("gprsparse", __gprsparse_thread, RT_NULL, 0x400, 20, 20);
    if (s_gprs_parse_thread) {
        rt_thddog_register(s_gprs_parse_thread, 60);
        rt_thread_startup(s_gprs_parse_thread);
    }
    
    bAllInitOk = RT_FALSE;
    bIsGprsInitOk = mdFALSE;

    if( (g_xCellNetType == E_4G_EC20) || (g_xCellNetType == E_4G_EC200S) || (g_xCellNetType == E_AIR720) ){
         bHasGPRSFlag = RT_TRUE;   
    }


    while (!bAllInitOk) {
        rt_thddog_feed("");
        
        if(g_gprs_work_cfg.eWMode == GPRS_WM_SHUTDOWN){
            rt_kprintf("1111 vGPRS_PowerDown\r\n");
            vGPRS_PowerDown();
        }
        
        if (g_gprs_work_cfg.eWMode != GPRS_WM_SHUTDOWN) {

            rt_thddog_feed("");

            int at_cnt = 3;
            while(at_cnt--){
               if(bGPRS_ATTest()) break; 
               rt_thread_delay(RT_TICK_PER_SECOND);
            }

            if(!bGPRS_ATTest()    ) {
                    rt_kprintf("复位网络模块\r\n");
                    if (g_xCellNetType == E_GPRS_M26) {
                        rt_kprintf("重启2G模块\r\n");
                        vGPRS_PowerDown();
                        rt_thread_delay(RT_TICK_PER_SECOND / 10);
                        vGPRS_PowerUp();
                        rt_thread_delay(RT_TICK_PER_SECOND / 10);
                        vGPRS_TermUp();
                    } else if ( (g_xCellNetType == E_4G_EC20) || (g_xCellNetType == E_4G_EC200S) || (g_xCellNetType == E_AIR720) ) {
                
                       // rt_kprintf("未检测到ttyUSB3节点\r\n");
                        rt_kprintf("重启4G模块,vGPRS_HWRest\r\n"); 
                        vGPRS_HWRest();
                       /* vGPRS_PowerDown();
                        rt_thread_delay(RT_TICK_PER_SECOND / 10);
                        vGPRS_PowerUp();
                        rt_thread_delay(RT_TICK_PER_SECOND / 10);
                         __gprs_term_ctrl(1);
                        rt_thread_delay(2.0 * RT_TICK_PER_SECOND);
                        __gprs_term_ctrl(0);*/
                        rt_kprintf("GPRS wait 30s check network type!\r\n");
                        rt_thread_delete(s_gprs_parse_thread);
                        s_gprs_parse_thread = NULL;
                        serial_helper_close(BOARD_GPRS_UART);
                        s_gprs_serial = RT_NULL;
                        for(int i = 0 ; i < 30; i++){
                            rt_thread_delay(1 * RT_TICK_PER_SECOND);
                            if(access("/dev/ttyUSB2", R_OK) == 0){
                                rt_thread_delay(2 * RT_TICK_PER_SECOND);
                                break;
                            }
                        }
                        if(!bHasGPRSFlag && --retry <= 0) {
                           // g_gprs_work_cfg.eWMode = GPRS_WM_SHUTDOWN;
                            bIsGprsInitOk = RT_FALSE;
                            elog_w("init_gprs", "GPRS init err 3 times");
                            rt_kprintf("GPRS INIT err 3 times,复位一下模块 \r\n");
                             vGPRS_HWRest();
                             goto _start;
                        }
                        goto _retry;
                 
                } else if (g_xCellNetType == E_NBIOT_BC26) {
                    rt_kprintf("重启NB模块\r\n"); 
                }
            }
            
            if(bGPRS_ATTest()) {
                bHasGPRSFlag = RT_TRUE;
                //bGPRS_TcpOt();
                rt_kprintf("wait 1 sec before getcsq\n");
				//bGPRS_ATReset();
                rt_thread_delay(1 * RT_TICK_PER_SECOND);
                bGPRS_GetCSQ(RT_TRUE);
                rt_kprintf("CSQ = %d\n", g_nGPRS_CSQ);

                rt_uint8_t btCREG = 0xFF;
                bGPRS_GetCREG(RT_TRUE);
                rt_kprintf("CREG = %d\n", g_nGPRS_CREG);

                bAllInitOk = bGPRS_NetInit(g_gprs_net_cfg.szAPN, g_gprs_net_cfg.szAPNNo, g_gprs_net_cfg.szUser, g_gprs_net_cfg.szPsk);
                rt_kprintf("gprs net init = %d\n", bAllInitOk);

                if (bAllInitOk && (g_xCellNetType != E_4G_EC20) && (g_xCellNetType != E_4G_EC200S) ) {
                    bGPRS_SetCSCA(g_gprs_net_cfg.szMsgNo);
                    break;
                }
            }
        } else {
            rt_kprintf("gprs cfg shutdown!\n");
            break;
        }
        
        if(!bHasGPRSFlag && --retry <= 0) {
            bIsGprsInitOk = RT_FALSE;
            elog_w("init_gprs", "GPRS init err 3 times");
            rt_kprintf("GPRS INIT err 3 times,复位一下模块 \r\n");
            vGPRS_HWRest();
            goto _start;
        }
    }
    
    if (g_gprs_work_cfg.eWMode != GPRS_WM_SHUTDOWN) {
        bIsGprsInitOk = mdTRUE;

        bGPRS_GetCOPS(RT_TRUE);
        bGPRS_GetSMOND(RT_TRUE);

        for (int i = 0; i < 20; i++) {
            rt_thddog_feed("");
            bGPRS_GetCSQ(RT_TRUE);
            rt_kprintf("CSQ = %d\n", g_nGPRS_CSQ);
            rt_thread_delay(1 * RT_TICK_PER_SECOND);
            bGPRS_GetCREG(RT_TRUE);
            rt_kprintf("CREG = %d\n", g_nGPRS_CREG);
            rt_thread_delay(1 * RT_TICK_PER_SECOND);
            if (1 == g_nGPRS_CREG || 5 == g_nGPRS_CREG) {
                bGPRS_GetCSQ(RT_TRUE);
                rt_kprintf("CSQ = %d\n", g_nGPRS_CSQ);
                rt_kprintf("CREG = %d\n", g_nGPRS_CREG);
                break;
            }
        }

        if( (1 != g_nGPRS_CREG) && (5 != g_nGPRS_CREG) ){
            rt_thread_delete(s_gprs_parse_thread);
            s_gprs_parse_thread = NULL;
            serial_helper_close(BOARD_GPRS_UART);
            s_gprs_serial = RT_NULL;
            if( --start_cnt <= 0) {
                g_gprs_work_cfg.eWMode = GPRS_WM_SHUTDOWN;
                bIsGprsInitOk = RT_FALSE;
                elog_w("bGPRS_GetCREG", "bGPRS_GetCREG err 3 times");
                rt_kprintf("未注册到网络,关机%d\n",start_cnt);
            }else {
                vGPRS_HWRest();
                 rt_kprintf("未注册模块，复位模块\n");
                goto _start;
            }
        }
    }
    

    rt_thread_delete(s_gprs_parse_thread);
    s_gprs_parse_thread = NULL;
    serial_helper_close(BOARD_GPRS_UART);
    s_gprs_serial = RT_NULL;
    if (bIsGprsInitOk) {
        if(g_xCellNetType == E_GPRS_M26) {
            my_system("pppd call gprs &");
        } else if( (g_xCellNetType == E_4G_EC20) || (g_xCellNetType == E_4G_EC200S) ) {
            my_system("pppd call quectel-ppp &");
             // my_system("pppd call air720-ppp &");
        } else if(g_xCellNetType == E_NBIOT_BC26) {
            //my_system("pppd call quectel-ppp &");
        }else if(g_xCellNetType == E_AIR720) {
            my_system("pppd call air720-ppp &");
        }
        
        if (!__gprs_wait_pppd_up(30)) {
            my_system("killall pppd");
            bIsGprsInitOk = RT_FALSE;
            rt_kprintf("拨号失败，复位一下模块\n");
            vGPRS_HWRest();
            goto _start;
           /* if (!bHasGPRSFlag) {
                g_gprs_work_cfg.eWMode = GPRS_WM_SHUTDOWN;
                vGPRS_PowerDown();
            }*/
        } else {
            if (net_is_link()) {
                struct das_net_list_node net;
                memset(&net, 0, sizeof(net));
                das_do_get_net_info(DAS_NET_TYPE_ETH, 0, &net);
            }
            das_do_del_route(das_do_get_net_driver_name(DAS_NET_TYPE_LTE, 0));
            net_do_add_route(das_do_get_net_driver_name(DAS_NET_TYPE_LTE, 0));
        }
    } else {
        vGPRS_PowerDown();
    }
    return -RT_EOK;
}

void vGPRS_HWRest(void)
{

    rt_kprintf("vGPRS_HWRest\r\n");
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    __gprs_reset_ctrl(1);
    rt_thread_delay(RT_TICK_PER_SECOND );
    __gprs_reset_ctrl(0);
     rt_thread_delay(RT_TICK_PER_SECOND);

    rt_mutex_release(&gprs_mutex);
}

rt_bool_t bGPRS_ATE(rt_bool_t bECHO)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    GPRS_AT_DELAY();
    prvSendCmd("ATE");
    if (bECHO) {
        prvSendCmd("1\r\n");
    } else {
        prvSendCmd("0\r\n");
    }

    bRet = (prvWaitResult(200) && s_bRcvCode[GPRS_CODE_OK]);

    rt_mutex_release(&gprs_mutex);
    return bRet;
}

rt_bool_t bGPRS_GetNetTime(rt_bool_t bWait)
{
    static rt_bool_t bInTask = RT_FALSE;
    static rt_tick_t ulStartTick = 0;
    rt_uint32_t ulWaitTick = 10 * 60 * RT_TICK_PER_SECOND;
    rt_bool_t bRet = RT_FALSE;

    if(!_gprs_net_time_flag) {
        ulWaitTick = 60 * RT_TICK_PER_SECOND;
    }

    if (!bInTask && rt_tick_get() - ulStartTick > ulWaitTick) {
        rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

        GPRS_AT_DELAY();
        prvClearEvt(GPRS_EVT_NWTIME);
        prvSendCmd("AT+CCLK?\r\n");
        if (bWait && prvWaitResult(1000) && s_bRcvCode[GPRS_CODE_OK]) {
            bRet = RT_TRUE;
        }

        rt_mutex_release(&gprs_mutex);
        ulStartTick = rt_tick_get();
        bInTask = RT_FALSE;
    } else {
        bRet = RT_TRUE;
    }
    return bRet;
}

rt_bool_t bGPRS_GetCOPN(rt_bool_t bWait)
{
    rt_bool_t bRet = RT_FALSE;
    return bRet;
}

rt_bool_t bGPRS_GetCOPS(rt_bool_t bWait)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    GPRS_AT_DELAY();
    prvClearEvt(GPRS_EVT_COPS);
    prvSendCmd("AT+COPS?\r\n");
    if (bWait && prvWaitEvt(GPRS_EVT_COPS, RT_FALSE,500)) {
        bRet = RT_TRUE;
    }

    rt_mutex_release(&gprs_mutex);
    return bRet;
}

rt_bool_t bGPRS_GetSMOND(rt_bool_t bWait)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    

    rt_mutex_release(&gprs_mutex);
    return bRet;
}

rt_bool_t bGPRS_GetCSQ(rt_bool_t bWait)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    GPRS_AT_DELAY();
    prvClearEvt(GPRS_EVT_CSQ);
    prvSendCmd("AT+CSQ\r\n");
    if (bWait && prvWaitResult(500) && s_bRcvCode[GPRS_CODE_OK] && g_nGPRS_CSQ != UINT32_MAX) {
        bRet = RT_TRUE;
    }

    rt_mutex_release(&gprs_mutex);
    return bRet;
}

rt_bool_t bGPRS_GetCREG(rt_bool_t bWait)
{
    static rt_bool_t bInTask = RT_FALSE;
    static rt_tick_t ulStartTick = 0;
    static rt_uint32_t ulWaitTick = (RT_TICK_PER_SECOND * 5000 + 999) / 1000;
    rt_bool_t bRet = RT_FALSE;

    if (!bInTask && rt_tick_get() - ulStartTick > ulWaitTick) {
        rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

        GPRS_AT_DELAY();
        prvClearEvt(GPRS_EVT_CREG);
        prvSendCmd("AT+CGREG?\r\n");
        if (bWait && prvWaitResult(500) && s_bRcvCode[GPRS_CODE_OK] && g_nGPRS_CREG != UINT32_MAX) {
            bRet = RT_TRUE;
        }

        rt_mutex_release(&gprs_mutex);
        ulStartTick = rt_tick_get();
        bInTask = RT_FALSE;
    } else {
        bRet = RT_TRUE;
    }
    return bRet;
}


rt_bool_t bGPRS_GetMux(rt_bool_t bWait)
{
    
    rt_bool_t bRet = RT_FALSE;

    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    GPRS_AT_DELAY();
    prvClearEvt(GPRS_EVT_MUX);
    prvSendCmd("AT+QIMUX?\r\n");
    if (bWait && prvWaitEvt(GPRS_EVT_MUX, RT_FALSE,500) && g_nGPRS_MUX != -1) {
        bRet = RT_TRUE;
    }

    rt_mutex_release(&gprs_mutex);


    return bRet;
}

rt_bool_t bGPRS_SetCSCA(const char *szMsgNo)
{
    rt_bool_t bRet = RT_TRUE;
    rt_bool_t check = RT_TRUE;

    if (szMsgNo && szMsgNo[0]) {
        int len = strlen(szMsgNo);
        if (szMsgNo[0] != '+' && !isdigit(szMsgNo[0])) {
            check = RT_FALSE;
        }
        for (int i = 1; i < len; i++) {
            if (!isdigit(szMsgNo[i])) {
                check = RT_FALSE;
                break;
            }
        }
    } else {
        check = RT_FALSE;
    }

    if (check) {
        rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

        char cmd[64];
        snprintf(cmd, sizeof(cmd), "AT+CSCA=\"%s\"\r\n", szMsgNo);
        prvSendCmd(cmd);
        if (!prvWaitResult(1000) || !s_bRcvCode[GPRS_CODE_OK]) {
            bRet = RT_FALSE;
        }

        rt_mutex_release(&gprs_mutex);
    }

    return bRet;
}

// szAPN: default cmnet
rt_bool_t bGPRS_NetInit(const char *szAPN, const char *szAPNNo, const char *szUser, const char *szPsk)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

#if 0
    char cmd[64];

    //vGPRS_HWRest();
    bGPRS_ATE(0);

    if(bGPRS_GetMux(RT_TRUE)){
        rt_kprintf("g_nGPRS_MUX = %d\r\n", g_nGPRS_MUX);
    }

    if(g_nGPRS_MUX != 1){
        GPRS_AT_DELAY();
        prvSendCmd("AT+QIMUX=1\r\n");	 //设置启用多路复用
        if (!prvWaitResult(1000) || !s_bRcvCode[GPRS_CODE_OK]) {
            bRet = RT_FALSE; goto __END;
        }
    }

    GPRS_AT_DELAY();
    snprintf(cmd, sizeof(cmd), "AT+QICSGP=1,\"%s\"\r\n", (szAPN && szAPN[0]) ? szAPN : "CMNET"); //设置GPRS的APN
    prvSendCmd(cmd);
    if (!prvWaitResult(1000) || !s_bRcvCode[GPRS_CODE_OK]) {
        bRet = RT_FALSE; goto __END;
    }
#endif

    bRet = RT_TRUE;

__END:
    rt_mutex_release(&gprs_mutex);
    return bRet;
}

rt_bool_t bGPRS_ATTest(void)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    GPRS_AT_DELAY();
    prvSendCmd("AT\r\n");
    bRet = prvWaitResult(500);

    rt_mutex_release(&gprs_mutex);
    return bRet;
}


rt_bool_t bGPRS_ATReset(void)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    GPRS_AT_DELAY();
    prvSendCmd("AT+CFUN=1,1\r\n");
    bRet = prvWaitResult(500);

    rt_mutex_release(&gprs_mutex);
    return bRet;
}

//TCP/IP 数据包未确认时关闭链接需要等待的时长，单位为秒 60s
rt_bool_t bGPRS_TcpOt(void)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    GPRS_AT_DELAY();
    prvSendCmd("AT^SCFG=TCP/OT,60\r\n");
    bRet = prvWaitResult(500);

    rt_mutex_release(&gprs_mutex);
    return bRet;
}


void vGPRS_PowerDown(void)
{
    return;
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);
    
    __gprs_power_ctrl(1);
    rt_thread_delay(1.5 * RT_TICK_PER_SECOND);
    
    rt_mutex_release(&gprs_mutex);
}

void vGPRS_PowerUp(void)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    __gprs_power_ctrl(0);
    __gprs_reset_ctrl(0);

    rt_mutex_release(&gprs_mutex);

    // vGPRS_HWRest();
}


void vGPRS_TermDown(void)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    rt_thread_delay(RT_TICK_PER_SECOND);
    while (bGPRS_ATTest()) {
        __gprs_term_ctrl(0);
        rt_thread_delay(1.2 * RT_TICK_PER_SECOND);
        __gprs_term_ctrl(1);
        rt_thread_delay(RT_TICK_PER_SECOND / 2);
    }

    rt_mutex_release(&gprs_mutex);
}

void vGPRS_TermUp(void)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    int cnt = 0;
    while (!bGPRS_ATTest()) {
        __gprs_term_ctrl(1);
        rt_thread_delay(1.5 * RT_TICK_PER_SECOND);
        __gprs_term_ctrl(0);
        rt_thread_delay(3 * RT_TICK_PER_SECOND);
        {
            int n = 7;
            while (n--) {
                rt_thread_delay(1 * RT_TICK_PER_SECOND);
                if (bGPRS_ATTest()) break;
            }
        }
        if (cnt++ >= 2) {
            break;
        }
    }

    rt_mutex_release(&gprs_mutex);

    // vGPRS_HWRest();
}

static rt_bool_t prvWaitEvt(eGPRS_Evt eEvt, rt_bool_t bCheckErr, rt_base_t nMs)
{
    rt_bool_t bRet = RT_FALSE;
    rt_uint32_t ulStartTick = rt_tick_get();
    rt_uint32_t ulWaitTick = rt_tick_from_millisecond(nMs);

    while (rt_tick_get() - ulStartTick < ulWaitTick) {
        if (s_bIsEvtParse[eEvt] || (bCheckErr && s_bRcvCode[GPRS_CODE_ERROR])) {
            bRet = RT_TRUE;
            break;
        }
        rt_thread_delay(RT_TICK_PER_SECOND / 100);
    }

    return bRet;
}

static rt_bool_t prvWaitResult(rt_base_t nMs)
{
    rt_bool_t bRet = RT_FALSE;
    rt_uint32_t ulStartTick = rt_tick_get();
    rt_uint32_t ulWaitTick = rt_tick_from_millisecond(nMs);

    while (rt_tick_get() - ulStartTick < ulWaitTick) {
        if (s_bRcvCode[GPRS_CODE_OK] || s_bRcvCode[GPRS_CODE_ERROR]) {
            bRet = RT_TRUE;
            break;
        }
        rt_thread_delay(RT_TICK_PER_SECOND / 100);
    }

    return bRet;
}

