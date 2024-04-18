
#include "board.h"

#include "mdtypedef.h"
#include "mbus_603.h"
#include "stdlib.h"
#include <arpa/inet.h>

#define DLT645_debug        rt_kprintf

static S_MBUS_A_t gMbus603Addr = {
	0x23  //35
};

static rt_mq_t s_mbus603_queue[BOARD_UART_MAX] = {0};
static pMbus603SendDataFun prvMbus603SendData = NULL;
S_Mbus603_Result_t g_result[BOARD_UART_MAX];



void Mbus603_init(int index, pMbus603SendDataFun sendfunc)
{
    if ((s_mbus603_queue[index] = rt_mq_create("mbus603", sizeof(S_Mbus603_Result_t), 1, RT_IPC_FLAG_PRIO)) != RT_NULL) {
        ;
    } else {
        rt_kprintf("mbus603 rt_mq_create mbus603 falied..\n");
        return ;
    }
    
    prvMbus603SendData = sendfunc;
}

/*static S_DLT645_A_t gDlt645AddrReal = {
	{0x96 ,0x58 ,0x72 ,0x10, 0x16 ,0x04}
};
const static S_DLT645_A_t gBroadcastAddr = {
	{0x99,0x99,0x99,0x99,0x99,0x99}
};*/

/*
const unsigned int g_ENERGY_DATA_MARKER_E[] = {
ENERGY_DATA_MARK_00,ENERGY_DATA_MARK_01,ENERGY_DATA_MARK_02,ENERGY_DATA_MARK_03,ENERGY_DATA_MARK_04,ENERGY_DATA_MARK_05,ENERGY_DATA_MARK_06,ENERGY_DATA_MARK_07,ENERGY_DATA_MARK_08,
ENERGY_DATA_MARK_20,ENERGY_DATA_MARK_21,ENERGY_DATA_MARK_22,ENERGY_DATA_MARK_23,ENERGY_DATA_MARK_24,ENERGY_DATA_MARK_25,ENERGY_DATA_MARK_26,ENERGY_DATA_MARK_27,
ENERGY_DATA_MARK_40,ENERGY_DATA_MARK_41,ENERGY_DATA_MARK_42,ENERGY_DATA_MARK_43,ENERGY_DATA_MARK_44,ENERGY_DATA_MARK_45,ENERGY_DATA_MARK_46,ENERGY_DATA_MARK_47,
ENERGY_DATA_MARK_60,ENERGY_DATA_MARK_61,ENERGY_DATA_MARK_62,ENERGY_DATA_MARK_63,ENERGY_DATA_MARK_64,ENERGY_DATA_MARK_65,ENERGY_DATA_MARK_66,ENERGY_DATA_MARK_67
};

const unsigned int g_VAR_DATA_MARKER_E[] = {
VAR_DATA_MARK_00,VAR_DATA_MARK_01,VAR_DATA_MARK_02,VAR_DATA_MARK_03,VAR_DATA_MARK_04,VAR_DATA_MARK_05,VAR_DATA_MARK_06,VAR_DATA_MARK_07,
VAR_DATA_MARK_08,VAR_DATA_MARK_09,VAR_DATA_MARK_0A,VAR_DATA_MARK_0B,VAR_DATA_MARK_0C,VAR_DATA_MARK_0D,VAR_DATA_MARK_0E,VAR_DATA_MARK_0F,
VAR_DATA_MARK_10,VAR_DATA_MARK_11,VAR_DATA_MARK_12,VAR_DATA_MARK_13,VAR_DATA_MARK_14,VAR_DATA_MARK_15,VAR_DATA_MARK_16,VAR_DATA_MARK_17,
VAR_DATA_MARK_18,VAR_DATA_MARK_19,VAR_DATA_MARK_1A,VAR_DATA_MARK_1B,VAR_DATA_MARK_1C,VAR_DATA_MARK_1D,VAR_DATA_MARK_1E,VAR_DATA_MARK_1F,

};*/

static byte CheckSum(pbyte buf, int len)
{
	byte sum = 0;
	int i = 0;
	for(i = 0 ; i < len; i++){
		sum+=buf[i];
	}
	return sum;
}
//pDlt645Debug DLT645_debug = NULL;

