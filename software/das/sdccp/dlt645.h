#ifndef DLT645_H
#define DLT645_H

#include "mdtypedef.h"

#define DLT645_PRE             0x68  /* 起始字符 */
#define DLT645_MID             0x68  /* 控制域起始字符 */
#define DLT645_EOM             0x16  /* 结束字符 */



//控制域功能码 D0-D4

typedef enum{
	C_CODE_REV = 0x00, 		   //保留
	C_CODE_CHECK_TIME = 0X08,     //广播对时
	C_CODE_READ_DATA =  0x11,     //读数据
	C_CODE_READ_LAST_DATA = 0x12, //读后续数据
	C_CODE_READ_ADDR = 0x13,      //读通讯地址
	C_CODE_WRITE_DATA = 0x14,      //写数据
	C_CODE_WRITE_ADDR = 0x15,       //写通讯地址
	C_CODE_FREEZE_CDM = 0x16,       //冻结命令
	C_CODE_SET_BAUD =   0x17,       //修改通讯波特率
	C_CODE_SET_PASSWD = 0x18,       //修改密码
	C_CODE_MAX_DEMAND_CLEAR = 0x19,   //最大需量清零
	C_CODE_METER_CLEAR = 0x1A,   //电表清零
	C_CODE_EVENT_CLEAR = 0x1B,   //事件清零
}S_DLT645_C_CODE_E;

typedef enum{
	C_DIR_HOST = 0,   //主站发出的命令帧
	C_DIR_SLAVE = 1,   //从站发出的应答帧
}S_DLT645_C_DIR_E;

typedef enum{
	C_ACK_OK = 0,  //从站正确应答
	C_ACK_ERR = 1, //从站异常应答
}S_DLT645_C_ACK_E;

typedef enum{
	C_FCK_0 = 0,  //无后续数据帧
	C_FCK_1 = 1,  //有后续数据帧
}S_DLT645_C_FCK_E;



//电能量数据标识

