
#include <board.h>
#include "sqlite3.h"

static pthread_mutex_t cfg_mutex;
cfg_info_t g_cfg_info;

const rt_uint8_t _empty_buffer[128] = { 0 };

static sqlite3 *s_cfg_db = RT_NULL;
static rt_bool_t s_cfg_dbinit = RT_FALSE;

#define TABLE_CFG_VAREXT                "cfg_varext"
#define TABLE_CFG_RULE                  "cfg_rule"
#define TABLE_CFG_BOARD                 "cfg_board"

#define CREATE_CFG_VAREXT_TABLE_SQL     "CREATE TABLE IF NOT EXISTS " TABLE_CFG_VAREXT " " \
                                        "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL DEFAULT 0, " \
                                        "time INTEGER, name TEXT, cfg TEXT );"
                                        
#define CREATE_CFG_RULE_TABLE_SQL       "CREATE TABLE IF NOT EXISTS " TABLE_CFG_RULE " " \
                                        "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL DEFAULT 0, " \
                                        "time INTEGER, name TEXT, cfg TEXT );"
                                        
#define CREATE_CFG_BOARD_TABLE_SQL      "CREATE TABLE IF NOT EXISTS " TABLE_CFG_BOARD " " \
                                        "(name TEXT PRIMARY KEY, time INTEGER, cfg BLOB, crc INTEGER );"

void board_cfg_init(void)
{
    int result = SQLITE_OK;
    char *errmsg = RT_NULL;

    my_system("mkdir -p "BOARD_CFG_PATH);
    result = sqlite3_open(BOARD_CFG_PATH"rtu_cfg_v0.db", &s_cfg_db);

    if (result != SQLITE_OK) {
        goto _err;
    }

    rt_thddog_feed("sqlite3_exec");
    if (SQLITE_OK != sqlite3_exec(s_cfg_db, CREATE_CFG_VAREXT_TABLE_SQL, NULL, NULL, &errmsg) ||
        SQLITE_OK != sqlite3_exec(s_cfg_db, CREATE_CFG_RULE_TABLE_SQL, NULL, NULL, &errmsg) ||
        SQLITE_OK != sqlite3_exec(s_cfg_db, CREATE_CFG_BOARD_TABLE_SQL, NULL, NULL, &errmsg)) {
        goto _err;
    }

    if (rt_mutex_init(&cfg_mutex, "cfg_mutex", RT_IPC_FLAG_PRIO) != RT_EOK) {
        rt_kprintf("init cfg_mutex failed\n");
        while (1);
        return;
    }
    
    s_cfg_dbinit = RT_TRUE;

    if (!board_cfg_read(CFG_INFO_NAME, &g_cfg_info, sizeof(g_cfg_info))) {
        g_cfg_info.usVer = CFG_VER;
        board_cfg_write(CFG_INFO_NAME, &g_cfg_info, sizeof(g_cfg_info));
    }
    return ;
    
    _err:
    s_cfg_dbinit = RT_FALSE;

    if (s_cfg_db) {
        sqlite3_close(s_cfg_db); s_cfg_db = RT_NULL;
    }

    if (errmsg) {
        rt_kprintf("sqlite ==> ERR: %s\n", errmsg);
        sqlite3_free(errmsg);
    }
}

void board_cfg_uinit(void)
{
    if (s_cfg_dbinit) {
        rt_mutex_take(&cfg_mutex, RT_WAITING_FOREVER);
        if (s_cfg_db) {
            sqlite3_close(s_cfg_db); s_cfg_db = RT_NULL;
        }
        s_cfg_dbinit = RT_FALSE;
        rt_mutex_release(&cfg_mutex);
    }
}

