#ifndef DLT645_1997_H
#define DLT645_1997_H

#include "mdtypedef.h"

#define DLT645_1997_PRE             0x68  /* 起始字符 */
#define DLT645_1997_MID             0x68  /* 控制域起始字符 */
#define DLT645_1997_EOM             0x16  /* 结束字符 */

//控制域功能码 D0-D4

typedef enum{
	C_1997_CODE_REV = 0x00, 		   //保留
	C_1997_CODE_CHECK_TIME = 0X08,     //广播对时
	C_1997_CODE_READ_DATA =  0x01,     //读数据
	C_1997_CODE_READ_LAST_DATA = 0x02, //读后续数据
	C_1997_CODE_WRITE_DATA = 0x04,      //写数据
	C_1997_CODE_WRITE_ADDR = 0x0a,       //写通讯地址
	C_1997_CODE_SET_BAUD =   0x0c,       //修改通讯波特率
	C_1997_CODE_SET_PASSWD = 0x0f,       //修改密码
	C_1997_CODE_MAX_DEMAND_CLEAR = 0x10,   //最大需量清零

}S_DLT645_1997_C_1997_CODE_E;

typedef enum{
	C_1997_DIR_HOST = 0,   //主站发出的命令帧
	C_1997_DIR_SLAVE = 1,   //从站发出的应答帧
}S_DLT645_1997_C_DIR_E;

typedef enum{
	C_1997_ACK_OK = 0,  //从站正确应答
	C_1997_ACK_ERR = 1, //从站异常应答
}S_DLT645_1997_C_ACK_E;

typedef enum{
	C_1997_FCK_0 = 0,  //无后续数据帧
	C_1997_FCK_1 = 1,  //有后续数据帧
}S_DLT645_1997_C_FCK_E;



//电能量数据标识