//无功电能单位是kvarh （千乏时）
//有功电能单位是kWh （千瓦时)
typedef enum{

	ENERGY_DATA_MARK_A0	= 0x00000000, //(当前)组合有功总电能
	ENERGY_DATA_MARK_A1	= 0x00000100, //(当前)费率1（尖）
	ENERGY_DATA_MARK_A2	= 0x00000200, //(当前)费率2（峰）
	ENERGY_DATA_MARK_A3	= 0x00000300, //(当前)费率3（平）
	ENERGY_DATA_MARK_A4	= 0x00000400, //(当前)费率4（谷）
	
	ENERGY_DATA_MARK_B0	= 0x00010000, //(当前)正向有功总电能
	ENERGY_DATA_MARK_B1	= 0x00010100, //(当前)费率1（尖）
	ENERGY_DATA_MARK_B2	= 0x00010200, //(当前)费率2（峰）
	ENERGY_DATA_MARK_B3	= 0x00010300, //(当前)费率3（平）
	ENERGY_DATA_MARK_B4	= 0x00010400, //(当前)费率4（谷）
	
	
	ENERGY_DATA_MARK_C0	= 0x00020000, //(当前)反向有功总电能
	ENERGY_DATA_MARK_C1	= 0x00020100, //(当前)费率1（尖）
	ENERGY_DATA_MARK_C2	= 0x00020200, //(当前)费率2（峰）
	ENERGY_DATA_MARK_C3	= 0x00020300, //(当前)费率3（平）
	ENERGY_DATA_MARK_C4	= 0x00020400, //(当前)费率4（谷）
	
	
	ENERGY_DATA_MARK_D0	= 0x00030000, //(当前)组合无功1总电能
	ENERGY_DATA_MARK_D1	= 0x00030100, //(当前)费率1（尖）
	ENERGY_DATA_MARK_D2	= 0x00030200, //(当前)费率2（峰）
	ENERGY_DATA_MARK_D3	= 0x00030300, //(当前)费率3（平）
	ENERGY_DATA_MARK_D4	= 0x00030400, //(当前)费率4（谷）
	
	ENERGY_DATA_MARK_E0	= 0x00040000, //(当前)组合无功2总电能
	ENERGY_DATA_MARK_E1	= 0x00040100, //(当前)费率1（尖）
	ENERGY_DATA_MARK_E2	= 0x00040200, //(当前)费率2（峰）
	ENERGY_DATA_MARK_E3	= 0x00040300, //(当前)费率3（平）
	ENERGY_DATA_MARK_E4	= 0x00040400, //(当前)费率4（谷）
	
	
	ENERGY_DATA_MARK_F0	= 0x00050000, //(当前)第一象限无功总电能
	ENERGY_DATA_MARK_F1	= 0x00050100, //(当前)费率1（尖）
	ENERGY_DATA_MARK_F2	= 0x00050200, //(当前)费率2（峰）
	ENERGY_DATA_MARK_F3	= 0x00050300, //(当前)费率3（平）
	ENERGY_DATA_MARK_F4	= 0x00050400, //(当前)费率4（谷）
	
	ENERGY_DATA_MARK_G0	= 0x00060000, //(当前)第二象限无功总电能
	ENERGY_DATA_MARK_G1	= 0x00060100, //(当前)费率1（尖）
	ENERGY_DATA_MARK_G2	= 0x00060200, //(当前)费率2（峰）
	ENERGY_DATA_MARK_G3	= 0x00060300, //(当前)费率3（平）
	ENERGY_DATA_MARK_G4	= 0x00060400, //(当前)费率4（谷）
	
	ENERGY_DATA_MARK_H0	= 0x00070000, //(当前)第三象限无功总电能
	ENERGY_DATA_MARK_H1	= 0x00070100, //(当前)费率1（尖）
	ENERGY_DATA_MARK_H2	= 0x00070200, //(当前)费率2（峰）
	ENERGY_DATA_MARK_H3	= 0x00070300, //(当前)费率3（平）
	ENERGY_DATA_MARK_H4	= 0x00070400, //(当前)费率4（谷）
	
	ENERGY_DATA_MARK_J0	= 0x00080000, //(当前)第四象限无功总电能
	ENERGY_DATA_MARK_J1	= 0x00080100, //(当前)费率1（尖）
	ENERGY_DATA_MARK_J2	= 0x00080200, //(当前)费率2（峰）
	ENERGY_DATA_MARK_J3	= 0x00080300, //(当前)费率3（平）
	ENERGY_DATA_MARK_J4	= 0x00080400, //(当前)费率4（谷）

	
	ENERGY_DATA_MARK_20	= 0x00150000, //(当前)A相正向有功电能
	ENERGY_DATA_MARK_21	= 0x00160000, //(当前)A相反向有功电能
	ENERGY_DATA_MARK_22	= 0x00170000, //(当前)A相组合无功1电能
	ENERGY_DATA_MARK_23	= 0x00180000, //(当前)A相组合无功2电能
	ENERGY_DATA_MARK_24	= 0x00190000, //(当前)A相第一象限无功电能
	ENERGY_DATA_MARK_25	= 0x001a0000, //(当前)A相第二象限无功电能
	ENERGY_DATA_MARK_26	= 0x001b0000, //(当前)A相第三象限无功电能
	ENERGY_DATA_MARK_27	= 0x001c0000, //(当前)A相第四象限无功电能

	ENERGY_DATA_MARK_40	= 0x00290000, //(当前)B相正向有功电能
	ENERGY_DATA_MARK_41	= 0x002A0000, //(当前)B相反向有功电能
	ENERGY_DATA_MARK_42	= 0x002B0000, //(当前)B相组合无功1电能
	ENERGY_DATA_MARK_43	= 0x002C0000, //(当前)B相组合无功2电能
	ENERGY_DATA_MARK_44	= 0x002D0000, //(当前)B相第一象限无功电能
	ENERGY_DATA_MARK_45	= 0x002E0000, //(当前)B相第二象限无功电能
	ENERGY_DATA_MARK_46	= 0x002F0000, //(当前)B相第三象限无功电能
	ENERGY_DATA_MARK_47	= 0x00300000, //(当前)B相第四象限无功电能

	ENERGY_DATA_MARK_60	= 0x003D0000, //(当前)C相正向有功电能
	ENERGY_DATA_MARK_61	= 0x003E0000, //(当前)C相反向有功电能
	ENERGY_DATA_MARK_62	= 0x003F0000, //(当前)C相组合无功1电能
	ENERGY_DATA_MARK_63	= 0x00400000, //(当前)C相组合无功2电能
	ENERGY_DATA_MARK_64	= 0x00410000, //(当前)C相第一象限无功电能
	ENERGY_DATA_MARK_65	= 0x00420000, //(当前)C相第二象限无功电能
	ENERGY_DATA_MARK_66	= 0x00430000, //(当前)C相第三象限无功电能
	ENERGY_DATA_MARK_67	= 0x00440000, //(当前)C相第四象限无功电能

	
}S_DLT645_ENERGY_DATA_MARKER_E;

