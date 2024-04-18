#include <board.h>
#include <ad7707.h>

static const s_CalEntry_t gDefaultCalEntry[ADC_CHANNEL_NUM] = {
	[ADC_CHANNEL_0] = {
		.xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},	
		.xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},	
		.xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},	
		.factor = {0.0f},
	},
	[ADC_CHANNEL_1] = {
		.xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},	
		.xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},	
		.xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},	
		.factor = {0.0f},
	},
	[ADC_CHANNEL_2] = {
		.xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},	
		.xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},	
		.xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},	
		.factor = {0.0f},
	},
	[ADC_CHANNEL_3] = {
		.xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},	
		.xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},	
		.xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},	
		.factor = {0.0f},
	},
	[ADC_CHANNEL_4] = {
		.xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},	
		.xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},	
		.xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},	
		.factor = {0.0f},
	},
	[ADC_CHANNEL_5] = {
		.xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},	
		.xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},	
		.xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},	
		.factor = {0.0f},
	},
	[ADC_CHANNEL_6] = {
		.xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},	
		.xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},	
		.xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},	
		.factor = {0.0f},
	},
	[ADC_CHANNEL_7] = {
		.xMin = {DEFAULT_CAL_0_20MA_MIN_METER_VAL , DEFAULT_CAL_0_20MA_MIN_ADC_VAL},	
		.xMiddle = {DEFAULT_CAL_0_20MA_MIDDLE_METER_VAL,DEFAULT_CAL_0_20MA_MIDDLE_ADC_VAL},	
		.xMax = {DEFAULT_CAL_0_20MA_MAX_METER_VAL,DEFAULT_CAL_0_20MA_MAX_ADC_VAL},	
		.factor = {0.0f},
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

void vAd7707Init(void)
{
    
}


void vAdcPowerEnable(void)
{
    
}

void vAdcPowerDisable(void)
{
    
}

static unsigned long getAdcChnn2(ADC_CHANNEL_E chnn)
{
    float value = 0;
    das_do_get_ai_value((int)chnn, &value;
    return (unsigned long)value;
}


// A = (D - D0) *　(Am - A0) / (Dm-D0) + A0
// A 表示测量值
// D 表示ADC值

// eHardwareRangeType  硬件量程，表示硬件上面可以输入的量程范围，目前我们硬件是可以测量0-20ma
// eSoftwareType       软件量程，软件上面限制量程范围，比如实际可以测量0-20ma,但网页上面只能测量4-20MA. 低于4MA,网页就提示超量程，且此时工程量值和测量值都为0



void vGetAdcValue(ADC_CHANNEL_E chnn , eRangeType_t eHardwareRangeType, eRangeType_t eSoftwareType, float ext_min, float ext_max, s_CorrectionFactor_t factor, s_AdcValue_t *pVal)
{
	unsigned long usAdcVal = ((getAdcChnn2(chnn)) >> 4);


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
	case Range_4_20MA: {
			if (usAdcVal > 0 && usAdcVal <= (0x7FFFF)) {
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
				
				
				           
			} else if(usAdcVal > (0x7FFFF)) {
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
			break;
		}
	}

	
}

//用于测试模式下，获取校准过后的ADC值

void vGetAdcTestValueInfo(ADC_CHANNEL_E chnn , eRangeType_t eHardwareRangeType, eRangeType_t eSoftwareType, float ext_min, float ext_max, s_CorrectionFactor_t factor, s_AdcValue_t *pVal)
{
	unsigned long usAdcVal = ((getAdcChnn2(chnn)) >> 4);


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
			if (usAdcVal > 0 && usAdcVal <= (0x7FFFF)) {
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
				
				
				           
			} else if(usAdcVal > (0x7FFFF)) {
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
			if (fMeterVal >= 4) {
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

//用于校准，获取ADC值，校准获取ADC的值，使用理论值进行计算

void vGetAdcValueTest(ADC_CHANNEL_E chnn , eRangeType_t eHardwareRangeType, eRangeType_t eSoftwareType, float ext_min, float ext_max, s_CorrectionFactor_t factor, s_AdcValue_t *pVal)
{
	unsigned long usAdcVal = (getAdcChnn2(chnn) >> 4);

	float fPercent = 0 , fMeterVal = 0;

    switch(eSoftwareType) {
		case Range_0_20MA:{
		    if( Range_0_20MA == eHardwareRangeType ) {
		    }
	        break;
		}
		case Range_4_20MA:{
			 if( Range_0_20MA == eHardwareRangeType ) {
			 	if(usAdcVal < (gDefaultCalEntry[chnn].xMiddle.usAdcValue/gDefaultCalEntry[chnn].xMiddle.fMeterValue) * 4.0 ){
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
	case Range_4_20MA:/*{
			fPercent = (float)(usAdcVal / 1048576.0f);
			//fMeterVal = (float)(temp * (gRange[Range_0_20MA].ucMax - gRange[Range_0_20MA].ucMin) / 65535.0f);
			if(usAdcVal >= 0 && usAdcVal <= (0x7FFFF)){
				fMeterVal = (usAdcVal - gCalEntry[Range_4_20MA].xMin.usAdcValue) * 
					    (gCalEntry[Range_4_20MA].xMiddle.fMeterValue -  gCalEntry[Range_4_20MA].xMin.fMeterValue) / 
					    ( gCalEntry[Range_4_20MA].xMiddle.usAdcValue - gCalEntry[Range_4_20MA].xMin.usAdcValue ) + 
				            gCalEntry[Range_4_20MA].xMin.fMeterValue;	
			}else if(usAdcVal > (0x7FFFF)){
				fMeterVal = (usAdcVal - gCalEntry[Range_4_20MA].xMiddle.usAdcValue) * 
					    (gCalEntry[Range_4_20MA].xMax.fMeterValue -  gCalEntry[Range_4_20MA].xMiddle.fMeterValue) / 
					    ( gCalEntry[Range_4_20MA].xMax.usAdcValue - gCalEntry[Range_4_20MA].xMiddle.usAdcValue ) + 
				            gCalEntry[Range_4_20MA].xMiddle.fMeterValue;	
			}
			pVal->fPercentUnit = fPercent;
			//pVal->fPercentUnit = fMeterVal;
			pVal->fMeasure = fMeterVal;
			pVal->fMeterUnit = (ext_max-ext_min)*(fMeterVal-4)/(20-4)+ext_min+factor.factor;
			
			break;
		}*/
        
         case Range_0_5V:/*{
			fPercent = (float)(usAdcVal / 1048576.0f);
			
			if(usAdcVal >= 0 && usAdcVal <= (0x7FFFF)){
				fMeterVal = (usAdcVal - gCalEntry[Range_0_5V].xMin.usAdcValue) * 
					    (gCalEntry[Range_0_5V].xMiddle.fMeterValue -  gCalEntry[Range_0_5V].xMin.fMeterValue) / 
					    ( gCalEntry[Range_0_5V].xMiddle.usAdcValue - gCalEntry[Range_0_5V].xMin.usAdcValue ) + 
				            gCalEntry[Range_0_5V].xMin.fMeterValue;	
			}else if(usAdcVal > (0x7FFFF)){
				fMeterVal = (usAdcVal - gCalEntry[Range_0_5V].xMiddle.usAdcValue) * 
					    (gCalEntry[Range_0_5V].xMax.fMeterValue -  gCalEntry[Range_0_5V].xMiddle.fMeterValue) / 
					    ( gCalEntry[Range_0_5V].xMax.usAdcValue - gCalEntry[Range_0_5V].xMiddle.usAdcValue ) + 
				            gCalEntry[Range_0_5V].xMiddle.fMeterValue;	
			}
			
			pVal->fPercentUnit = fPercent;
			pVal->fMeasure = fMeterVal;
			pVal->fMeterUnit = (ext_max-ext_min)*(fMeterVal-0)/(5-0)+ext_min+factor.factor;
			break;
		}*/
        case Range_1_5V:/*{
			fPercent = (float)(usAdcVal / 1048576.0f);
			
			if(usAdcVal >= 0 && usAdcVal <= (0x7FFFF)){
				fMeterVal = (usAdcVal - gCalEntry[Range_1_5V].xMin.usAdcValue) * 
					    (gCalEntry[Range_1_5V].xMiddle.fMeterValue -  gCalEntry[Range_1_5V].xMin.fMeterValue) / 
					    ( gCalEntry[Range_1_5V].xMiddle.usAdcValue - gCalEntry[Range_1_5V].xMin.usAdcValue ) + 
				            gCalEntry[Range_1_5V].xMin.fMeterValue;	
			}else if(usAdcVal > (0x7FFFF)){
				fMeterVal = (usAdcVal - gCalEntry[Range_1_5V].xMiddle.usAdcValue) * 
					    (gCalEntry[Range_1_5V].xMax.fMeterValue -  gCalEntry[Range_1_5V].xMiddle.fMeterValue) / 
					    ( gCalEntry[Range_1_5V].xMax.usAdcValue - gCalEntry[Range_1_5V].xMiddle.usAdcValue ) + 
				            gCalEntry[Range_1_5V].xMiddle.fMeterValue;	
			}
			
			pVal->fPercentUnit = fPercent;
			pVal->fMeasure = fMeterVal;
			pVal->fMeterUnit = (ext_max-ext_min)*(fMeterVal-1)/(5-1)+ext_min+factor.factor;
			break;
		}*/
		case Range_0_20MA:{
			fPercent = (float)(usAdcVal / 1048576.0f);
			//fMeterVal = (float)(temp * (gRange[Range_4_20MA].ucMax - gRange[Range_4_20MA].ucMin) / 65535.0f);

			if(usAdcVal >= 0 && usAdcVal <= (0x7FFFF)){
				
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
				
			}else if(usAdcVal > (0x7FFFF)){

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
				
			}else {
              fPercent = 0.0f;
            }
			
			//pVal->fPercentUnit = fPercent;
			//pVal->fMeasure = fMeterVal;
			//pVal->fMeterUnit = (ext_max-ext_min)*(fMeterVal-0)/(20-0)+ext_min+factor.factor;

			pVal->fMeasure = fMeterVal;
			if (fMeterVal >= 4) {
			    pVal->fPercentUnit = (fMeterVal-0)/(20-0);
			    pVal->fMeterUnit = (ext_max-ext_min)*(fMeterVal-0)/(20-0)+ext_min+factor.factor;
			} else {
			    pVal->fPercentUnit = 0;
			    pVal->fMeterUnit = ext_min+factor.factor;
			}
			
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