void board_cfg_del_all(void)
{
    int result = SQLITE_OK;
    char *errmsg = RT_NULL;
    if (s_cfg_dbinit) {
        rt_mutex_take(&cfg_mutex, RT_WAITING_FOREVER);
        board_cfg_uinit();
        // delete
        //my_system("rm "BOARD_CFG_PATH" -rf");
        // bakeup
        my_system("rm "BOARD_CFG_PATH"*.ini -rf");
       // my_system("rm  "DM101_INI_CFG_PATH_PREFIX" -rf"); 
        my_system("mv "BOARD_CFG_PATH"rtu_cfg_v0.db "BOARD_CFG_PATH"rtu_cfg_v0.db.bak -f");
        result = sqlite3_open(BOARD_CFG_PATH, &s_cfg_db);
        if (result != SQLITE_OK) {
            rt_mutex_release(&cfg_mutex);
            goto _err;
        }
        rt_thddog_feed("sqlite3_exec");
        if (SQLITE_OK != sqlite3_exec(s_cfg_db, CREATE_CFG_VAREXT_TABLE_SQL, NULL, NULL, &errmsg) ||
            SQLITE_OK != sqlite3_exec(s_cfg_db, CREATE_CFG_RULE_TABLE_SQL, NULL, NULL, &errmsg) ||
            SQLITE_OK != sqlite3_exec(s_cfg_db, CREATE_CFG_BOARD_TABLE_SQL, NULL, NULL, &errmsg)) {
            rt_mutex_release(&cfg_mutex);
            goto _err;
        }
        board_cfg_write(CFG_INFO_NAME, &g_cfg_info, sizeof(g_cfg_info));
        board_cfg_write(REG_CFG_NAME, &g_reg, sizeof(g_reg));
        rt_mutex_release(&cfg_mutex);
    }
    
    return ;
    _err:
    s_cfg_dbinit = RT_FALSE;

    if (s_cfg_db) {
        sqlite3_close(s_cfg_db); s_cfg_db = RT_NULL;
    }

    if (errmsg) {
        rt_kprintf("sqlite ==> ERR: %s\n", errmsg);
        sqlite3_free(errmsg);
    }
}

rt_bool_t board_cfg_del_one(const char *type)
{
    rt_bool_t ret = RT_FALSE;
    if (s_cfg_dbinit) {
        rt_mutex_take(&cfg_mutex, RT_WAITING_FOREVER);
        {
            char sql[256] = {0};
            sprintf(sql, "DELETE FROM "TABLE_CFG_BOARD" WHERE name='%s'", type);
            if (SQLITE_OK == sqlite3_exec(s_cfg_db, sql, NULL, NULL, NULL)) {
                ret = RT_TRUE;
            }
        }
        rt_mutex_release(&cfg_mutex);
    }
    return ret;
}

// 带校验读
rt_bool_t board_cfg_read(const char *type, void *pcfg, int len)
{
    rt_bool_t ret = RT_FALSE;
    if (s_cfg_dbinit) {
        rt_mutex_take(&cfg_mutex, RT_WAITING_FOREVER);
        {
            sqlite3_stmt *stmt = RT_NULL;
            char sql[256] = {0};
            sprintf(sql, "SELECT name,cfg,crc FROM "TABLE_CFG_BOARD" WHERE name='%s'", type);
            if (SQLITE_OK == sqlite3_prepare_v2(s_cfg_db, sql, -1, &stmt, 0)) {
                while (SQLITE_ROW == sqlite3_step(stmt)) {
                    const char *name = sqlite3_column_text(stmt, 0);
                    int cfg_len = sqlite3_column_bytes(stmt, 1);
                    const void *cfg = sqlite3_column_blob(stmt, 1);
                    int crc = sqlite3_column_int(stmt, 2);
                    if (name && cfg && cfg_len >= len && ((uint16_t)crc == das_crc16((uint8_t *)cfg, cfg_len))) {
                        memcpy(pcfg, cfg, len);
                        ret = RT_TRUE;
                    }
                }
            }
            if (stmt) {
                sqlite3_finalize(stmt);
            }
        }
        rt_mutex_release(&cfg_mutex);
    }
    return ret;
}

// 带校验写
rt_bool_t board_cfg_write(const char *type, void const *pcfg, int len)
{
    rt_bool_t ret = RT_FALSE;
    if (s_cfg_dbinit) {
        rt_mutex_take(&cfg_mutex, RT_WAITING_FOREVER);
        {
            sqlite3_stmt *stmt = RT_NULL;
            board_cfg_del_one(type);
            if (SQLITE_OK == sqlite3_prepare_v2(s_cfg_db, "INSERT INTO "TABLE_CFG_BOARD" values(?,?,?,?)", -1, &stmt, 0)) {
                sqlite3_bind_text(stmt, 1, type, strlen(type), NULL);
                sqlite3_bind_int(stmt, 2, das_get_time());
                sqlite3_bind_blob(stmt, 3, pcfg, len, NULL);
                sqlite3_bind_int(stmt, 4, (int)das_crc16((uint8_t *)pcfg, len));
                if (SQLITE_DONE == sqlite3_step(stmt)) {
                    ret = RT_TRUE;
                }
            }
            if (stmt) sqlite3_finalize(stmt);
        }
        rt_mutex_release(&cfg_mutex);
    }
    return ret;
}

