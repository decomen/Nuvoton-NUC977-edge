#include <board.h>
#include <ad7689.h>


//0-20MA 量程 计算理论值
#define DEFAULT_CAL_0_20MA_MIN_METER_VAL      (4.0)
#define DEFAULT_CAL_0_20MA_MIN_ADC_VAL        (13107)

#define DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL     (10.0)
#define DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL     (32768)

#define DEFAULT_CAL_0_20MA_MAX_METER_VAL     (18.0)
#define DEFAULT_CAL_0_20MA_MAX_ADC_VAL     (58982)

void vAdcPowerEnable(void) 
{ 
}

void vAdcPowerDisable(void) 
{ 
}

void vAd7689Init(void) 
{
   // SetAdcCalCfgDefault();
}

const s_CalEntry_t gDefaultCalEntry[ADC_CHANNEL_NUM] = {
    [ADC_CHANNEL_0] = {
        .xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},    
        .xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},    
        .xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},    
        .factor = 0.0f,
    },
    [ADC_CHANNEL_1] = {
        .xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},    
        .xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},    
        .xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},    
        .factor = 0.0f,
    },
    [ADC_CHANNEL_2] = {
        .xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},    
        .xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},    
        .xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},    
        .factor = 0.0f,
    },
    [ADC_CHANNEL_3] = {
        .xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},    
        .xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},    
        .xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},    
        .factor = 0.0f,
    },
    [ADC_CHANNEL_4] = {
        .xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},    
        .xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},    
        .xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},    
        .factor = 0.0f,
    },
    [ADC_CHANNEL_5] = {
        .xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},    
        .xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},    
        .xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},    
        .factor = 0.0f,
    },
    [ADC_CHANNEL_6] = {
        .xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},    
        .xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},    
        .xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},    
        .factor = 0.0f,
    },
    [ADC_CHANNEL_7] = {
        .xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},    
        .xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},    
        .xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},    
        .factor = 0.0f,
    },
};

s_CalEntry_t gCalEntry[ADC_CHANNEL_NUM];
s_CalEntry_t gCalEntryBak[ADC_CHANNEL_NUM];

void SetAdcCalCfgDefault()
{
	for(int i = 0; i < ADC_CHANNEL_NUM;i++){
		gCalEntry[i] = gDefaultCalEntry[i];
		gCalEntryBak[i]= gDefaultCalEntry[i];
	}
}

// A = (D - D0) *　(Am - A0) / (Dm-D0) + A0
// A 表示测量值
// D 表示ADC值

//#define _ADC_GROUP_SZ           (CHANNEL_QUEUE_SZ/100)

s_AdcCfgPram gAdcCfgPram = {
	CHANNEL_QUEUE_SZ,
	CHANNEL_QUEUE_SZ/100,
	10,
	ADC_MODE_NORMAL
};


typedef struct {
    rt_base_t nFront;
    rt_base_t nRear;
    rt_uint16_t pData[100+1];
} _Adc_Queue_t;

static int _adc_group[ADC_CHANNEL_NUM] = {0};
static int _adc_count_group[ADC_CHANNEL_NUM] = {0};
static _Adc_Queue_t _adc_queue[ADC_CHANNEL_NUM];

static unsigned long AD7689_spi(ADC_CHANNEL_E chnn)
{
    float value = 0;
    das_do_get_ai_value((int)chnn, &value);
    return (unsigned long)value;
}