// 表变量数据标识编码表

//电压单位 V 伏特
//电流单位 A 安培
//有功功率单位 kw 千瓦
//无功功率单位 kvar 千乏
//视在功率单位 kVA 
//功率因素单位 无单位 
//相角单位 °(度) 

typedef enum{

	VAR_DATA_MARK_00	= 0x02010100,  // A相电压
	VAR_DATA_MARK_01	= 0x02010200,  // B相电压
	VAR_DATA_MARK_02	= 0x02010300,  // C相电压
	VAR_DATA_MARK_03	= 0x0201FF00,  // 电压数据块

	VAR_DATA_MARK_04	= 0x02020100,  // A相电流
	VAR_DATA_MARK_05	= 0x02020200,  // B相电流
	VAR_DATA_MARK_06	= 0x02020300,  // C相电流
	VAR_DATA_MARK_07	= 0x0202FF00,  // 电流数据块

	VAR_DATA_MARK_08	= 0x02030000, // 瞬时总有功功率
	VAR_DATA_MARK_09	= 0x02030100, // 瞬时A相有功功率
	VAR_DATA_MARK_0A	= 0x02030200, // 瞬时B相有功功率
	VAR_DATA_MARK_0B	= 0x02030300, // 瞬时C相有功功率
	VAR_DATA_MARK_0C	= 0x0203FF00, // 瞬时有功功率数据块

	VAR_DATA_MARK_0D	= 0x02040000, // 瞬时总无功功率
	VAR_DATA_MARK_0E	= 0x02040100, // 瞬时A相无功功率
	VAR_DATA_MARK_0F	= 0x02040200, // 瞬时B相无功功率
	VAR_DATA_MARK_10	= 0x02040300, // 瞬时C相无功功率
	VAR_DATA_MARK_11	= 0x0204FF00, // 瞬时无功功率数据块

	VAR_DATA_MARK_12	= 0x02050000, // 瞬时总视在功率
	VAR_DATA_MARK_13	= 0x02050100, // 瞬时A相视在功率
	VAR_DATA_MARK_14	= 0x02050200, // 瞬时B相视在功率
	VAR_DATA_MARK_15	= 0x02050300, // 瞬时C相视在功率
	VAR_DATA_MARK_16	= 0x0205FF00, // 瞬时视在功率数据块


	VAR_DATA_MARK_17	= 0x02060000, // 总功率因数
	VAR_DATA_MARK_18	= 0x02060100, // A相功率因数
	VAR_DATA_MARK_19	= 0x02060200, // B相功率因数
	VAR_DATA_MARK_1A	= 0x02060300, // C相功率因数
	VAR_DATA_MARK_1B	= 0x0206FF00, // 功率因数数据块

	VAR_DATA_MARK_1C	= 0x02070100, // A相相角
	VAR_DATA_MARK_1D	= 0x02070200, // B相相角
	VAR_DATA_MARK_1E	= 0x02070300, // C相相角
	VAR_DATA_MARK_1F	= 0x02070400, // 相角数据块

	VAR_DATA_MARK_20	= 0x01010000, // (当前)正向有功总最大需量及发生时间
	VAR_DATA_MARK_21	= 0x01020000, // (当前)反向有功总最大需量及发生时间
	VAR_DATA_MARK_22	= 0x01030000, // (当前)组合无功1总最大需量及发生时间
	VAR_DATA_MARK_23	= 0x01040000, // (当前)组合无功2总最大需量及发生时间
	
	
}S_DLT645_VAR_DATA_MARKER_E;