#include "varmanage.h"
void vVarManage_InsertNode(ExtData_t *data);
var_bool_t jsonFillExtDataInfo(ExtData_t *data, cJSON *pItem);
var_bool_t jsonParseExtDataInfo(cJSON *pItem, ExtData_t *data);
rt_bool_t board_cfg_varext_loadlist( ExtDataList_t *pList )
{
    rt_bool_t ret = RT_FALSE;
    if (s_cfg_dbinit) {
        rt_mutex_take(&cfg_mutex, RT_WAITING_FOREVER);
        {
            sqlite3_stmt *stmt = RT_NULL;
            if (SQLITE_OK == sqlite3_prepare_v2(s_cfg_db, "SELECT id,name,cfg FROM "TABLE_CFG_VAREXT" WHERE 1 ORDER BY id ASC", -1, &stmt, 0)) {
                while (SQLITE_ROW == sqlite3_step(stmt)) {
                    //int id = sqlite3_column_int(stmt, 0);
                    const char *name = sqlite3_column_text(stmt, 1);
                    const char *cfg = sqlite3_column_text(stmt, 2);
                    if (name && cfg) {
                        ExtData_t *data = VAR_MANAGE_CALLOC(1, sizeof(ExtData_t));
                        if(data) {
                            cJSON *pItem = cJSON_Parse(cfg);
                            if(pItem) {
                                if (jsonParseExtDataInfo(pItem, data)) {
                                    vVarManage_InsertNode(data);
                                } else {
                                    VAR_MANAGE_FREE(data);
                                }
                            } else {
                                VAR_MANAGE_FREE(data);
                            }
                            cJSON_Delete(pItem);
                        }
                        ret = RT_TRUE;
                    }
                }
            }

            if (stmt) {
                sqlite3_finalize(stmt);
            }
        }
        rt_mutex_release(&cfg_mutex);
    }

    return ret;
}

rt_bool_t board_cfg_varext_del(const char *name)
{
    rt_bool_t ret = RT_FALSE;
    if (s_cfg_dbinit) {
        rt_mutex_take(&cfg_mutex, RT_WAITING_FOREVER);
        {
            char sql[256] = {0};
            sprintf(sql, "DELETE FROM "TABLE_CFG_VAREXT" WHERE name='%s'", name);
            if (SQLITE_OK == sqlite3_exec(s_cfg_db, sql, NULL, NULL, NULL)) {
                ret = RT_TRUE;
            }
        }
        rt_mutex_release(&cfg_mutex);
    }
    return ret;
}

rt_bool_t board_cfg_varext_del_all(void)
{
    rt_bool_t ret = RT_FALSE;
    if (s_cfg_dbinit) {
        rt_mutex_take(&cfg_mutex, RT_WAITING_FOREVER);
        {
            if (SQLITE_OK == sqlite3_exec(s_cfg_db, "DELETE FROM "TABLE_CFG_VAREXT, NULL, NULL, NULL)) {
                ret = RT_TRUE;
            }
        }
        rt_mutex_release(&cfg_mutex);
    }
    return ret;
}

