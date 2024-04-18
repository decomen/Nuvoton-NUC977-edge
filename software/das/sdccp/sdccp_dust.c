
#include "board.h"

#include "mdtypedef.h"
#include "sdccp_dust.h"
#include "stdlib.h"

#define DUST_debug        rt_kprintf

static rt_mq_t s_dust_queue[BOARD_UART_MAX] = {0};
static pDustSendDataFun prvDustSendData = NULL;

#define DUST_DATA_CNT_MAX       (60)
static rt_int8_t s_dust_data_cnt[BOARD_UART_MAX] = {0};

void dust_init(int index, pDustSendDataFun sendfunc)
{
    if ((s_dust_queue[index] = rt_mq_create("dust", sizeof(S_Dust_Result_t), 1, RT_IPC_FLAG_PRIO)) != RT_NULL) {
        ;
    } else {
        rt_kprintf("dlt645 rt_mq_create dlt645 falied..\n");
        return ;
    }
    
    prvDustSendData = sendfunc;
}


static byte CheckSum(pbyte buf, int len)
{
	byte sum = 0;
	int i = 0;
	for(i = 0 ; i < len; i++){
		sum+=buf[i];
	}
	return sum;
}

static pbyte prvCreatePaket(S_DUST_C_CODE_E code , mdBYTE *pPacketLen)
{
	static mdBYTE buf[64] = {0};
	int index = 0;   
	mdBYTE check = 0;

	buf[index] = DUST_PRE1;
	index++;
    buf[index] = DUST_PRE2;
	index++;
    buf[index] = 0x01;
    index++;

    buf[index] = (mdBYTE)(code&0xff);
    index++;

	check = CheckSum(&buf[2],index-2);
	buf[index] = check;
	index++;

	*pPacketLen = index;

	return buf;	
}

rt_bool_t vDustRequestReadDevNo(int index, mdUINT32 *pDevNo,int timeout)   //读取设备号
{
    S_Dust_Result_t result;
    memset(&result, 0 ,sizeof(S_Dust_Result_t));

    mdBYTE xPacketLen = 0;
	mdBYTE *pbuf = NULL;
	pbuf = prvCreatePaket(CODE_READ_DEVNO, &xPacketLen);
	if (prvDustSendData) prvDustSendData(0, pbuf ,xPacketLen);

    if (s_dust_queue[index]) {
    	while( RT_EOK == rt_mq_recv(s_dust_queue[index], &result, sizeof(S_Dust_Result_t), rt_tick_from_millisecond(timeout)) ) {
    	    if (result.code == CODE_READ_DEVNO) {
                *pDevNo = result.result.DevNo.ulDevNo;
                return RT_TRUE;
            }
    	}
	}

	return RT_FALSE;

}

/*

void vDlt645ReadVer()    //读取设备通讯地址  该命令不生效。
{
	mdBYTE xPacketLen = 0;
	mdBYTE *pbuf = NULL;
	pbuf = prvCreatePaket(CODE_READ_VERSION, &xPacketLen);
	if (prvDustSendData) prvDustSendData(0, pbuf ,xPacketLen);
}

*/


rt_bool_t bDustStartSample(int index, int timeout)
{
    S_Dust_Result_t result;
    memset(&result, 0 ,sizeof(S_Dust_Result_t));
    
	mdBYTE xPacketLen = 0;
	mdBYTE *pbuf = NULL;
	pbuf = prvCreatePaket(CODE_START_SAMPLE, &xPacketLen);
	if (prvDustSendData) prvDustSendData(0, pbuf ,xPacketLen);
       if (s_dust_queue[index]) {
	    rt_mq_recv(s_dust_queue[index], &result, sizeof(S_Dust_Result_t), RT_WAITING_NO);
	}
	if (prvDustSendData) prvDustSendData(index, pbuf ,xPacketLen);

	if (s_dust_queue[index]) {
    	while( RT_EOK == rt_mq_recv(s_dust_queue[index], &result, sizeof(S_Dust_Result_t), rt_tick_from_millisecond(timeout)) ) {
    	    if (result.code == CODE_OK) {
                s_dust_data_cnt[index] = 0;
                return RT_TRUE;
            }
    	}
	}

	return RT_FALSE;
}

rt_bool_t bDustStopSample(int index , int timeout)
{
    S_Dust_Result_t result;
    memset(&result, 0 ,sizeof(S_Dust_Result_t));

    mdBYTE xPacketLen = 0;
	mdBYTE *pbuf = NULL;
	pbuf = prvCreatePaket(CODE_STOP_SAMPLE, &xPacketLen);
	if (prvDustSendData) prvDustSendData(0, pbuf ,xPacketLen);

    if (s_dust_queue[index]) {
    	while( RT_EOK == rt_mq_recv(s_dust_queue[index], &result, sizeof(S_Dust_Result_t), rt_tick_from_millisecond(timeout)) ) {
    	    if (result.code == CODE_OK) {
                s_dust_data_cnt[index] = DUST_DATA_CNT_MAX;
                return RT_TRUE;
            }
    	}
	}
    
    return RT_FALSE;
}