#pragma pack(1)

//控制域
typedef struct{
	mdBYTE C_CODE   :5; //D0-D4 功能码
	mdBYTE FCK  	  :1; // 0 无后续数据帧，1有后续数据帧
	mdBYTE ACK      :1; //ACK = 0 从站正确应答 ACK = 1 从站异常应答
	mdBYTE DIR      :1; //传输方向位DIR 0主站发出的下行报文  1 从站发出的应答帧
}S_DLT645_C_t;

//地址域
typedef struct{
	mdBYTE  addr[6];
}S_DLT645_A_t;


typedef struct {
    mdBYTE 		  btPre;     //起始字符      1
    S_DLT645_A_t  xAddr;     //地址域        6
    mdBYTE        btMid;     //起始字符		 1
    S_DLT645_C_t  xCon; 	 //控制域        1
    mdBYTE		  btLen;	 //数据域长度    1
    pbyte         pData;     //数据域 		 n
    mdBYTE        btCheck;   //帧校验和      1
    mdBYTE        btEom;     //结束字符      1   
} S_DLT645_Package_t;


typedef struct{
	mdBYTE YY; // 年
	mdBYTE MM; //月
	mdBYTE DD; //日
	mdBYTE hh; //时   
	mdBYTE mm; //分  
	mdBYTE ss; //秒 
}S_DLT645_Time_t;

//通讯速率状态字
typedef enum{
	BAUD_600BPS   = (1<<1),
	BAUD_1200BPS  = (1<<2),
	BAUD_2400BPS  = (1<<3),
	BAUD_4800BPS  = (1<<4),
	BAUD_9600BPS  = (1<<5),
	BAUD_19200BPS = (1<<6),
}S_DLT645_BAUD_t;

typedef struct {
    int index;
    S_DLT645_A_t    addr;
	mdUINT32        op;
	float           val;
} S_DLT645_Result_t;

#pragma pack()

#ifdef __cplusplus  
extern "C" {  
#endif  

//void vDlt645RequestCheckTime(S_DLT645_A_t xaddr, S_DLT645_Time_t *ptime);    //广播对时
//void vDlt645ReadAddr(S_DLT645_A_t xaddr);    //读取设备通讯地址
//void vDlt645SetBaud(S_DLT645_A_t xaddr, S_DLT645_BAUD_t baud);     //设置通讯波特率 默认2400
rt_bool_t bDlt645ReadData(int index, S_DLT645_A_t xaddr, mdUINT32 DataMarker, int timeout, S_DLT645_Result_t *result);

typedef void (*pDlt645SendDataFun)(int index,mdBYTE *pdata,mdBYTE len);

//typedef void (*pDlt645Debug)(const char *fmt,...);
//extern pDlt645Debug DLT645_debug;

extern const unsigned int g_ENERGY_DATA_MARKER_E[];
extern const unsigned int g_VAR_DATA_MARKER_E[];

mdBOOL Dlt645_PutBytes(int index, mdBYTE *pBytes, mdUSHORT usBytesLen); //解析函数，每收到一包数据调用一次
void vDlt645ResponAddr();    //从机反馈设备地址(用于测试)

void dlt645_init(int index, pDlt645SendDataFun sendfunc);

#ifdef __cplusplus  
}  
#endif 

#endif