//无功电能单位是kvarh （千乏时）
//有功电能单位是kWh （千瓦时)
typedef enum{
	
	ENERGY_1997_DATA_MARK_B0	= 0x9010, //(当前)正向有功总电能
	ENERGY_1997_DATA_MARK_B1	= 0x9011, //(当前)费率1（尖）
	ENERGY_1997_DATA_MARK_B2	= 0x9012, //(当前)费率2（峰）
	ENERGY_1997_DATA_MARK_B3	= 0x9013, //(当前)费率3（平）
	ENERGY_1997_DATA_MARK_B4	= 0x9014, //(当前)费率4（谷）
	
	
	ENERGY_1997_DATA_MARK_C0	= 0x9020, //(当前)反向有功总电能
	ENERGY_1997_DATA_MARK_C1	= 0x9021, //(当前)费率1（尖）
	ENERGY_1997_DATA_MARK_C2	= 0x9022, //(当前)费率2（峰）
	ENERGY_1997_DATA_MARK_C3	= 0x9023, //(当前)费率3（平）
	ENERGY_1997_DATA_MARK_C4	= 0x9024, //(当前)费率4（谷）
	
	
	ENERGY_1997_DATA_MARK_D0	= 0x9110, //(当前)正向无功总电能
	ENERGY_1997_DATA_MARK_D1	= 0x9111, //(当前)费率1（尖）
	ENERGY_1997_DATA_MARK_D2	= 0x9112, //(当前)费率2（峰）
	ENERGY_1997_DATA_MARK_D3	= 0x9113, //(当前)费率3（平）
	ENERGY_1997_DATA_MARK_D4	= 0x9114, //(当前)费率4（谷）
	
	ENERGY_1997_DATA_MARK_E0	= 0x9120, //(当前)反向无功总电能
	ENERGY_1997_DATA_MARK_E1	= 0x9121, //(当前)费率1（尖）
	ENERGY_1997_DATA_MARK_E2	= 0x9122, //(当前)费率2（峰）
	ENERGY_1997_DATA_MARK_E3	= 0x9123, //(当前)费率3（平）
	ENERGY_1997_DATA_MARK_E4	= 0x9124, //(当前)费率4（谷）
	
	
	ENERGY_1997_DATA_MARK_F0	= 0x9130, //(当前)第一象限无功总电能
	ENERGY_1997_DATA_MARK_F1	= 0x9131, //(当前)费率1（尖）
	ENERGY_1997_DATA_MARK_F2	= 0x9132, //(当前)费率2（峰）
	ENERGY_1997_DATA_MARK_F3	= 0x9133, //(当前)费率3（平）
	ENERGY_1997_DATA_MARK_F4	= 0x9134, //(当前)费率4（谷）
	
	ENERGY_1997_DATA_MARK_G0	= 0x9140, //(当前)第四象限无功总电能
	ENERGY_1997_DATA_MARK_G1	= 0x9141, //(当前)费率1（尖）
	ENERGY_1997_DATA_MARK_G2	= 0x9142, //(当前)费率2（峰）
	ENERGY_1997_DATA_MARK_G3	= 0x9143, //(当前)费率3（平）
	ENERGY_1997_DATA_MARK_G4	= 0x9144, //(当前)费率4（谷）
	
	ENERGY_1997_DATA_MARK_H0	= 0x9150, //(当前)第二象限无功总电能
	ENERGY_1997_DATA_MARK_H1	= 0x9151, //(当前)费率1（尖）
	ENERGY_1997_DATA_MARK_H2	= 0x9152, //(当前)费率2（峰）
	ENERGY_1997_DATA_MARK_H3	= 0x9153, //(当前)费率3（平）
	ENERGY_1997_DATA_MARK_H4	= 0x9154, //(当前)费率4（谷）
	
	ENERGY_1997_DATA_MARK_J0	= 0x9160, //(当前)第三象限无功总电能
	ENERGY_1997_DATA_MARK_J1	= 0x9161, //(当前)费率1（尖）
	ENERGY_1997_DATA_MARK_J2	= 0x9162, //(当前)费率2（峰）
	ENERGY_1997_DATA_MARK_J3	= 0x9163, //(当前)费率3（平）
	ENERGY_1997_DATA_MARK_J4	= 0x9164, //(当前)费率4（谷）

	ENERGY_1997_DATA_MARK_K0	= 0xA010,  // (当前)正向有功总最大需量及发生时间
	ENERGY_1997_DATA_MARK_K1	= 0xA020,  // (当前)反向有功总最大需量及发生时间
	ENERGY_1997_DATA_MARK_K2	= 0xA110,  // (当前)正向无功总最大需量及发生时间
	ENERGY_1997_DATA_MARK_K3	= 0xA120,  // (当前)反向无功总最大需量及发生时间

	

	
}S_DLT645_1997_ENERGY_1997_DATA_MARKER_E;

// 表变量数据标识编码表

//电压单位 V 伏特
//电流单位 A 安培
//有功功率单位 kw 千瓦
//无功功率单位 kvar 千乏
//视在功率单位 kVA 
//功率因素单位 无单位 
//相角单位 °(度) 

typedef enum{

	VAR_1997_DATA_MARK_00	= 0xB611,  // A相电压
	VAR_1997_DATA_MARK_01	= 0xB612,  // B相电压
	VAR_1997_DATA_MARK_02	= 0xB613,  // C相电压

	VAR_1997_DATA_MARK_04	= 0xB621,  // A相电流
	VAR_1997_DATA_MARK_05	= 0xB622,  // B相电流
	VAR_1997_DATA_MARK_06	= 0xB623,  // C相电流

	VAR_1997_DATA_MARK_08	= 0xB630, // 瞬时有功功率
	VAR_1997_DATA_MARK_09	= 0xB631, // 瞬时A相有功功率
	VAR_1997_DATA_MARK_0A	= 0xB632, // 瞬时B相有功功率
	VAR_1997_DATA_MARK_0B	= 0xB633, // 瞬时C相有功功率

	VAR_1997_DATA_MARK_0D	= 0xB640, // 瞬时无功功率
	VAR_1997_DATA_MARK_0E	= 0xB641, // 瞬时A相无功功率
	VAR_1997_DATA_MARK_0F	= 0xB642, // 瞬时B相无功功率
	VAR_1997_DATA_MARK_10	= 0xB643, // 瞬时C相无功功率


	VAR_1997_DATA_MARK_17	= 0xB650, // 总功率因数
	VAR_1997_DATA_MARK_18	= 0xB651, // A相功率因数
	VAR_1997_DATA_MARK_19	= 0xB652, // B相功率因数
	VAR_1997_DATA_MARK_1A	= 0xB653, // C相功率因数


}S_DLT645_1997_VAR_DATA_MARKER_E;

