#include <board.h>
#include "sqlite3.h"

static pthread_mutex_t s_log_mutex;

#define STORAGE_LOG_NAND_PATH                BOARD_LOG_PATH
#define STORAGE_LOG_SDCARD_PATH              BOARD_LOG_PATH

static sqlite3 *s_log_db = RT_NULL;
static rt_bool_t s_log_dbinit = RT_FALSE;
static rt_bool_t s_log_transaction = RT_FALSE;
static rt_uint32_t s_log_limit_count;

#define _LOG_DEL_NUM                    (200)
// 重要 log 暂存 10000
#define _LOG_LIMIT                      (20000)

#define TABLE_LOG_DATA                  "Log_data"

// fixed : id, time
#define CREATE_LOG_TABLE_SQL(_table)    "CREATE TABLE IF NOT EXISTS "_table" " \
                                        "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL DEFAULT 0, " \
                                        "time INTEGER, level INTEGER, log TEXT(256), flag TINYINT );"

#define CREATE_INDEX_SQL(_table)        "CREATE INDEX IF NOT EXISTS "_table"_INDEX ON "_table" (time ASC);"
#define DROP_TABLE_SQL(_table)          "DROP TABLE IF EXISTS "_table

static void __begin_transaction(void);
static void __commit_transaction(void);
static void __rollback_transaction(void);

static rt_mq_t s_stroage_log_mq;

void rt_storage_log_thread(void *parameter);

void storage_log_init(void)
{
    if (rt_mutex_init(&s_log_mutex, "s_log_mutex", RT_IPC_FLAG_PRIO) != RT_EOK) {
        rt_kprintf("init s_log_mutex failed\n");
        while (1);
        return;
    }

    s_stroage_log_mq = rt_mq_create("sto_msg", sizeof(StorageLogItem_t), 10, RT_IPC_FLAG_PRIO);

    if (s_stroage_log_mq) {
        rt_thread_t storage_log_thread = rt_thread_create("storage_log", rt_storage_log_thread, RT_NULL, 0x1200, 24, 20);

        if (storage_log_thread != RT_NULL) {
            rt_thddog_register(storage_log_thread, 60);
            rt_thread_startup(storage_log_thread);
        }
    } else {
        rt_kprintf("vStorageInit : no mem\n");
    }
}

void storage_log_uinit(void)
{
    if (s_log_dbinit) {
        rt_mutex_take(&s_log_mutex, RT_WAITING_FOREVER);
        if (s_log_db) {
            __commit_transaction();
            sqlite3_close(s_log_db); s_log_db = RT_NULL;
        }
        s_log_dbinit = RT_FALSE;
        rt_mutex_release(&s_log_mutex);
    }
}

static void __begin_transaction(void)
{
    if (s_log_dbinit) {
        s_log_transaction = (SQLITE_OK == sqlite3_exec(s_log_db, "begin;", 0, 0, 0));
    }
}

static void __commit_transaction(void)
{
    if (s_log_dbinit && s_log_transaction) {
        if (sqlite3_exec(s_log_db, "commit;", 0, 0, 0) != SQLITE_OK) {
            __rollback_transaction();
        }
        s_log_transaction = RT_FALSE;
    }
}

static void __rollback_transaction(void)
{
    if (s_log_dbinit) {
        sqlite3_exec(s_log_db, "rollback;", 0, 0, 0);
    }
}

static void __relimit_table(void)
{
    rt_uint32_t maxid = sqlite3_last_insert_rowid(s_log_db);
    rt_uint32_t minid = maxid;
    char sql[128];
    
    if (maxid > s_log_limit_count) {
        sqlite3_stmt *stmt = RT_NULL;
        if (SQLITE_OK == sqlite3_prepare_v2(s_log_db, "SELECT min(id) FROM "TABLE_LOG_DATA" WHERE 1", -1, &stmt, 0)) {
            if (SQLITE_ROW == sqlite3_step(stmt)) {
                if (SQLITE_INTEGER == sqlite3_column_type(stmt, 0)) {
                    minid = sqlite3_column_int(stmt, 0);
                }
            }
        }
        if (stmt) {
            sqlite3_finalize(stmt);
        }
        if (maxid - minid > s_log_limit_count) {
            rt_sprintf(sql, "DELETE FROM "TABLE_LOG_DATA" WHERE id<%d", (int)maxid - (int)s_log_limit_count + _LOG_DEL_NUM);
            sqlite3_exec(s_log_db, sql, NULL, NULL, NULL);
        }
    }
}

