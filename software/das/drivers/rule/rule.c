#include <board.h>
#include <stdio.h>
#include "rule.h"
#include "ez_exp_os.h"

#define RULE_HASH_LIMIT         97

struct rule_hash_node {
    char                    key[RULE_NAME_SIZE + 1];
    struct rule_node        *rule;
    struct rule_hash_node   *next;
};

static struct rule_hash_node *s_rule_table[RULE_HASH_LIMIT];
static int s_rule_count = 0;
static pthread_mutex_t s_rule_mutex;

static void __rule_cfg_default(void)
{
    memset(s_rule_table, 0, sizeof(s_rule_table));
    s_rule_count = 0;
}

void rule_init(void)
{
    __rule_cfg_default();

    if( !board_cfg_rule_loadlist() ) {
        __rule_cfg_default();
    }

    if (rt_mutex_init(&s_rule_mutex, "rule_mtx", RT_IPC_FLAG_FIFO) != RT_EOK) {
        rt_kprintf("init rule mutex failed\n");
    }
}

void rule_lock(void)
{
    rt_mutex_take(&s_rule_mutex);
}

void rule_unlock(void)
{
    rt_mutex_release(&s_rule_mutex);
}

static int __get_hash(const char *str)
{
    // 31 131 1313 13131 131313 etc..
    unsigned int seed = 131;
    unsigned int hash = 0;

    while(*str) {
        hash = hash * seed + (*str++);
    }

    return (hash % RULE_HASH_LIMIT);
}

static void __add_custom_exp(struct rule_node *rule)
{
    if (rule) {
        int p_in_len = rule->p_in ? strlen(rule->p_in) : 0;
        int c_in_len = rule->c_in ? strlen(rule->c_in) : 0;
        int p_out_len = rule->p_out ? strlen(rule->p_out) : 0;
        int c_out_len = rule->c_out ? strlen(rule->c_out) : 0;
        char *func_in = rt_malloc(strlen(rule->name) + p_in_len + 3);
        char *func_out = rt_malloc(strlen(rule->name) + 2 + p_out_len + 3);

        if (rule->c_in && func_in) {
            sprintf(func_in, "%s(%s)", rule->name, rule->p_in ? rule->p_in : "");
            printf("func_in = %s\n", func_in);
            ez_exp_custom_new((const char *)func_in, (const char *)rule->c_in);
        }
        if (rule->c_out && func_out) {
            sprintf(func_out, "%s_o(%s)", rule->name, rule->p_out ? rule->p_out : "");
            printf("func_out = %s\n", func_out);
            ez_exp_custom_new((const char *)func_out, (const char *)rule->c_out);
        }
        if (func_in) rt_free(func_in);
        if (func_out) rt_free(func_out);
    }
}

static void __del_custom_exp(struct rule_node *rule)
{
    if (rule && rule->name) {
        char *name_out = ez_exp_strdup_ex(rule->name, "_o");
        ez_exp_custom_del((const char *)rule->name);
        if (name_out) {
            ez_exp_custom_del((const char *)name_out);
            rt_free(name_out);
        }
    }
}

struct rule_node *rule_new(
    int enable,
    const char *name, 
    enum rule_type type, 
    const char *p_in, 
    const char *c_in, 
    const char *p_out, 
    const char *c_out, 
    enum rule_sys sys)
{
    if (name) {
        struct rule_hash_node *new_node = NULL;
        int pos = __get_hash(name);
        struct rule_hash_node *head = s_rule_table[pos];
        while (head) {
            head = head->next;
        }
        new_node = (struct rule_hash_node *)malloc(sizeof(struct rule_hash_node));
        if (new_node) {
            struct rule_node *rule = (struct rule_node *)malloc(sizeof(struct rule_node));
            if (rule) {
                memset(rule, 0, sizeof(*rule));
                rule->enable = enable;
                rule->type = type;
                rule->sys = sys;
                strncpy(rule->name, name, sizeof(rule->name));
                strncpy(new_node->key, name, sizeof(new_node->key));
                if (p_in) rule->p_in = ez_exp_strdup(p_in);
                if (c_in) rule->c_in = ez_exp_strdup(c_in);
                if (p_out) rule->p_out = ez_exp_strdup(p_out);
                if (c_out) rule->c_out = ez_exp_strdup(c_out);
                new_node->rule = rule;
                new_node->next = s_rule_table[pos];
                s_rule_table[pos] = new_node;
                s_rule_count++;
                __add_custom_exp(rule);
            }
            return rule;
        }
    }
    
    return NULL;
}

int rule_del(const char *name)
{
    int result = -1;
    int pos = __get_hash(name);
    if (pos >= 0) {
        struct rule_hash_node *head = s_rule_table[pos];
        if (head) {
            struct rule_hash_node *last = NULL;
            struct rule_hash_node *remove = NULL;
            while (head) {
                if (0 == strcmp(head->key, name)) {
                    remove = head;
                    break;
                }
                last = head;
                head = head->next;
            }
            if (remove) {
                if (last) 
                    last->next = remove->next;
                else
                    s_rule_table[pos] = remove->next;

                __del_custom_exp(remove->rule);
                if (remove->rule && remove->rule->p_in) free(remove->rule->p_in);
                if (remove->rule && remove->rule->c_in) free(remove->rule->c_in);
                if (remove->rule && remove->rule->p_out) free(remove->rule->p_out);
                if (remove->rule && remove->rule->c_out) free(remove->rule->c_out);
                if (remove->rule) free(remove->rule);
                free(remove);
                s_rule_count--;
                result = 0;
            }
        }
    }

    return result;
}