rt_bool_t board_cfg_varext_update(const char *name, ExtData_t *data)
{
    rt_bool_t ret = RT_FALSE;
    if (s_cfg_dbinit) {
        rt_mutex_take(&cfg_mutex, RT_WAITING_FOREVER);
        {
            cJSON *pItem = cJSON_CreateObject();
            if(pItem) {
                sqlite3_stmt *stmt = RT_NULL;
                char sql[256] = {0};
                char *szJSON = VAR_NULL;
                sprintf(sql, "UPDATE "TABLE_CFG_VAREXT" SET name=?,cfg=? WHERE name='%s'", name);
                if (SQLITE_OK == sqlite3_prepare_v2(s_cfg_db, sql, -1, &stmt, 0)) {
                    sqlite3_bind_text(stmt, 1, data->xName.szName, strlen(data->xName.szName), NULL);
                    if (jsonFillExtDataInfo(data, pItem) && (szJSON = cJSON_PrintUnformatted(pItem)) != RT_NULL) {
                        sqlite3_bind_text(stmt, 2, szJSON, strlen(szJSON), NULL);
                    }
                    if (SQLITE_DONE == sqlite3_step(stmt)) {
                        ret = RT_TRUE;
                    }
                    if (szJSON) rt_free(szJSON);
                }
                if (stmt) sqlite3_finalize(stmt);
            }
            cJSON_Delete(pItem);
        }
        rt_mutex_release(&cfg_mutex);
    }
    return ret;
}

rt_bool_t board_cfg_varext_add(ExtData_t *data)
{
    rt_bool_t ret = RT_FALSE;
    if (s_cfg_dbinit) {
        rt_mutex_take(&cfg_mutex, RT_WAITING_FOREVER);
        {
            cJSON *pItem = cJSON_CreateObject();
            if(pItem) {
                char *szJSON = VAR_NULL;
                sqlite3_stmt *stmt = RT_NULL;
                board_cfg_varext_del((const char *)data->xName.szName);
                if (SQLITE_OK == sqlite3_prepare_v2(s_cfg_db, "INSERT INTO "TABLE_CFG_VAREXT" values(NULL,?,?,?)", -1, &stmt, 0)) {
                    sqlite3_bind_int(stmt, 1, time(0));
                    sqlite3_bind_text(stmt, 2, (const char *)data->xName.szName, strlen(data->xName.szName), NULL);
                    if (jsonFillExtDataInfo(data, pItem) && (szJSON = cJSON_PrintUnformatted(pItem)) != RT_NULL) {
                        sqlite3_bind_text(stmt, 3, szJSON, strlen(szJSON), NULL);
                    }
                    if (SQLITE_DONE == sqlite3_step(stmt)) {
                        ret = RT_TRUE;
                    }
                }
                if (szJSON) rt_free(szJSON);
                if (stmt) sqlite3_finalize(stmt);
            }
            cJSON_Delete(pItem);
        }
        rt_mutex_release(&cfg_mutex);
    }
    return ret;
}

#include "rule.h"
rt_bool_t jsonFillRuleInfo(struct rule_node *rule, cJSON *pItem);
rt_bool_t jsonParseRuleInfo(cJSON *pItem, struct rule_node *rule);
rt_bool_t board_cfg_rule_loadlist(void)
{
    rt_bool_t ret = RT_FALSE;
    if (s_cfg_dbinit) {
        rt_mutex_take(&cfg_mutex, RT_WAITING_FOREVER);
        {
            sqlite3_stmt *stmt = RT_NULL;
            if (SQLITE_OK == sqlite3_prepare_v2(s_cfg_db, "SELECT id,name,cfg FROM "TABLE_CFG_RULE" WHERE 1 ORDER BY id ASC", -1, &stmt, 0)) {
                struct rule_node *rule = rt_calloc(1, sizeof(struct rule_node));
                if (rule) {
                    while (SQLITE_ROW == sqlite3_step(stmt)) {
                        //int id = sqlite3_column_int(stmt, 0);
                        const char *name = sqlite3_column_text(stmt, 1);
                        const char *cfg = sqlite3_column_text(stmt, 2);
                        if (name && cfg) {
                            if(rule) {
                                cJSON *pItem = cJSON_Parse(cfg);
                                if(pItem) {
                                    rule->type = RULE_TYPE_ERR;
                                    rule->sys = RULE_SYS_ERR;
                                    if (jsonParseRuleInfo(pItem, rule)) {
                                        rule_new(rule->enable, rule->name, rule->type, rule->p_in, rule->c_in, rule->p_out, rule->c_out, rule->sys);
                                    }
                                }
                                cJSON_Delete(pItem);
                            }
                            ret = RT_TRUE;
                        }
                    }
                    rt_free(rule);
                }
            }

            if (stmt) {
                sqlite3_finalize(stmt);
            }
        }
        rt_mutex_release(&cfg_mutex);
    }

    return ret;
}