void rt_storage_log_thread(void *parameter)
{
    rt_tick_t start_tick = rt_tick_get();
    rt_bool_t insert = RT_FALSE;

    int result = SQLITE_OK;
    char *errmsg = RT_NULL;

    rt_thddog_feed("sqlite3_open");
    switch (g_storage_cfg.ePath) {
    case ST_P_SPIFLASH:
        my_system("mkdir -p "STORAGE_LOG_NAND_PATH);
        result = sqlite3_open(STORAGE_LOG_NAND_PATH"rtu_log.db", &s_log_db);
        break;

    case ST_P_SD:
        my_system("mkdir -p "STORAGE_LOG_SDCARD_PATH);
        result = sqlite3_open(STORAGE_LOG_SDCARD_PATH"rtu_log.db", &s_log_db);
        break;

    default:
        result = SQLITE_ERROR;
        break;
    }
    
    if (result != SQLITE_OK) {
        goto _err;
    }

    rt_thddog_feed("sqlite3_exec");
    if (SQLITE_OK != sqlite3_exec(s_log_db, CREATE_LOG_TABLE_SQL(TABLE_LOG_DATA), NULL, NULL, &errmsg) ||
        SQLITE_OK != sqlite3_exec(s_log_db, CREATE_INDEX_SQL(TABLE_LOG_DATA), NULL, NULL, &errmsg)) {
        goto _err;
    }

    s_log_dbinit = RT_TRUE;
    s_log_limit_count = _LOG_LIMIT;

    rt_thddog_feed("__begin_transaction");
    __begin_transaction();

    for (;;) {
        StorageLogItem_t item;

        rt_thddog_feed("rt_mq_recv with 5 sec");
        if (RT_EOK == rt_mq_recv(s_stroage_log_mq, (void *)&item, sizeof(StorageLogItem_t), rt_tick_from_millisecond(5000))) {
            if (s_log_db) {
                sqlite3_stmt *stmt = RT_NULL;
                int result = SQLITE_ERROR;
                rt_mutex_take(&s_log_mutex, RT_WAITING_FOREVER);
                result = sqlite3_prepare_v2(s_log_db, "INSERT INTO "TABLE_LOG_DATA" (id,time,level,log,flag) values(NULL,?,?,?,?)", -1, &stmt, 0);
                rt_thddog_feed("sqlite3_bind");
                if (SQLITE_OK == result) {
                    sqlite3_bind_int(stmt, 1, item.time);
                    sqlite3_bind_int(stmt, 2, item.lvl);
                    sqlite3_bind_text(stmt, 3, item.log, strlen(item.log), NULL);
                    sqlite3_bind_int(stmt, 4, 0);
                    rt_thddog_feed("sqlite3_step");
                    if (SQLITE_DONE == sqlite3_step(stmt)) {
                        sqlite3_finalize(stmt); stmt = RT_NULL;
                        rt_thddog_feed("__relimit_table");
                        __relimit_table();
                    }
                    insert = RT_TRUE;
                }

                if (stmt) {
                    sqlite3_finalize(stmt);
                }
                rt_mutex_release(&s_log_mutex);
            }
        }

        if (rt_tick_get() - start_tick > rt_tick_from_millisecond(10000) && insert) {
            rt_mutex_take(&s_log_mutex, RT_WAITING_FOREVER);
            rt_thddog_feed("__commit_transaction with 10 sec");
            __commit_transaction();
            __begin_transaction();
            insert = RT_FALSE;
            rt_mutex_release(&s_log_mutex);
            start_tick = rt_tick_get();
        }
    }
    
    _err:
    s_log_dbinit = RT_FALSE;

    if (s_log_db) {
        sqlite3_close(s_log_db); s_log_db = RT_NULL;
    }

    if (errmsg) {
        rt_kprintf("sqlite ==> ERR: %s\n", errmsg);
        sqlite3_free(errmsg);
    }

    rt_thddog_exit();
}