/*static void prvDlt645SendData(mdBYTE *pData  , mdBYTE len)
{
	
}*/


//10 5B 05 60 16

static mdBYTE buf[64] = {0};

static pbyte vCreatePaket(S_MBUS_A_t xAddr,mdBYTE *plen)
{
	int index = 0;   
	mdBYTE check = 0;

	buf[index] = 0x10;
	index++;

    buf[index] = 0x5B;
	index++;

    buf[index] = xAddr.addr;
	index++;

    buf[index] = (0x5B + xAddr.addr) & 0xff;
	index++;

    buf[index] = 0x16;
	index++;

    *plen = index;
    
	return buf;	
}


rt_bool_t bMbus603SampleData(int index, S_MBUS_A_t xaddr, int timeout)
{   
	mdBYTE len = 0;
	mdBYTE *pbuf = NULL;

    S_Mbus603_Result_t result;
    memset(&result,0,sizeof(S_Mbus603_Result_t));
	
	pbuf = vCreatePaket(xaddr, &len);

    if (s_mbus603_queue[index]) {
	    rt_mq_recv(s_mbus603_queue[index], &result, sizeof(S_Mbus603_Result_t), RT_WAITING_NO);
	}
    
	if (prvMbus603SendData) prvMbus603SendData(index, pbuf ,len);

	if (s_mbus603_queue[index]) {
    	while( RT_EOK == rt_mq_recv(s_mbus603_queue[index], &result, sizeof(S_Mbus603_Result_t), rt_tick_from_millisecond(timeout)) ) {
    	   // if (result.state == 0) {
    	        printf("mbus603 recive data:%d\r\n",result.sum_e1);
                g_result[index] = result; 
                return RT_TRUE;
           // }
    	}
	}

	return RT_FALSE;
}


rt_bool_t bMbus603ReadData(int index, S_MBUS_A_t xaddr, int timeout, S_Mbus603_Result_t *result)
{   
	/*mdBYTE len = 0;
	mdBYTE *pbuf = NULL;
	
	pbuf = vCreatePaket(xaddr, &len);

    if (s_mbus603_queue[index]) {
	    rt_mq_recv(s_mbus603_queue[index], result, sizeof(S_Mbus603_Result_t), RT_WAITING_NO);
	}
    
	if (prvMbus603SendData) prvMbus603SendData(index, pbuf ,len);

	if (s_mbus603_queue[index]) {
    	while( RT_EOK == rt_mq_recv(s_mbus603_queue[index], result, sizeof(S_Mbus603_Result_t), rt_tick_from_millisecond(timeout)) ) {
    	    if (result->state == 0) {
                return RT_TRUE;
            }
    	}
	}*/

    *result = g_result[index];

	return RT_TRUE;
}


/*

float           sum_e1; //累计热量
float           e8;    //能量E8
float           e9;    //能量E9
float           v1;    //累计流量V1
float           v2;    //累计流量V2
float           M1;    //质量流量M1
float           M2;    //质量流量M2
float           T1;    //进水温度T1
float           T2;    //回水温度T2
float           dt;    //温差
float           T3;    // T3
float           e1_e3; //瞬时功率
float           f_rate;    //瞬时流速 m3/h

*/



