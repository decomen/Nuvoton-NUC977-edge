/**************************************************************************************************
  Revised:        2014-12-04
  Author:         Zhu Jie . Jay . Sleepace
**************************************************************************************************/

/* ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 *   重定义
 * ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 */

#ifndef __MD_TYPEDEF_H__
#define __MD_TYPEDEF_H__

#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define MD_ABS(_x)      ((_x)>=0?(_x):(-_x))
#define MD_OFS(_t, _m)  (unsigned int)(&(((_t *)0)->_m))
#define MD_IS_UPPER(_c) (((_c)>='A')&&((_c)<='Z'))
#define MD_IS_LOWER(_c) (((_c)>='a')&&((_c)<='z'))
#define MD_IS_DIGIT(_c) (((_c)>='0')&&((_c)<='9'))


#define mdNULL      0
#define mdTRUE      1
#define mdFALSE     0

typedef int8_t          mdINT8;
typedef int16_t         mdINT16;
typedef int32_t         mdINT32;
typedef int64_t         mdINT64;

typedef uint8_t         mdUINT8;
typedef uint16_t        mdUINT16;
typedef uint32_t        mdUINT32;
typedef uint64_t        mdUINT64;

// Special numbers

typedef mdUINT8             mdBOOL;
typedef mdUINT16	    mdWORD;
typedef mdUINT32	    mdDWORD;

// Unsigned numbers
typedef unsigned char     mdBYTE;
typedef unsigned char     byte;
typedef unsigned char     *pbyte;
typedef unsigned char     u8;
typedef unsigned char     mdUCHAR;

typedef unsigned short  mdUSHORT;
typedef unsigned short  u16;
typedef unsigned short  ushort;
// int, long 平台相关
typedef unsigned int    mdUINT;
typedef unsigned int    u32;
typedef unsigned long   mdULONG;

// Signed numbers
typedef signed char     mdCHAR;
typedef signed short    mdSHORT;
typedef signed short    i16;
// int, long 平台相关
typedef signed int      mdINT;
typedef signed int      i32;
typedef signed long     mdLONG;

// decimal
typedef float           mdFLOAT;
typedef double          mdDOUBLE;

typedef enum {
    MD_ENOERR,      // 正常
    MD_EPARAM,      // 参数异常
    MD_EOVERFLOW,   // 越界
    MD_ENOINIT,     // 未初始化

    MD_EBUSY,       // 忙
    MD_ETIMEOUT,    // 超时

    MD_EUNKNOWN     // 未知错误
} eMDErrorCode;


typedef enum {
    MD_VER_TYPE_INVALID     = 0x00,     //无效
    MD_VER_TYPE_DEBUG       = 0x01,     //调试版
    MD_VER_TYPE_BETA        = 0x02,     //测试版
    MD_VER_TYPE_RELEASE     = 0x03,     //发行版本
} eMDVerType;

typedef enum {
    MD_IDE_TYPE_IAR         = 0x00,     //IAR
    MD_IDE_TYPE_KEIL        = 0x01,     //KEIL
    MD_IDE_TYPE_GCC         = 0x02,     //GCC
} eMDIDEType;

typedef enum {
    
    DM_MODEL_NONE = 0x00,   //未知

	DM_S100D     =    0x0100,  				 //数字量模块
	DM_S101D_N   =    0x0101,  				 //数字量(带NB-IOT)
	
	DM_S100A     =    0x0106,  				 //8路模拟量模块
	DM_S101A_N   =    0x0107,  				 //8路模拟量模块(带NB-IOT)
	
        DM_S200L   =    0x010D, 			  //井盖传感器(NB-IOT版本)
	DM_S200N   =    0x010E, 			  //NB-IOT协议适配模块
	
	
    DM_A300N   =       0x0200, 			 //NB-IOT DTU
    DM_A401F   =	   0x0201, 			 //多功能适配终端
    DM_A503H   =	   0x0202, 			//远程传输RTU终端  
    DM_A400H   =       0X0203, 			//4G多功能终端512M
	DM_A400L   =	   0X0204,         //4G多功能终端256M
	
	DM_C100G   =	0x0300,  //IOT-BOX物联网关（972）
	DM_C200G   =	0x0301,  //IOT-BOX物联网关（A7）
	DM_C300T   =	0x0302,  //场站边缘计算终端(A7)
	DM_C400D   =	0x0303,  //一体化显示屏（7寸）（B/S）
	DM_C401D   =	0x0304,  //一体化显示屏（7寸）（C/S）
	DM_C500D   =	0x0305,  //一体化显示屏（10.1寸）（B/S）
	DM_C501D   =	0x0306,  //一体化显示屏（10.1寸）（C/S）


	MD_MODEL_UNKNOWN        = 0xFFFF,
	
} eDMModel;


//产品型号
#define DM_S100D_PRODUCT_NAME		"DM-S100D"		//"数字量模块
#define DM_S100D_N_PRODUCT_NAME		"DM-S100D-N"	//"数字量模快(带NB-IOT无线上传)
#define DM_S100A_PRODUCT_NAME		"DM-S100A"		//"8路模拟量输入模块
#define DM_S100A_N_PRODUCT_NAME		"DM-S100A-N"		//"8路模拟量输入模块(带NB-IOT无线上传)
#define DM_S200L_PRODUCT_NAME		"DM-S200L"		//"井盖传感器
#define DM_S300N_PRODUCT_NAME		"DM-S300N"		//"NB-IOT协议适配模块
#define DM_A300N_PRODUCT_NAME		"DM-A300N"		//"NB-IOT DTU
#define DM_A401F_PRODUCT_NAME		"DM-A401F"		//"通讯适配器
#define DM_A503H_PRODUCT_NAME		"DM-A503H"		//"远程传输DM-RTU终端标准版本
#define DM_A400H_PRODUCT_NAME		"DM-A400H"		//"4G多功能终端(512Mbyte)
#define DM_A400L_PRODUCT_NAME		"DM-A400L"		//"4G多功能终端(256Mbyte)
     
         

#define BIN_MAGIC_WORD      (0xF57BA045930AE418ULL)
typedef struct xMD_BIN_INFO {
    mdINT64   ullMagic;     // 8字节魔数(指纹信息,防止读取错误信息)
    mdBYTE    btIDEType;    // 开发环境
    mdUSHORT  usModel;      // 设备类型
    mdBYTE    btVerType;    // 固件类型
    mdUSHORT  usVerCode;    // 固件版本
} MD_BinInfo_t;

typedef enum 
{
    SOFT_VER_TYPE_MODULE,        //模块软件
    SOFT_VER_TYPE_PRODUCT,       //产品软件
    
}MD_SOFT_VER_TYPE;


//ver_str:  输出字符串
//_p_name:  产品型号名称
//_sVer:    软件版本
//_hVer:    硬件版本
//_VerType: 版本类型 eMDVerType
//_date:  180503    //2018年5月3号
//_soft_ver:  软件版本类型 M 模块软件版本  P产品软件版本


#define DM_VER_FORMAT(ver_str, _pName,_sVer,hVer,_VerType,_date,_soft_type) do {\
    char *ver_type =( _VerType==MD_VER_TYPE_DEBUG) ?"debug":(_VerType==MD_VER_TYPE_BETA?"beta":"release");\
    char *soft_ver_type = (_soft_type == SOFT_VER_TYPE_MODULE)?"M":"P";\
    sprintf(ver_str,"%s_V%d.%02d_%s_%s_%s%d.%02d",_pName,_sVer/100,_sVer%100,_date,ver_type,soft_ver_type,hVer/100,hVer%100);\
}while(0);

#endif