rt_bool_t storage_log_printf(eLogLvl_t lvl, const char *fmt, ...)
{
    if (s_log_dbinit && fmt) {
    
        StorageLogItem_t item;
        item.time = das_get_time();
        item.lvl = lvl;
        {
            va_list args;
            va_start(args, fmt);
            rt_vsnprintf(item.log, sizeof(item.log) - 1, fmt, args);
            va_end(args);
            // wait if transaction
            rt_mutex_take(&s_log_mutex, RT_WAITING_FOREVER);
            rt_mutex_release(&s_log_mutex);
            rt_mq_send(s_stroage_log_mq, &item, sizeof(StorageLogItem_t));
        }
    }

    return RT_TRUE;
}

rt_bool_t storage_log_add(int elvl, const char *log, int size)
{
    if (s_log_dbinit && log && size > 0) {
    
        StorageLogItem_t item;
        item.time = das_get_time();
        item.lvl = elvl;
        if(size > sizeof(item.log)-1) size = sizeof(item.log)-1;
        strncpy(item.log, log, size);
        item.log[size] = '\0';
        // wait if transaction
        rt_mutex_take(&s_log_mutex, RT_WAITING_FOREVER);
        rt_mutex_release(&s_log_mutex);
        rt_mq_send(s_stroage_log_mq, &item, sizeof(StorageLogItem_t));
    }
    return RT_TRUE;
}

DEF_CGI_HANDLER(delLogData)
{
    rt_err_t err = RT_EOK;
    if (s_log_dbinit) {
        rt_mutex_take(&s_log_mutex, RT_WAITING_FOREVER);
        {
            char *errmsg = RT_NULL;
            if(
                SQLITE_OK != sqlite3_exec(s_log_db, DROP_TABLE_SQL(TABLE_LOG_DATA), NULL, NULL, &errmsg) ||
                SQLITE_OK != sqlite3_exec(s_log_db, CREATE_LOG_TABLE_SQL(TABLE_LOG_DATA), NULL, NULL, &errmsg) ||
                SQLITE_OK != sqlite3_exec(s_log_db, CREATE_INDEX_SQL(TABLE_LOG_DATA), NULL, NULL, &errmsg)
            ) {
                err = RT_ERROR;
            }
            if (errmsg) {
                rt_kprintf("sqlite ==> ERR: %s\n", errmsg);
                sqlite3_free(errmsg);
            }
        }
        rt_mutex_release(&s_log_mutex);
    }
    WEBS_PRINTF( "{\"ret\":%d}", err);
    WEBS_DONE(200);
}

DEF_CGI_HANDLER(getLogData)
{
#define LOG_PAGE_SIZE       (500)
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        rt_uint32_t start_time = (rt_uint32_t)cJSON_GetInt(pCfg, "start", 0);
        rt_uint32_t end_time = (rt_uint32_t)cJSON_GetInt(pCfg, "end", -1);
        int page = (rt_uint32_t)cJSON_GetInt(pCfg, "page", 0);
        //int lvl = cJSON_GetInt(pCfg, "lvl", -1);
        cJSON_Delete(pCfg);
        WEBS_PRINTF("{\"ret\":0,\"logs\":[");
        if (s_log_dbinit) {
            rt_mutex_take(&s_log_mutex, RT_WAITING_FOREVER);
            {
                sqlite3_stmt *stmt = RT_NULL;
                char sql[256] = "";
                rt_sprintf(sql, "SELECT id,time,level,log FROM "TABLE_LOG_DATA" WHERE time>=%u AND time<=%u ORDER BY time ASC LIMIT %d,%d", (unsigned int)start_time, (unsigned int)end_time, page * LOG_PAGE_SIZE, (page + 1) * LOG_PAGE_SIZE);
                if (SQLITE_OK == sqlite3_prepare_v2(s_log_db, sql, -1, &stmt, 0)) {
                    rt_bool_t first = RT_TRUE;
                    while (SQLITE_ROW == sqlite3_step(stmt)) {
                        int id = sqlite3_column_int(stmt, 0);
                        time_t htime = (time_t)sqlite3_column_int(stmt, 1);
                        int level = sqlite3_column_int(stmt, 2);
                        const char *log = sqlite3_column_text(stmt, 3);
                        if (log) {
                            cJSON *pItem = cJSON_CreateObject();
                            if(pItem) {
                                char *szItem = RT_NULL;
                                if (!first) WEBS_PRINTF(",");
                                first = RT_FALSE;
                                {
                                    struct tm lt;
                                    char date[32] = { 0 };
                                    das_localtime_r(&htime, &lt);
                                    rt_sprintf(date, "%04d-%02d-%02d %02d:%02d:%02d",
                                               lt.tm_year + 1900, lt.tm_mon + 1,
                                               lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
                                    cJSON_AddNumberToObject(pItem, "n", id);
                                    cJSON_AddStringToObject(pItem, "t", date);
                                    cJSON_AddNumberToObject(pItem, "l", level);
                                    cJSON_AddStringToObject(pItem, "g", log);
                                }
                                szItem = cJSON_PrintUnformatted(pItem);
                                
                                if(szItem) {
                                    WEBS_PRINTF(szItem);
                                    rt_free(szItem);
                                }
                            }
                            cJSON_Delete(pItem);
                        }
                    }
                }

                if (stmt) {
                    sqlite3_finalize(stmt);
                }
            }
            rt_mutex_release(&s_log_mutex);
        }
    } else {
        err = RT_ERROR;
    }
    WEBS_PRINTF("]}");
    WEBS_DONE(200);
}

