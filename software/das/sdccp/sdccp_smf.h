#ifndef __SDCCP_SMF_H__
#define __SDCCP_SMF_H__

#include "varmanage.h"

#define SDCCP_SMF_BUF_SIZE      (1024)
#define SDCCP_SMF_PARSE_STACK   (2048)      //解析任务内存占用

#define SMF_EOM1             '\r'  /* 截止字符 \r\n */
#define SMF_EOM2             '\n'  

/*

#Time Date Sensor1 Sensor2 Sensor3 Sensor4 Sensor5 Sensor6 Sensor7 internal_Temp Pressure RH% Sensor_temp PM1 PM2.5 PM10 unitID Unit_Status\r\n

Example：
2016-11-07 12:24:32,NO2,-241.136,O3,67.286,SO2,202.104,CO,0.499,N/A,-99999.000,N/A,-99999.000,N/A,-99999.000,intt,27.756,pres,101.856,RH,25.506,
sent,26.631,PM1,-99999.000,PM2.5,0.005,PM10,0.005,
thermo-grid-506583c54187,0\r\n


•Time – 读数保存时间
•Date – 读数保存日期
•Sensor1:Sensor7 – 传感器名称和读数。如果传感器未安装，则记录“NA -99999”，如果传感器损坏，则显示“SO2 -99999”  
•internal_temp – 内部温度传感器名称和读数
•Pressure – 仪器机箱内压力传感器名称和读数
•RH% - 仪器机箱内相对湿度传感器名称和读数
•Sensor temperature – 仪器机箱内温度传感器名称和读数.
•PM1 – PM1及读数
•PM2.5 -  PM2.5及读数
•PM10 -  PM10及读数
•unitID – 仪器ID
•Status = c
•一条信息以回车/换行结束 (CR/LF)

*/

#define COEF_AIR    (22.4)
#define COEF_AIR_1  (24.5)

#define  NO2_S 46 
#define  O3_S  48 
#define  SO2_S 64 
#define  NO_S  30 
#define  NOx_S 46 
#define  CO_S  28 

#define S02_CHANG(_val) (_val/1000.0*SO2_S/COEF_AIR)
#define O3_CHANG(_val)  (_val/1000.0*O3_S/COEF_AIR)
#define NO2_CHANG(_val) (_val/1000.0*NO2_S/COEF_AIR)
#define CO_CHANG(_val)  (_val*CO_S/COEF_AIR)

#define PM2_5_CHANG(_val)  (_val/1000.0)
#define PM10_CHANG(_val)   (_val/1000.0)

#define SENSOR_NO2_INDEX 0 
#define SENSOR_O3_INDEX  1 
#define SENSOR_SO2_INDEX 2 
#define SENSOR_CO_INDEX  3 

#define SENSOR_NUM_MAX 7

typedef struct
{
  char  name[16];
  float fval;
} s_data_t;

typedef struct 
{
    rt_uint8_t time[32];             //当前时间 2016-11-07 12:24:32
    s_data_t Sensor[SENSOR_NUM_MAX]; //sensor1-sensor7 传感器名称和读数
    s_data_t inter_temp;             // 仪器机箱内温度传感器名称和读数.
    s_data_t press;                  //仪器机箱内压力传感器名称和读数
    s_data_t RH;                    //仪器机箱内相对湿度传感器名称和读数
    s_data_t sensor_temp;          //仪器机箱内温度传感器名称和读数.
    s_data_t PM1;                 //   PM1及读数
    s_data_t PM2_5;                 //   PM2.5及读数
    s_data_t PM10;                 //   PM10及读数
    rt_uint8_t unitId[32];            // 仪器ID
    int status;            //仪器状态。 十进制整数，不同值标识不同错误信息；0表示一切正常，所有读数有效
} s_instrumentData_t;  

typedef enum {
    SMF_OP_NO2          = 1, 
    SMF_OP_O3           = 2, 
    SMF_OP_SO2          = 3,
    SMF_OP_CO           = 4,
    SMF_OP_PM2_5        = 5,
    SMF_OP_PM10         = 6,
    SMF_OP_T            = 7,    //机箱内部温度
    SMF_OP_H            = 8,    //机箱内部湿度
    SMF_OP_P            = 9,   //机箱内部压力
    SMF_OP_ST           = 10,   //传感器温度
} S_SMF_C_OP_E;


#ifdef __cplusplus  
extern "C" {  
#endif  

void smf_open(int index);
void smf_close(int index);
void smf_new_cli(int index, int cli);
void smf_close_cli(int index, int cli);
void smf_start_work(int index);
void smf_exit_work(int index);
rt_bool_t smf_read_data(int index, int timeout, s_instrumentData_t *result);
rt_bool_t smf_end_read_data(int index);
rt_bool_t smf_put_bytes(int index, int cli, rt_uint8_t *data, int len);

#ifdef __cplusplus  
}  
#endif 

#endif

