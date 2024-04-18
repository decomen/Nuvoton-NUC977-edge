#ifndef __STORAGE_LOG_H__
#define __STORAGE_LOG_H__

typedef enum {
    LOG_LVL_ASSERT      = 0,
    LOG_LVL_ERROR       = 1,
    LOG_LVL_WARN        = 2,
    LOG_LVL_INFO        = 3,
    LOG_LVL_DEBUG       = 4,
    LOG_LVL_VERBOSE     = 5,

    LOG_LVL_MAX,
} eLogLvl_t;

typedef struct {
    rt_time_t       time;
    eLogLvl_t       lvl;
    char            log[512+1];
} StorageLogItem_t;

void storage_log_init(void);
void storage_log_uinit(void);
rt_bool_t storage_log_printf(eLogLvl_t lvl, const char *fmt, ...);
rt_bool_t storage_log_add(int elvl, const char *log, int size);

rt_bool_t log_upload(int level, const char *tag, const char *log, int size, rt_time_t time);

#define bStorageLogAssert( fmt, ... )   storage_log_printf( LOG_LVL_ASSERT  , fmt, ##__VA_ARGS__ )
#define bStorageLogError( fmt, ... )    storage_log_printf( LOG_LVL_ERROR   , fmt, ##__VA_ARGS__ )
#define bStorageLogWarn( fmt, ... )     storage_log_printf( LOG_LVL_WARN    , fmt, ##__VA_ARGS__ )
#define bStorageLogInfo( fmt, ... )     storage_log_printf( LOG_LVL_INFO    , fmt, ##__VA_ARGS__ )
#define bStorageLogVerbose( fmt, ... )  storage_log_printf( LOG_LVL_VERBOSE , fmt, ##__VA_ARGS__ )
#define bStorageLogDebug( fmt, ... )    storage_log_printf( LOG_LVL_DEBUG   , fmt, ##__VA_ARGS__ )

#endif