void vGetAdcValue(ADC_CHANNEL_E chnn , eRangeType_t eHardwareRangeType, eRangeType_t eSoftwareType, float ext_min, float ext_max, s_CorrectionFactor_t factor, s_AdcValue_t *pVal)
{ 
    unsigned short usAdcVal = AD7689_spi(chnn) & 0xFFFF;

  //  printf("chnn:%d, adc_val: %d\r\n",chnn,usAdcVal);
    
    _adc_count_group[chnn]++;
    _adc_group[chnn]+=usAdcVal;
    if (_adc_count_group[chnn] >= gAdcCfgPram.AdcGroupSz) {
        bQueuePush(_adc_queue[chnn], (_adc_group[chnn]/_adc_count_group[chnn]));
        _adc_group[chnn] = 0;
        _adc_count_group[chnn] = 0;
    }
    {
        int len = nQueueLen(_adc_queue[chnn]);
        if (len > 0) {
            rt_uint32_t sum = 0;
            for (int i = 0; i < len; i++) {
                sum += xQueueGet(_adc_queue[chnn], i);
            }
            usAdcVal = sum / len;
        }
    }
    
    float fPercent = 0 , fMeterVal = 0;

    switch(eSoftwareType) {
		case Range_0_20MA:{
		    if( Range_0_20MA == eHardwareRangeType ) {
		    }
	        break;
		}
		case Range_4_20MA:{
			 if( Range_0_20MA == eHardwareRangeType ) {
			 	if(usAdcVal < (gCalEntry[chnn].xMiddle.usAdcValue/gCalEntry[chnn].xMiddle.fMeterValue) * 4.0 ){
					usAdcVal = 0;
				}
		     }
		    }
	        break;
		
		case Range_1_5V:{
			/*
		    if( Range_0_5V == eHardwareRangeType ) {
		        if(usAdcVal < (gCalEntry[chnn].xMiddle.usAdcValue/gCalEntry[chnn].xMiddle.fMeterValue) * 4.0 ){
					usAdcVal = 0;
				}
		    }*/
	        break;
		}
		case Range_0_5V:{
		    if( Range_0_5V == eHardwareRangeType ) {
		    }
	        break;
		}
    }
    
    
	pVal->usEngUnit = usAdcVal;
	pVal->usBinaryUnit = usAdcVal;
	
    switch(eSoftwareType){
	case Range_0_20MA:
	case Range_4_20MA: {
			if (usAdcVal > 0 && usAdcVal <= (32768)) {
				unsigned int TmpVal = 0;
				if (usAdcVal >= gCalEntry[chnn].xMin.usAdcValue) {
					TmpVal = usAdcVal - gCalEntry[chnn].xMin.usAdcValue;
					
					fMeterVal =  gCalEntry[chnn].xMin.fMeterValue +	
					   ( (TmpVal) * (gCalEntry[chnn].xMiddle.fMeterValue -  gCalEntry[chnn].xMin.fMeterValue) / 
					    ( gCalEntry[chnn].xMiddle.usAdcValue - gCalEntry[chnn].xMin.usAdcValue ) );
					
				} else {
					TmpVal =  gCalEntry[chnn].xMin.usAdcValue - usAdcVal;

					fMeterVal =  gCalEntry[chnn].xMin.fMeterValue -	
					   ( (TmpVal) * (gCalEntry[chnn].xMiddle.fMeterValue -  gCalEntry[chnn].xMin.fMeterValue) / 
					    ( gCalEntry[chnn].xMiddle.usAdcValue - gCalEntry[chnn].xMin.usAdcValue ) );
				}
				
				
				           
			} else if(usAdcVal > (32768)) {
				unsigned int TmpVal = 0;
				if( usAdcVal >= gCalEntry[chnn].xMiddle.usAdcValue){
					TmpVal = usAdcVal - gCalEntry[chnn].xMiddle.usAdcValue;
					
					fMeterVal =  gCalEntry[chnn].xMiddle.fMeterValue +	
					   ( (TmpVal) *(gCalEntry[chnn].xMax.fMeterValue -  gCalEntry[chnn].xMiddle.fMeterValue) / 
					    ( gCalEntry[chnn].xMax.usAdcValue - gCalEntry[chnn].xMiddle.usAdcValue ) );
					
				}else {
					TmpVal =  gCalEntry[chnn].xMiddle.usAdcValue - usAdcVal;

					fMeterVal =  gCalEntry[chnn].xMiddle.fMeterValue -	
					   ( (TmpVal) *(gCalEntry[chnn].xMax.fMeterValue -  gCalEntry[chnn].xMiddle.fMeterValue) / 
					    ( gCalEntry[chnn].xMax.usAdcValue - gCalEntry[chnn].xMiddle.usAdcValue ) );
				}
				            
			} else {
                fMeterVal = 0.0f;
            }
            
			//fMeterVal -= gCalEntry[chnn].factor;
			pVal->fMeasure = fMeterVal;
			if(eSoftwareType == Range_4_20MA) {
				if (fMeterVal>=4) {
					pVal->fPercentUnit = (fMeterVal-4)/(20-4)*100.0f;
					pVal->fMeterUnit = (ext_max-ext_min)*(fMeterVal-4)/(20-4)+ext_min+factor.factor;
				} else {
					pVal->fPercentUnit = 0;
					pVal->fMeterUnit = ext_min+factor.factor;
				}
			} else if(eSoftwareType == Range_0_20MA) {
				pVal->fPercentUnit = (float)((pVal->fMeasure)/20.0f * 100.0f);
			    pVal->fMeterUnit = (ext_max-ext_min)*(pVal->fMeasure)/20.0f+ ext_min + factor.factor;
			}
			break;
		}
        
         case Range_0_5V:
		 	break;
        case Range_1_5V:
			break;
		//case Range_0_20MA:{
		//	break;
			//break;
		//}
	}


}