#pragma pack(1)

//控制域
typedef struct{
	mdBYTE C_1997_CODE   :5; //D0-D4 功能码
	mdBYTE FCK  	  :1; // 0 无后续数据帧，1有后续数据帧
	mdBYTE ACK      :1; //ACK = 0 从站正确应答 ACK = 1 从站异常应答
	mdBYTE DIR      :1; //传输方向位DIR 0主站发出的下行报文  1 从站发出的应答帧
}S_DLT645_1997_C_t;

//地址域
typedef struct{
	mdBYTE  addr[6];
}S_DLT645_1997_A_t;


typedef struct {
    mdBYTE 		  btPre;     //起始字符      1
    S_DLT645_1997_A_t  xAddr;     //地址域        6
    mdBYTE        btMid;     //起始字符		 1
    S_DLT645_1997_C_t  xCon; 	 //控制域        1
    mdBYTE		  btLen;	 //数据域长度    1
    pbyte         pData;     //数据域 		 n
    mdBYTE        btCheck;   //帧校验和      1
    mdBYTE        btEom;     //结束字符      1   
} S_DLT645_1997_Package_t;


typedef struct{
	mdBYTE YY; // 年
	mdBYTE MM; //月
	mdBYTE DD; //日
	mdBYTE hh; //时   
	mdBYTE mm; //分  
	mdBYTE ss; //秒 
}S_DLT645_1997_Time_t;

//通讯速率状态字

/*
typedef enum{
	BAUD_600BPS   = (1<<1),
	BAUD_1200BPS  = (1<<2),
	BAUD_2400BPS  = (1<<3),
	BAUD_4800BPS  = (1<<4),
	BAUD_9600BPS  = (1<<5),
	BAUD_19200BPS = (1<<6),
}S_DLT645_1997_BAUD_t;

*/

typedef struct {
    int index;
    S_DLT645_1997_A_t    addr;
	mdUINT32        op;
	float           val;
} S_DLT645_1997_Result_t;

#pragma pack()

#ifdef __cplusplus  
extern "C" {  
#endif  

//void vDlt645_1997RequestCheckTime(S_DLT645_1997_A_t xaddr, S_DLT645_1997_Time_t *ptime);    //广播对时
//void vDlt645_1997ReadAddr(S_DLT645_1997_A_t xaddr);    //读取设备通讯地址
//void vDlt645_1997SetBaud(S_DLT645_1997_A_t xaddr, S_DLT645_1997_BAUD_t baud);     //设置通讯波特率 默认2400
rt_bool_t bDlt645_1997ReadData(int index, S_DLT645_1997_A_t xaddr, mdUINT16 DataMarker, int timeout, S_DLT645_1997_Result_t *result);


typedef void (*pDlt645_1997SendDataFun)(int index,mdBYTE *pdata,mdBYTE len);

//typedef void (*pDlt645_1997Debug)(const char *fmt,...);
//extern pDlt645_1997Debug DLT645_1997_debug;

extern const unsigned int g_ENERGY_1997_DATA_MARKER_E[];
extern const unsigned int g_VAR_DATA_MARKER_E[];

void dlt645_1997_init(int index, pDlt645_1997SendDataFun sendfunc);
mdBOOL Dlt645_1997_PutBytes(int index, mdBYTE *pBytes, mdUSHORT usBytesLen); //解析函数，每收到一包数据调用一次
void vDlt645_1997ResponAddr();    //从机反馈设备地址(用于测试)

#ifdef __cplusplus  
}  
#endif 

#endif