static rt_bool_t _s_in_upload = RT_FALSE;
rt_bool_t log_upload(int level, const char *tag, const char *log, int size, rt_time_t time)
{
    rt_bool_t ret = RT_FALSE;
    if(!_s_in_upload) {
        _s_in_upload = RT_TRUE;
        if (das_do_is_enet_up()) {
            hclient_session_t *session = hclient_create(2048);
            if (session) {
                cJSON *json = cJSON_CreateObject();
                if (json) {
                    char strbuf[128] = {0};
                    cJSON_AddStringToObject(json, "product_model", PRODUCT_MODEL);
                    rt_memset(strbuf, 0, sizeof(strbuf));
                    rt_memcpy(strbuf, g_sys_info.SN, sizeof(g_sys_info.SN));
                    cJSON_AddStringToObject(json, "sn", strbuf);
                    rt_sprintf(strbuf, "%d.%02d", SW_VER_MAJOR, SW_VER_MINOR);
                    cJSON_AddStringToObject(json, "software_no", strbuf);
                    rt_sprintf(strbuf, "%d.%02d", HW_VER_VERCODE/100, HW_VER_VERCODE%100);
                    cJSON_AddStringToObject(json, "device_no", strbuf);
                    cJSON_AddNumberToObject(json, "type", level);
                    cJSON_AddStringToObject(json, "code", _STR(tag));
                    if(log) {
                        char *log_buf = rt_calloc(1, size+1);
                        if(log_buf) {
                            rt_strncpy(log_buf, log, size);
                            log_buf[size] = '\0';
                            cJSON_AddStringToObject(json, "msg", log_buf);
                            rt_free(log_buf);
                        }
                    }
                    {
                        struct tm lt;
                        das_localtime_r(&time, &lt);
                        rt_sprintf(strbuf, "%04d-%02d-%02d% 02d:%02d:%02d",
                                   lt.tm_year + 1900,
                                   lt.tm_mon + 1,
                                   lt.tm_mday,
                                   lt.tm_hour,
                                   lt.tm_min,
                                   lt.tm_sec
                                  );
                        cJSON_AddStringToObject(json, "date_time", strbuf);
                    }
                    hclient_get_sign(json, strbuf, "sn", "device_no", "code", "date_time", NULL);
                    cJSON_AddStringToObject(json, "sign", strbuf);
                    {
                        char *szJson = cJSON_PrintUnformatted(json);
                        if (szJson) {
                            //rt_kprintf("log_upload:%s\n", szJson);
                            hclient_err_e err = hclient_post(session, HTTP_SERVER_HOST"/log_add.action", szJson, 5000);
                            rt_free(szJson);
                            if (HCLIENT_ERR_OK == err) {
                                ret = RT_TRUE;
                            }
                        }
                    }
                    cJSON_Delete(json);
                }
                hclient_destroy(session);
            }
        }
    }
    _s_in_upload = RT_FALSE;
    return ret;
}