void vGetAdcValueCal(ADC_CHANNEL_E chnn , eRangeType_t eHardwareRangeType, eRangeType_t eSoftwareType, float ext_min, float ext_max, s_CorrectionFactor_t factor, s_AdcValue_t *pVal)
{ 
    unsigned short usAdcVal = AD7689_spi(chnn) & 0xFFFF;

    _adc_count_group[chnn]++;
    _adc_group[chnn]+=usAdcVal;
    if (_adc_count_group[chnn] >= gAdcCfgPram.AdcGroupSz) {
        bQueuePush(_adc_queue[chnn], (_adc_group[chnn]/_adc_count_group[chnn]));
        _adc_group[chnn] = 0;
        _adc_count_group[chnn] = 0;
    }
    {
        int len = nQueueLen(_adc_queue[chnn]);
        if (len > 0) {
            rt_uint32_t sum = 0;
            for (int i = 0; i < len; i++) {
                sum += xQueueGet(_adc_queue[chnn], i);
            }
            usAdcVal = sum / len;
        }
    }
    
    float fPercent = 0 , fMeterVal = 0;

    switch(eSoftwareType) {
		case Range_0_20MA:{
		    if( Range_0_20MA == eHardwareRangeType ) {
		    }
	        break;
		}
		case Range_4_20MA:{
			 if( Range_0_20MA == eHardwareRangeType ) {
			 	/*if(usAdcVal < (gCalEntry[chnn].xMiddle.usAdcValue/gCalEntry[chnn].xMiddle.fMeterValue) * 4.0 ){
					usAdcVal = 0;
				}*/
		     }
		    }
	        break;
		
		case Range_1_5V:{
			/*
		    if( Range_0_5V == eHardwareRangeType ) {
		        if(usAdcVal < (gCalEntry[chnn].xMiddle.usAdcValue/gCalEntry[chnn].xMiddle.fMeterValue) * 4.0 ){
					usAdcVal = 0;
				}
		    }*/
	        break;
		}
		case Range_0_5V:{
		    if( Range_0_5V == eHardwareRangeType ) {
		    }
	        break;
		}
    }
    
    
	pVal->usEngUnit = usAdcVal;
	pVal->usBinaryUnit = usAdcVal;
	
    switch(eSoftwareType){
	case Range_4_20MA: {
			if (usAdcVal > 0 && usAdcVal <= (32768)) {
				unsigned int TmpVal = 0;
				if (usAdcVal >= gCalEntry[chnn].xMin.usAdcValue) {
					TmpVal = usAdcVal - gCalEntry[chnn].xMin.usAdcValue;
					
					fMeterVal =  gCalEntry[chnn].xMin.fMeterValue +	
					   ( (TmpVal) * (gCalEntry[chnn].xMiddle.fMeterValue -  gCalEntry[chnn].xMin.fMeterValue) / 
					    ( gCalEntry[chnn].xMiddle.usAdcValue - gCalEntry[chnn].xMin.usAdcValue ) );
					
				} else {
					TmpVal =  gCalEntry[chnn].xMin.usAdcValue - usAdcVal;

					fMeterVal =  gCalEntry[chnn].xMin.fMeterValue -	
					   ( (TmpVal) * (gCalEntry[chnn].xMiddle.fMeterValue -  gCalEntry[chnn].xMin.fMeterValue) / 
					    ( gCalEntry[chnn].xMiddle.usAdcValue - gCalEntry[chnn].xMin.usAdcValue ) );
				}
				
				
				           
			} else if(usAdcVal > (32768)) {
				unsigned int TmpVal = 0;
				if( usAdcVal >= gCalEntry[chnn].xMiddle.usAdcValue){
					TmpVal = usAdcVal - gCalEntry[chnn].xMiddle.usAdcValue;
					
					fMeterVal =  gCalEntry[chnn].xMiddle.fMeterValue +	
					   ( (TmpVal) *(gCalEntry[chnn].xMax.fMeterValue -  gCalEntry[chnn].xMiddle.fMeterValue) / 
					    ( gCalEntry[chnn].xMax.usAdcValue - gCalEntry[chnn].xMiddle.usAdcValue ) );
					
				}else {
					TmpVal =  gCalEntry[chnn].xMiddle.usAdcValue - usAdcVal;

					fMeterVal =  gCalEntry[chnn].xMiddle.fMeterValue -	
					   ( (TmpVal) *(gCalEntry[chnn].xMax.fMeterValue -  gCalEntry[chnn].xMiddle.fMeterValue) / 
					    ( gCalEntry[chnn].xMax.usAdcValue - gCalEntry[chnn].xMiddle.usAdcValue ) );
				}
				            
			} else {
                fMeterVal = 0.0f;
            }
            
			//fMeterVal -= gCalEntry[chnn].factor;
			pVal->fMeasure = fMeterVal;
			if (fMeterVal>=4) {
			    pVal->fPercentUnit = (fMeterVal-4)/(20-4);
			    pVal->fMeterUnit = (ext_max-ext_min)*(fMeterVal-4)/(20-4)+ext_min+factor.factor;
			} else {
			    pVal->fPercentUnit = 0;
			    pVal->fMeterUnit = ext_min+factor.factor;
			}
			break;
		}
        
         case Range_0_5V:
		 	break;
        case Range_1_5V:
			break;
		case Range_0_20MA:{
			break;

		}
	}

}