struct rule_node *rule_get(const char *name)
{
    if (name && name[0]) {
        int pos = __get_hash(name);
        if (pos >= 0) {
            struct rule_hash_node *node = s_rule_table[pos];
            while (node) {
                if (0 == strcmp(node->key, name)) {
                    return node->rule;
                }
                node = node->next;
            }
        }
    }
    return NULL;
}

// for websocket

rt_bool_t jsonFillRuleInfo(struct rule_node *rule, cJSON *pItem)
{
    if (rule && pItem) {
        cJSON_AddNumberToObject(pItem, "en",  rule->enable);
        cJSON_AddStringToObject(pItem, "name",  rule->name);
        cJSON_AddNumberToObject(pItem, "type",  rule->type);
        cJSON_AddStringToObject(pItem, "p_in",  rule->p_in ? rule->p_in : "");
        cJSON_AddStringToObject(pItem, "c_in",  rule->c_in ? rule->c_in : "");
        cJSON_AddStringToObject(pItem, "p_out", rule->p_out ? rule->p_out : "");
        cJSON_AddStringToObject(pItem, "c_out", rule->c_out ? rule->c_out : "");
        cJSON_AddNumberToObject(pItem, "sys",   rule->sys);
        return RT_TRUE;
    }

    return RT_FALSE;
}

rt_bool_t jsonParseRuleInfo(cJSON *pItem, struct rule_node *rule)
{
    if (rule && pItem) {
        int itmp;
        const char *str;
    
        rule->enable = cJSON_GetInt(pItem, "en", 0);

        str = cJSON_GetString(pItem, "name", RT_NULL);
        if (str) {
            memset(rule->name, 0, sizeof(rule->name)); strncpy(rule->name, str, sizeof(rule->name));
        }
        rule->type = cJSON_GetInt(pItem, "type", RULE_TYPE_ERR);
        
        str = cJSON_GetString(pItem, "p_in", RT_NULL);
        rt_free(rule->p_in);
        rule->p_in = NULL;
        if (str && str[0]) {
            rule->p_in = rt_strdup(str);
        }
        str = cJSON_GetString(pItem, "c_in", RT_NULL);
        rt_free(rule->c_in);
        rule->c_in = NULL;
        if (str && str[0]) {
            rule->c_in = rt_strdup(str);
        }

        str = cJSON_GetString(pItem, "p_out", RT_NULL);
        rt_free(rule->p_out);
        rule->p_out = NULL;
        if (str && str[0]) {
            rule->p_out = rt_strdup(str);
        }
        str = cJSON_GetString(pItem, "c_out", RT_NULL);
        rt_free(rule->c_out);
        rule->c_out = NULL;
        if (str && str[0]) {
            rule->c_out = rt_strdup(str);
        }
        rule->sys = cJSON_GetInt(pItem, "sys", RULE_SYS_ERR);
        return RT_TRUE;
    }
    return RT_FALSE;
}

DEF_CGI_HANDLER(getRuleList)
{
    rt_err_t err = RT_EOK;
    char *szRetJSON = RT_NULL;
    rt_bool_t first = RT_TRUE;

    WEBS_PRINTF("{\"ret\":0,\"list\":[");
    for (int i = 0; i < RULE_HASH_LIMIT; i++ ) {
        struct rule_hash_node *node = s_rule_table[i];
        while (node) {
            cJSON *pItem = cJSON_CreateObject();
            if(pItem) {
                if (!first) WEBS_PRINTF(",");
                first = RT_FALSE;
                jsonFillRuleInfo(node->rule, pItem);
                szRetJSON = cJSON_PrintUnformatted(pItem);
                if(szRetJSON) {
                    WEBS_PRINTF(szRetJSON);
                    rt_free(szRetJSON);
                }
            }
            cJSON_Delete(pItem);
            node = node->next;
        }
    }
    WEBS_PRINTF("]}");

    if (err != RT_EOK) {
        WEBS_PRINTF("{\"ret\":%d}", err);
    }
    WEBS_DONE(200);
}

void setRuleWithJson(cJSON *pCfg)
{
    struct rule_node rule_bak;
    const char *name = cJSON_GetString(pCfg, "name", RT_NULL);
    if (name) {
        struct rule_node *rule = rt_calloc(1, sizeof(*rule));
        if (rule) {
            rule->type = RULE_TYPE_ERR;
            rule->sys = RULE_SYS_ERR;
            if (jsonParseRuleInfo(pCfg, rule)) {
                rule_del(name);
                board_cfg_rule_del(name);
                rule_new(rule->enable, rule->name, rule->type, rule->p_in, rule->c_in, rule->p_out, rule->c_out, rule->sys);
                board_cfg_rule_add(rule);
            }
            rt_free(rule);
        }
    }
}

DEF_CGI_HANDLER(setRule)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        setRuleWithJson(pCfg);
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}

DEF_CGI_HANDLER(addRule)
{
    rt_err_t err = RT_ERROR;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        const char *name = cJSON_GetString(pCfg, "name", RT_NULL);
        if (name && name[0] && !rule_get(name)) {
            setRuleWithJson(pCfg);
            err = RT_EOK;
        }
    }
    cJSON_Delete(pCfg);

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}

DEF_CGI_HANDLER(delRule)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        const char *name = cJSON_GetString(pCfg, "name", RT_NULL);
        if (name && strlen(name) > 0) {
            if (board_cfg_rule_del(name)) {
                rule_del(name);
            } else {
                err = RT_ERROR;
            }
        }
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}

