#ifndef __ANALOG_CFG_H__
#define __ANALOG_CFG_H__

typedef struct {
    rt_bool_t enable;
    rt_uint32_t interval;
    eRangeType_t range;
    eUnitType_t unit;

    float ext_range_min;
    float ext_range_max;
    s_CorrectionFactor_t ext_corr;
} analog_cfg_t;

extern analog_cfg_t g_analog_cfgs[ADC_CHANNEL_NUM];

rt_err_t analog_cfgs_init( void );

#endif