void Mbus603_ParsePack(int index, S_Mbus603_Package_t *pPack)
{
    S_Mbus603_Result_t result;
    memset(&result,0,sizeof(S_Mbus603_Result_t));

    result.state = pPack->state;
    result.ver  = pPack->ver;

    mdUINT32 value = 0;

    mdUINT16 value_short = 0;

    int offset = 0;
    printf("====== 累计热量E1：==========\n");
    for(int i = 0; i < 7;i++){
        printf("%02x ",pPack->pData[i+offset]);
    }
    memcpy(&value,&pPack->pData[offset+3],4);
    printf("value: %d\r\n",value);
    printf("==============================\n\n");
    offset += 7;
    result.sum_e1 = value;

    printf("====== 累计热量E3：==========\n");
    for(int i = 0; i < 9;i++){
        printf("%02x ",pPack->pData[i+offset]);
    }
    memcpy(&value,&pPack->pData[offset+5],4);
    printf("value: %d\r\n",value);
    printf("==============================\n\n");
    offset += 9;
    result.e3 = value;


    printf("====== 能量E8：==========\n");
    for(int i = 0; i < 7;i++){
        printf("%02x ",pPack->pData[i+offset]);
    }
    value = 0;
    memcpy(&value,&pPack->pData[offset+3],4);
    printf("value: %d\r\n",value);
    printf("==============================\n\n");
    offset += 7;
    result.e8 = value;


    printf("====== 能量E9：==========\n");
    for(int i = 0; i < 7;i++){
        printf("%02x ",pPack->pData[i+offset]);
    }
    value = 0;
    memcpy(&value,&pPack->pData[offset+3],4);
    printf("value: %d\r\n",value);
    printf("==============================\n\n");
    offset += 7;
    result.e9 = value;


    printf("====== 累计流量V1：==========\n");
    for(int i = 0; i < 6;i++){
        printf("%02x ",pPack->pData[i+offset]);
    }
    value = 0;
    memcpy(&value,&pPack->pData[offset+2],4);
    printf("value: %d\r\n",value);
    printf("==============================\n\n");
    offset += 6;
    result.v1 = value;


    offset += 7;  //跳过脉冲输入A
    offset += 8;  //跳过脉冲输入B



    printf("====== 累计工作时间：==========\n");
    for(int i = 0; i < 6;i++){
        printf("%02x ",pPack->pData[i+offset]);
    }
    value = 0;
    memcpy(&value,&pPack->pData[offset+2],4);
    printf("value: %d\r\n",value);
    printf("==============================\n\n");
    offset += 6;
    result.time_sum = value;



    offset += 6;  //跳过累计错误时间


    
    printf("====== 进水温度T1：==========\n");
    for(int i = 0; i < 4;i++){
        printf("%02x ",pPack->pData[i+offset]);
    }
    value_short = 0;
    memcpy(&value_short,&pPack->pData[offset+2],2);
    printf("value_short: %d\r\n",value_short);
    printf("==============================\n\n");
    offset +=4;
    result.T1 = value_short;


    printf("====== 回水温度T2：==========\n");
    for(int i = 0; i < 4;i++){
        printf("%02x ",pPack->pData[i+offset]);
    }
    value_short = 0;
    memcpy(&value_short,&pPack->pData[offset+2],2);
    printf("value_short: %d\r\n",value_short);
    printf("==============================\n\n");
    offset +=4;
     result.T2 = value_short;


    printf("====== 温差dt：==========\n");
    for(int i = 0; i < 4;i++){
        printf("%02x ",pPack->pData[i+offset]);
    }
    value_short = 0;
    memcpy(&value_short,&pPack->pData[offset+2],2);
    printf("value_short: %d\r\n",value_short);
    printf("==============================\n\n");
    offset +=4;
    result.dt = value_short;


    printf("====== 瞬时功率E1/E3：==========\n");
    for(int i = 0; i < 6;i++){
        printf("%02x ",pPack->pData[i+offset]);
    }
    value = 0;
    memcpy(&value,&pPack->pData[offset+2],4);
    printf("value: %d\r\n",value);
    printf("==============================\n\n");
    offset +=6;
    result.e1_e3 = value;


    printf("====== 月最大功率：==========\n");
    for(int i = 0; i < 6;i++){
        printf("%02x ",pPack->pData[i+offset]);
    }
    value = 0;
    memcpy(&value,&pPack->pData[offset+2],4);
    printf("value: %d\r\n",value);
    printf("==============================\n\n");
    offset +=6;
     result.max_month_power = value;


    printf("====== 瞬时流速：==========\n");
    for(int i = 0; i < 6;i++){
        printf("%02x ",pPack->pData[i+offset]);
    }
    value = 0;
    memcpy(&value,&pPack->pData[offset+2],4);
    printf("value: %d\r\n",value);
    printf("==============================\n\n");
    offset +=6;
    result.f_rate = value;

    if (s_mbus603_queue[index]) {
		rt_mq_send(s_mbus603_queue[index], &result, sizeof(S_Mbus603_Result_t));
	}
	
}


