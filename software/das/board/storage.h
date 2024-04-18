
#ifndef _STORAGE__H__
#define _STORAGE__H__

#define STORAGE_VER_STR         (0)

#define STORAGE_LINE_LIMIT      (10)

typedef enum {
    ST_P_SPIFLASH,
    ST_P_SD,

    ST_P_MAX,
} eStoragePath;

typedef enum {
    ST_T_RT_DATA,
    ST_T_MINUTE_DATA,
    ST_T_HOURLY_DATA,
    ST_T_DIDO_DATA,

    // for CC
    ST_T_CC_BJDC_DATA,

    ST_T_COMMIT,

    ST_T_MAX,
} eStorageType;

typedef struct {
    double          value;
    char            ident[12+1];
    char            alias[8+1];
} StorageDataItem_t;

// for cc_bjdc
typedef struct {
    int             desc_id;
    double          value;
    rt_uint8_t      flag;
} Storage_CC_BJDC_Item_t;

typedef struct {
    eStorageType    type;
    rt_time_t       time;
    union {
        StorageDataItem_t       xData;
        Storage_CC_BJDC_Item_t  xCC_BJDC;
    } xItem;
} StorageItem_t;

typedef struct {
    rt_bool_t       bEnable;
    rt_uint32_t     ulStep;
    eStoragePath    ePath;
} storage_cfg_t;

extern storage_cfg_t g_storage_cfg;

void vStorageInit( void );
rt_bool_t bStorageAddData( eStorageType type, char *ident, double value, char *alias );

rt_bool_t bStorageDoCommit(void);
rt_bool_t bStorageDoClose(void);

int nStorage_CC_BJDC_DataDesc(
    const char *meter_id,
    const char *func_idex
);

rt_err_t storage_cfg_init( void );

#endif