//用于校准，获取ADC值，使用理论值进行计算

void vGetAdcValueTest(ADC_CHANNEL_E chnn , eRangeType_t eHardwareRangeType, eRangeType_t eSoftwareType, float ext_min, float ext_max, s_CorrectionFactor_t factor, s_AdcValue_t *pVal)
{ 
    unsigned short usAdcVal = AD7689_spi(chnn) & 0xFFFF;
  //  printf("chan:%d, adc_val: %d\n",chnn, usAdcVal);

    _adc_count_group[chnn]++;
    _adc_group[chnn]+=usAdcVal;
    if (_adc_count_group[chnn] >= gAdcCfgPram.AdcGroupSz) {
        bQueuePush(_adc_queue[chnn], (_adc_group[chnn]/_adc_count_group[chnn]));
        _adc_group[chnn] = 0;
        _adc_count_group[chnn] = 0;
    }
    {
        int len = nQueueLen(_adc_queue[chnn]);
        if (len > 0) {
            rt_uint32_t sum = 0;
            for (int i = 0; i < len; i++) {
                sum += xQueueGet(_adc_queue[chnn], i);
            }
            usAdcVal = sum / len;
        }
    }
    
    float fPercent = 0 , fMeterVal = 0;

    switch(eSoftwareType) {
		case Range_0_20MA:{
		    if( Range_0_20MA == eHardwareRangeType ) {
		    }
	        break;
		}
		case Range_4_20MA:{
			 if( Range_0_20MA == eHardwareRangeType ) {
		     }
		    }
	        break;
		
		case Range_1_5V:{
			/*
		    if( Range_0_5V == eHardwareRangeType ) {
		        if(usAdcVal < (gCalEntry[chnn].xMiddle.usAdcValue/gCalEntry[chnn].xMiddle.fMeterValue) * 4.0 ){
					usAdcVal = 0;
				}
		    }*/
	        break;
		}
		case Range_0_5V:{
		    if( Range_0_5V == eHardwareRangeType ) {
		    }
	        break;
		}
    }
    
    
	pVal->usEngUnit = usAdcVal;
	pVal->usBinaryUnit = usAdcVal;
	
    switch(eSoftwareType){
	case Range_4_20MA: {
			if (usAdcVal > 0 && usAdcVal <= (32768)) {
				unsigned int TmpVal = 0;
				if (usAdcVal >= gDefaultCalEntry[chnn].xMin.usAdcValue) {
					TmpVal = usAdcVal - gDefaultCalEntry[chnn].xMin.usAdcValue;
					
					fMeterVal =  gDefaultCalEntry[chnn].xMin.fMeterValue +	
					   ( (TmpVal) * (gDefaultCalEntry[chnn].xMiddle.fMeterValue -  gDefaultCalEntry[chnn].xMin.fMeterValue) / 
					    ( gDefaultCalEntry[chnn].xMiddle.usAdcValue - gDefaultCalEntry[chnn].xMin.usAdcValue ) );
					
				} else {
					TmpVal =  gDefaultCalEntry[chnn].xMin.usAdcValue - usAdcVal;

					fMeterVal =  gDefaultCalEntry[chnn].xMin.fMeterValue -	
					   ( (TmpVal) * (gDefaultCalEntry[chnn].xMiddle.fMeterValue -  gDefaultCalEntry[chnn].xMin.fMeterValue) / 
					    ( gDefaultCalEntry[chnn].xMiddle.usAdcValue - gDefaultCalEntry[chnn].xMin.usAdcValue ) );
				}
				
				
				           
			} else if(usAdcVal > (32768)) {
				unsigned int TmpVal = 0;
				if( usAdcVal >= gDefaultCalEntry[chnn].xMiddle.usAdcValue){
					TmpVal = usAdcVal - gDefaultCalEntry[chnn].xMiddle.usAdcValue;
					
					fMeterVal =  gDefaultCalEntry[chnn].xMiddle.fMeterValue +	
					   ( (TmpVal) *(gDefaultCalEntry[chnn].xMax.fMeterValue -  gDefaultCalEntry[chnn].xMiddle.fMeterValue) / 
					    ( gDefaultCalEntry[chnn].xMax.usAdcValue - gDefaultCalEntry[chnn].xMiddle.usAdcValue ) );
					
				}else {
					TmpVal =  gDefaultCalEntry[chnn].xMiddle.usAdcValue - usAdcVal;

					fMeterVal =  gDefaultCalEntry[chnn].xMiddle.fMeterValue -	
					   ( (TmpVal) *(gDefaultCalEntry[chnn].xMax.fMeterValue -  gDefaultCalEntry[chnn].xMiddle.fMeterValue) / 
					    ( gDefaultCalEntry[chnn].xMax.usAdcValue - gDefaultCalEntry[chnn].xMiddle.usAdcValue ) );
				}
				            
			} else {
                fMeterVal = 0.0f;
            }
            
			//fMeterVal -= gCalEntry[chnn].factor;
			pVal->fMeasure = fMeterVal;
			if (fMeterVal>=4) {
			    pVal->fPercentUnit = (fMeterVal-4)/(20-4);
			    pVal->fMeterUnit = (ext_max-ext_min)*(fMeterVal-4)/(20-4)+ext_min+factor.factor;
			} else {
			    pVal->fPercentUnit = 0;
			    pVal->fMeterUnit = ext_min+factor.factor;
			}
			break;
		}
        
         case Range_0_5V:
		 	break;
        case Range_1_5V:
			break;
		case Range_0_20MA:{
			break;
			break;
		}
	}

}


void vGetCalValue(ADC_CHANNEL_E chnn , s_CalEntry_t *pCalEntry)
{
	*pCalEntry = gCalEntry[chnn];
}

void vSetCalValue(ADC_CHANNEL_E chnn ,eCalCountType_t eCalType, s_CalEntry_t *pCalEntry)
{
	switch(eCalType){
		case SET_MIN:
		{
			gCalEntryBak[chnn].xMin = pCalEntry->xMin;
			//gCalEntryBak[chnn].factor = pCalEntry->factor;
			break;
		}
		case SET_MIDDLE:
		{
			gCalEntryBak[chnn].xMiddle = pCalEntry->xMiddle;
			//gCalEntryBak[chnn].factor = pCalEntry->factor;
			break;
		}
		case SET_MAX:
		{
			gCalEntryBak[chnn].xMax = pCalEntry->xMax;
			//gCalEntryBak[chnn].factor = pCalEntry->factor;
			break;
		}
		case SET_ALL:{
			gCalEntryBak[chnn] = *pCalEntry;
			break;
		}
		default:
			break;
			
	}
}

