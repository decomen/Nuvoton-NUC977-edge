#ifndef __AD7689_H__
#define __AD7689_H__

#define CHANNEL_CHANGE_TIME (250)  //MS

typedef struct {
	int usEngUnit;     //工程量，原始ADC值
	float fMeasure;             //电参数实测值
	int usBinaryUnit;  //二进制补码 ， 目前二进制补码等于原始ADC值
	float fPercentUnit;         //百分比
	float fMeterUnit;           //仪表测量数据
}s_AdcValue_t;

typedef struct{
	float  fMeterValue;   //仪表测量值。
	int usAdcValue;    //0-65535
}s_CalValue_t;

//量程
typedef struct {
	s_CalValue_t xMin;
	s_CalValue_t xMiddle;
	s_CalValue_t xMax;
	float factor;
}s_CalEntry_t;

//修正系数
typedef struct{
	float factor;
}s_CorrectionFactor_t;

typedef enum{
	Unit_Eng = 0x00,
	Unit_Binary,
	Unit_Percent,
	Unit_Meter,
	
	Unit_Max,
}eUnitType_t;

typedef enum{
	Range_4_20MA = 0x00,
	Range_0_20MA,
	Range_0_5V,
	Range_1_5V,	
	
	RANG_TYPE_MAX
}eRangeType_t;

typedef enum{
	SET_MIN = 0x00,
	SET_MIDDLE ,
	SET_MAX ,
	SET_ALL,
}eCalCountType_t;

typedef enum {
	ADC_CHANNEL_0 = 0x00,
	ADC_CHANNEL_1,
	ADC_CHANNEL_2,
	ADC_CHANNEL_3,
	ADC_CHANNEL_4,
	ADC_CHANNEL_5,
	ADC_CHANNEL_6,
	ADC_CHANNEL_7, 
	ADC_CHANNEL_NUM, 
} ADC_CHANNEL_E;


//0通道 量程
#define DEFAULT_CAL_4_20MA_MIN_METER_VAL      (4.0)
#define DEFAULT_CAL_4_20MA_MIN_ADC_VAL        (0)

#define DEFAULT_CAL_4_20MA_MIDDLE_METER_VAL     (12.0)
#define DEFAULT_CAL_4_20MA_MIDDLE_ADC_VAL     (0x7FFFF)

#define DEFAULT_CAL_4_20MA_MAX_METER_VAL        (20.0)
#define DEFAULT_CAL_4_20MA_MAX_ADC_VAL        (0xFFFFF)


//0-20MA 量程 计算理论值
#define DEFAULT_CAL_0_20MA_MIN_METER_VAL      (4.0)
#define DEFAULT_CAL_0_20MA_MIN_ADC_VAL        (209715)

#define DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL     (10.0)
#define DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL     (524287)

#define DEFAULT_CAL_0_20MA_MAX_METER_VAL     (18.0)
#define DEFAULT_CAL_0_20MA_MAX_ADC_VAL     (943717)


//#define DEFAULT_CAL_0_20MA_MAX_METER_VAL        (20.0)
//#define DEFAULT_CAL_0_20MA_MAX_ADC_VAL        (0xFFFFF)




//0-5V 量程
#define DEFAULT_CAL_0_5V_MIN_METER_VAL      (0)
#define DEFAULT_CAL_0_5V_MIN_ADC_VAL        (0)

#define DEFAULT_CAL_0_5V_MIDDLE_METER_VAL     (2.5)
#define DEFAULT_CAL_0_5V_MIDDLE_ADC_VAL     (0x7FFFF)

#define DEFAULT_CAL_0_5V_MAX_METER_VAL        (5)
#define DEFAULT_CAL_0_5V_MAX_ADC_VAL        (0xFFFFF)


//1-5V 量程
#define DEFAULT_CAL_1_5V_MIN_METER_VAL      (1)
#define DEFAULT_CAL_1_5V_MIN_ADC_VAL        (0)

#define DEFAULT_CAL_1_5V_MIDDLE_METER_VAL     (3.0)
#define DEFAULT_CAL_1_5V_MIDDLE_ADC_VAL     (0x7FFFF)

#define DEFAULT_CAL_1_5V_MAX_METER_VAL        (5.0)
#define DEFAULT_CAL_1_5V_MAX_ADC_VAL        (0xFFFFF)



void vAd7707Init(void);
void vAdcPowerEnable(void); //ADC有一个电源使能脚，在使用之前请使能电源
void vAdcPowerDisable(void);


//获取ADC值，目前factor可以传入0，暂时没有使用该参数 ，使用该函数获取ADC值之前，请进行校准，校准后请保存到SPIFLASH内，每次上电重新读取。
//没有校验，会用默认的校验值。会影响精度。校验请调用 vSetCalValue 函数设置校验值。

//校验方法: 在ADC通道口，接入一个高精度固定的量程值如(4-20mA) ,分别接入 4MA 12MA 20MA , 通过 vGetAdcValue获取到ADC原始值，分别设置对应的值

// eHardwareRangeType  硬件量程，表示硬件上面可以输入的量程范围，目前我们硬件是可以测量0-20ma
// eSoftwareType       软件量程，软件上面限制量程范围，比如实际可以测量0-20ma,但网页上面只能测量4-20MA. 低于4MA,网页就提示超量程，且此时工程量值和测量值都为0

void vGetAdcValue(ADC_CHANNEL_E chnn , eRangeType_t eHardwareRangeType, eRangeType_t eSoftwareType, float ext_min, float ext_max, s_CorrectionFactor_t factor, s_AdcValue_t *pVal);
void vGetAdcValueByDefaultParm(ADC_CHANNEL_E chnn , eRangeType_t eHardwareRangeType, eRangeType_t eSoftwareType, float ext_min, float ext_max, s_CorrectionFactor_t factor, s_AdcValue_t *pVal);

void vGetCalValue(ADC_CHANNEL_E chnn , s_CalEntry_t *pCalEntry);
void vSetCalValue(ADC_CHANNEL_E chnn ,eCalCountType_t eCalType, s_CalEntry_t *pCalEntry);
extern s_CalEntry_t gCalEntry[ADC_CHANNEL_NUM];
extern s_CalEntry_t gCalEntryBak[ADC_CHANNEL_NUM];
void SetAdcCalCfgDefault();


#endif