/*

rt_bool_t bDlt645ReadData(int index, mdUINT32 DataMarker, int timeout, S_DLT645_Result_t *result) //不需要主动采集，发送开始命令后，每隔1S主动上报一次数据，上报60条数据后，自动停止。
{   
	S_DLT645_C_t xCon;
	mdBYTE xPacketLen = 0;
	mdBYTE *pbuf = NULL;
	mdUINT32 datamarker = 0;

	xCon.C_CODE = C_CODE_READ_DATA;
	xCon.DIR = C_DIR_HOST;
	xCon.ACK = C_ACK_OK;
	xCon.FCK = C_FCK_0;

	datamarker = (mdUINT32)DataMarker + 0x33333333;
	
	pbuf = vCreatePaket(xaddr, xCon, (mdBYTE*)&datamarker, 4, &xPacketLen);
	if (s_dust_queue[index]) {
	    rt_mq_recv(s_dust_queue[index], result, sizeof(S_Dust_Result_t), RT_WAITING_NO);
	}
	if (prvDustSendData) prvDustSendData(index, pbuf ,xPacketLen);

	if (s_dust_queue[index]) {
    	while( RT_EOK == rt_mq_recv(s_dust_queue[index], result, sizeof(S_Dust_Result_t), rt_tick_from_millisecond(timeout)) ) {
    	    if ((0 == memcmp(&result->addr, &xaddr, sizeof(S_DLT645_A_t))) && result->op == DataMarker) {
                return RT_TRUE;
            }
    	}
	}

	return RT_FALSE;
}
*/

static void Dust_ParsePack(int index, S_Dust_Package_t *pPack)
{
	
    S_Dust_Result_t result;
    memset(&result, 0 , sizeof(S_Dust_Result_t));

	if(pPack->btLen == 0){
		DUST_debug("ACK\r\n");
        if (s_dust_queue[index]){
            result.index = index;
            result.code = CODE_OK;
    		rt_mq_send(s_dust_queue[index], &result, sizeof(S_Dust_Result_t));
    	} 
		return;
	}

	if(pPack->btLen == 0x04){
		DUST_debug("获取设备号\r\n");
        if (s_dust_queue[index]) {
            result.index = index;
            result.code = CODE_READ_DEVNO;
            memcpy(&result.result.DevNo.ulDevNo,pPack->pData,4);
            result.result.DevNo.ulDevNo = htonl(result.result.DevNo.ulDevNo);
    		rt_mq_send(s_dust_queue[index], &result, sizeof(S_Dust_Result_t));
    	}
		return;
	}

	if(pPack->btLen == sizeof(s_Dust_Data_t)){
		DUST_debug("获取到数据\r\n");
        s_Dust_Data_t data;
    	memset(&data,0,sizeof(s_Dust_Data_t));
		memcpy(&data,pPack->pData,sizeof(s_Dust_Data_t));
        if (s_dust_queue[index]) {
            result.index = index;
            result.code = CODE_DATA;
            result.result.data.PM25 = htons(data.PM25);
            result.result.data.PM10 = htons(data.PM10);
            result.result.data.PM10Sum = htonl(data.PM10Sum);
            result.result.data.PM25Num = htons(data.PM25Num);
            result.result.data.PM10Num = htons(data.PM10Num);
            result.result.data.T = data.T;
            result.result.data.H = data.H;
            if(s_dust_data_cnt[index] < DUST_DATA_CNT_MAX) s_dust_data_cnt[index]++;
            rt_mq_reset(s_dust_queue[index], RT_IPC_CMD_RESET, NULL);
            rt_mq_send(s_dust_queue[index], &result, sizeof(S_Dust_Result_t));
    	}
        
		return;
	}
}

rt_bool_t bDust_ReadData(int index, int timeout, Dust_ResultData_t *data)
{
    if (s_dust_data_cnt[index] >= DUST_DATA_CNT_MAX) 
        timeout = 0;
    
    S_Dust_Result_t result;
    if (RT_EOK == rt_mq_recv(s_dust_queue[index], &result, sizeof(S_Dust_Result_t), rt_tick_from_millisecond(timeout))) {
        if (result.code == CODE_DATA) {
            *data = result.result.data;
            return RT_TRUE;
        }
    }

	return RT_FALSE;
}

//每收到一包数据，调用该函数
rt_bool_t Dust_PutBytes(int index, mdBYTE *pBytes, mdUSHORT usBytesLen)
{
	byte check = 0;
	S_Dust_Package_t pack;
	int i = 0;
	for(i = 0 ; i < usBytesLen; i++){
		if(pBytes[i] == DUST_PRE1 && pBytes[i+1] == DUST_PRE2){
			break;
		}
	}

   pBytes += i;
   usBytesLen -= i;
	
	if(pBytes[0] != DUST_PRE1 || pBytes[1] != DUST_PRE2){
		return RT_FALSE;
	}

	check = CheckSum(&pBytes[2],usBytesLen - 3);
	if(check != pBytes[usBytesLen-1]){
		return RT_FALSE;
	}

	memcpy(&pack,pBytes, 3);
	if(usBytesLen > 3){
		pack.pData = &pBytes[3];
	}

	Dust_ParsePack(index, &pack);

    return RT_TRUE;
}

