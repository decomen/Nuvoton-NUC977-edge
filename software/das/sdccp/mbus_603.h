#ifndef MBUS603_H
#define MBUS603_H

#include "mdtypedef.h"

#define MBUS603_PRE             0x68  /* 起始字符 */
#define MBUS603_MID             0x68  /* 控制域起始字符 */
#define MBUS603_EOM             0x16  /* 结束字符 */

#pragma pack(1)


//地址域
typedef struct{
	mdBYTE  addr;
}S_MBUS_A_t;


typedef struct {
    mdBYTE 		  btPre;     //起始字符      1
    S_MBUS_A_t  xAddr;     //地址域        6
    mdBYTE     state;  //状态
    mdBYTE     ver;    //版本
    mdBYTE		  btLen;	 //数据域长度    1
    pbyte         pData;     //数据域 		 n
    mdBYTE        btCheck;   //帧校验和      1
    mdBYTE        btEom;     //结束字符      1   
} S_Mbus603_Package_t;


typedef struct {
	mdBYTE        state;    //状态
	mdBYTE        ver;     //35代表603表
	mdUINT32           sum_e1; //累计热量
	mdUINT32           e3;    //能量E3
	mdUINT32           e8;    //能量E8
	mdUINT32           e9;    //能量E9
	mdUINT32           v1;    //累计流量V1
	mdUINT32           time_sum; //累计工作时间
	mdUINT32           T1;    //进水温度T1
	mdUINT32           T2;    //回水温度T2
	mdUINT32           dt;    //温差
	//mdUINT32           T3;    // T3
	mdUINT32           e1_e3; //瞬时功率
	mdUINT32           max_month_power; //月最大功率
	mdUINT32           f_rate;    //瞬时流速 m3/h
	
} S_Mbus603_Result_t;


typedef enum{
    MBUS603_OP_SUM_E1   = 1,
    MBUS603_OP_E3       = 2,
    MBUS603_OP_E8       = 3,
    MBUS603_OP_E9       = 4,
    MBUS603_OP_V1       = 5,
    MBUS603_OP_TIME_SUM = 6,
    MBUS603_OP_T1       = 7,
    MBUS603_OP_T2       = 8,
    MBUS603_OP_DT       = 9,
    //MBUS603_OP_T3       = 10,
    MBUS603_OP_E1_E3    = 10,       //瞬时功率
    MBUS603_OP_MAX_MONTH_POWER = 11, //月最大功率
    MBUS603_OP_F_RATE   = 12,
} MBUS603_OP_E;

#pragma pack()

#ifdef __cplusplus  
extern "C" {  
#endif  

//void vDlt645RequestCheckTime(S_DLT645_A_t xaddr, S_DLT645_Time_t *ptime);    //广播对时
//void vDlt645ReadAddr(S_DLT645_A_t xaddr);    //读取设备通讯地址
//void vDlt645SetBaud(S_DLT645_A_t xaddr, S_DLT645_BAUD_t baud);     //设置通讯波特率 默认2400
rt_bool_t bMbus603SampleData(int index, S_MBUS_A_t xaddr, int timeout);

rt_bool_t bMbus603ReadData(int index, S_MBUS_A_t xaddr, int timeout, S_Mbus603_Result_t *result);

typedef void (*pMbus603SendDataFun)(int index,mdBYTE *pdata,mdBYTE len);

//typedef void (*pDlt645Debug)(const char *fmt,...);
//extern pDlt645Debug DLT645_debug;


mdBOOL Mbus603_PutBytes(int index, mdBYTE *pBytes, mdUSHORT usBytesLen); //解析函数，每收到一包数据调用一次

void Mbus603_init(int index, pMbus603SendDataFun sendfunc);

#ifdef __cplusplus  
}  
#endif 

#endif