rt_bool_t board_cfg_rule_del(const char *name)
{
    rt_bool_t ret = RT_FALSE;
    if (s_cfg_dbinit) {
        rt_mutex_take(&cfg_mutex, RT_WAITING_FOREVER);
        {
            char sql[256] = {0};
            sprintf(sql, "DELETE FROM "TABLE_CFG_RULE" WHERE name='%s'", name);
            if (SQLITE_OK == sqlite3_exec(s_cfg_db, sql, NULL, NULL, NULL)) {
                ret = RT_TRUE;
            }
        }
        rt_mutex_release(&cfg_mutex);
    }
    return ret;
}

rt_bool_t board_cfg_rule_del_all(void)
{
    rt_bool_t ret = RT_FALSE;
    if (s_cfg_dbinit) {
        rt_mutex_take(&cfg_mutex, RT_WAITING_FOREVER);
        {
            if (SQLITE_OK == sqlite3_exec(s_cfg_db, "DELETE FROM "TABLE_CFG_RULE, NULL, NULL, NULL)) {
                ret = RT_TRUE;
            }
        }
        rt_mutex_release(&cfg_mutex);
    }
    return ret;
}

rt_bool_t board_cfg_rule_update(const char *name, struct rule_node *rule)
{
    rt_bool_t ret = RT_FALSE;
    if (s_cfg_dbinit) {
        rt_mutex_take(&cfg_mutex, RT_WAITING_FOREVER);
        {
            cJSON *pItem = cJSON_CreateObject();
            if(pItem) {
                sqlite3_stmt *stmt = RT_NULL;
                char sql[256] = {0};
                char *szJSON = VAR_NULL;
                sprintf(sql, "UPDATE "TABLE_CFG_RULE" SET name=?,cfg=? WHERE name='%s'", name);
                if (SQLITE_OK == sqlite3_prepare_v2(s_cfg_db, sql, -1, &stmt, 0)) {
                    sqlite3_bind_text(stmt, 1, rule->name, strlen(rule->name), NULL);
                    if (jsonFillRuleInfo(rule, pItem) && (szJSON = cJSON_PrintUnformatted(pItem)) != RT_NULL) {
                        sqlite3_bind_text(stmt, 2, szJSON, strlen(szJSON), NULL);
                    }
                    if (SQLITE_DONE == sqlite3_step(stmt)) {
                        ret = RT_TRUE;
                    }
                    rt_free(szJSON);
                }
                if (stmt) sqlite3_finalize(stmt);
            }
            cJSON_Delete(pItem);
        }
        rt_mutex_release(&cfg_mutex);
    }
    return ret;
}

rt_bool_t board_cfg_rule_add(struct rule_node *rule)
{
    rt_bool_t ret = RT_FALSE;
    if (s_cfg_dbinit) {
        rt_mutex_take(&cfg_mutex, RT_WAITING_FOREVER);
        {
            cJSON *pItem = cJSON_CreateObject();
            if(pItem) {
                char *szJSON = VAR_NULL;
                sqlite3_stmt *stmt = RT_NULL;
                board_cfg_rule_del((const char *)rule->name);
                if (SQLITE_OK == sqlite3_prepare_v2(s_cfg_db, "INSERT INTO "TABLE_CFG_RULE" values(NULL,?,?,?)", -1, &stmt, 0)) {
                    sqlite3_bind_int(stmt, 1, time(0));
                    sqlite3_bind_text(stmt, 2, (const char *)rule->name, strlen(rule->name), NULL);
                    if (jsonFillRuleInfo(rule, pItem) && (szJSON = cJSON_PrintUnformatted(pItem)) != RT_NULL) {
                        sqlite3_bind_text(stmt, 3, szJSON, strlen(szJSON), NULL);
                    }
                    if (SQLITE_DONE == sqlite3_step(stmt)) {
                        ret = RT_TRUE;
                    }
                }
                rt_free(szJSON);
                if (stmt) sqlite3_finalize(stmt);
            }
            cJSON_Delete(pItem);
        }
        rt_mutex_release(&cfg_mutex);
    }
    return ret;
}


#include "cfg_recover.c"

