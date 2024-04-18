#ifndef __DIDO_CFG_H__
#define __DIDO_CFG_H__

#define DI_EXP_LEN      (1024)
#define DO_EXP_LEN      (1024)

typedef struct {
    rt_bool_t enable;
    rt_uint32_t interval;
    char exp[DI_EXP_LEN];
} di_cfg_t;

typedef struct {
    char exp[DO_EXP_LEN];
} do_cfg_t;

extern di_cfg_t g_di_cfgs[DI_CHAN_NUM];
extern do_cfg_t g_do_cfgs[DO_CHAN_NUM];

rt_err_t di_cfgs_init( void );
rt_err_t do_cfgs_init( void );

#endif
