#ifndef DUST_H
#define DUST_H

#include "mdtypedef.h"

#define DUST_PRE1             0xff  /* 起始字符 0xff 0x43 */
#define DUST_PRE2             0x43  

//控制域功能码 D0-D4

typedef enum {
    CODE_OK = 0x00,

    CODE_START_SAMPLE = 0x01,     //开始采集
    CODE_STOP_SAMPLE =  0x02,     //结束采集

    CODE_READ_DEVNO =   0x04, 		   //读取设备号
    CODE_READ_VERSION = 0x05,         //读取版本号

    CODE_DATA = 0xaa,
}S_DUST_C_CODE_E;

typedef enum{
    DUST_OP_PM25        = 0x01,
    DUST_OP_PM10        = 0x02,
    DUST_OP_PM25_SUM    = 0x03,
    DUST_OP_PM10_SUM    = 0x04,
    DUST_OP_PM25_NUM    = 0x05,
    DUST_OP_PM10_NUM    = 0x06,
    DUST_OP_T           = 0x07,
    DUST_OP_H           = 0x08,
} S_DUST_C_OP_E;

#pragma pack(1)

typedef struct {
    mdBYTE 		  btPre1;     //起始字符      1
    mdBYTE 		  btPre2;     //起始字符      1
    mdBYTE		  btLen;	  //数据域长度    1
    pbyte         pData;      //数据域 		 n
    mdBYTE        btCheck;    //帧校验和      1
} S_Dust_Package_t;

typedef struct 
{
    mdUINT16 PM25;   //通道1颗粒物粒径值
	mdUINT16 PM10;   //通道2颗粒物粒径值
    mdUINT32 PM25Sum; //通道1的每升累积值
	mdUINT32 PM10Sum; //通道2的每升累计值
	mdUINT16 PM25Num; //PM2.5的计数结果(ug/m3)
	mdUINT16 PM10Num; //PM10的计数结果(ug/m3)
	mdBYTE   T;  //温度
	mdBYTE   H;  //湿度
} Dust_ResultData_t;

typedef struct
{
    mdUINT32 ulDevNo;
} Dust_ResultDevNo_t;


typedef struct {
    int index;
    S_DUST_C_CODE_E code;
    union{
       Dust_ResultData_t    data;
       Dust_ResultDevNo_t   DevNo;
    }result;
} S_Dust_Result_t;


typedef struct
{
	mdBYTE c_code;   //控制码  8:0温湿度无效 1温湿度有效  7澹?0日期数据无效 1日期数据有效
	mdUINT16 PM25;   //通道1颗粒物粒径值
	mdUINT16 PM10;   //通道2颗粒物粒径值
	mdUINT32 PM25Sum; //通道1的每升累积值
	mdUINT32 PM10Sum; //通道2的每升累计值
	mdUINT16 PM25Num; //PM2.5的计数结果(ug/m3)
	mdUINT16 PM10Num; //PM10的计数结果(ug/m3)
	mdBYTE   T;
	mdBYTE   H;
	mdBYTE   year;
	mdBYTE   mon;
	mdBYTE   day;
    mdBYTE   week;
	mdBYTE   hour;
	mdBYTE   min;
	mdBYTE   sec;
	mdBYTE   RemainTime;
}s_Dust_Data_t;

#pragma pack()

#ifdef __cplusplus  
extern "C" {  
#endif  

typedef void (*pDustSendDataFun)(int index,mdBYTE *pdata,mdBYTE len);

rt_bool_t bDustStartSample(int index, int timeout);
rt_bool_t bDustStopSample(int index , int timeout);
rt_bool_t Dust_PutBytes(int index, mdBYTE *pBytes, mdUSHORT usBytesLen); //解析函数，每收到一包数据调用一次
rt_bool_t bDust_ReadData(int index, int timeout, Dust_ResultData_t *data);

void dust_init(int index, pDustSendDataFun sendfunc);

#ifdef __cplusplus  
}  
#endif 

#endif