//校验位和停止位
//✓ 校验位：对报文头之后，校验位之前的所有字节进行和校验
//✓ 停止位：16h

//每收到一包数据，调用该函数
mdBOOL Mbus603_PutBytes(int index, mdBYTE *pBytes, mdUSHORT usBytesLen)
{

	mdBYTE check = 0;
	S_Mbus603_Package_t pack;

	if(pBytes[0] != MBUS603_PRE || pBytes[3] != MBUS603_MID || pBytes[usBytesLen-1] != MBUS603_EOM){
        DLT645_debug("不是完整的mbus帧\r\n");
		return mdFALSE;
	}

    pack.btLen = pBytes[1];
    printf("pack.btLen: %d\r\n",pack.btLen);

	check = CheckSum(&pBytes[4], pack.btLen);
    printf("check: 0x%02x pBytes[usBytesLen-2]: 0x%02x\r\n", check,  pBytes[usBytesLen-2]);
	if(check != pBytes[usBytesLen-2]){
        printf("mbus 603 check failed\r\n");
		return mdFALSE;
	}

    pack.ver = pBytes[13];   //版本
    pack.state = pBytes[16];  //状态码
	pack.pData = &pBytes[19];

    printf("mbus 603 state: %d\r\n",pack.state);

	Mbus603_ParsePack(index, &pack);
}



static void pMbus603SendDataFunTest(int index,mdBYTE *pdata,mdBYTE len)
{
    
}

static mdBYTE test_buf[]={
0x68,0xDB,0xDB,0x68,0x08,0x23,0x72,0x35,0x50,0x61,0x80,0x2D,0x2C,0x35,0x0D,0x09,0x10,0x00,0x00,0x04,0xFB,0x09,0xC6,0x1A,0x00,0x00,0x04,0xFB,0x89,0xFF,0x02,0x00,0x00,0x00,0x00,0x04,0xFF,0x07,0x59,0x79,
0x00,0x00,0x04,0xFF,0x08,0xDA,0x36,0x00,0x00,0x04,0x16,0x14,0x7E,0x00,0x00,0x84,0x40,0x14,0x00,0x00,0x00,0x00,0x84,0x80,0x40,0x14,0x00,0x00,0x00,0x00,0x04,0x22,0x3C,0x18,0x00,0x00,0x34,0x22,0x3F,
0x02,0x00,0x00,0x02,0x59,0x68,0x25,0x02,0x5D,0x5C,0x0F,0x02,0x61,0x0C,0x16,0x04,0x2F,0xE1,0x00,0x00,0x00,0x14,0x2F,0x94,0x02,0x00,0x00,0x04,0x3D,0x63,0x01,0x00,0x00,0x14,0x3D,0x3D,0x04,0x00,0x00,
0x04,0xFF,0x22,0x00,0x00,0x01,0x00,0x04,0x6D,0x06,0x2B,0xB2,0x21,0x44,0xFB,0x09,0xBC,0x07,0x00,0x00,0x44,0xFB,0x89,0xFF,0x02,0x00,0x00,0x00,0x00,0x44,0xFF,0x07,0x89,0x21,0x00,0x00,0x44,0xFF,0x08,
0x52,0x0E,0x00,0x00,0x44,0x16,0x06,0x23,0x00,0x00,0xC4,0x40,0x14,0x00,0x00,0x00,0x00,0xC4,0x80,0x40,0x14,0x00,0x00,0x00,0x00,0x54,0x2F,0x63,0x02,0x00,0x00,0x54,0x3D,0x3C,0x04,0x00,0x00,0x42,0x6C,
0xA1,0x21,0x02,0xFF,0x1A,0x02,0x1B,0x0C,0x78,0x35,0x50,0x61,0x80,0x04,0xFF,0x16,0xE5,0x84,0x1E,0x00,0x04,0xFF,0x17,0xC1,0xD5,0xB4,0x00,0xF7,0x16
};

void mbus603_test(void)
{
   // Mbus603_init(0, pMbus603SendDataFunTest);
   // Mbus603_PutBytes(0, test_buf, sizeof(test_buf));
}

