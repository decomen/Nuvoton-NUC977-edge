
/*
 * File      : lora_std.c
 *
 * All rights reserved.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2016-07-19     Jay      the first version
 */

// 双链表结构, 便于后期扩展
// 子节点链表有队列,要注意空间分配和释放

#include <board.h>
#include <time.h>

static lora_mnode_t *s_lora_mnode = RT_NULL;
static pthread_mutex_t s_lora_std_mutex;

static lora_snode_t* _lora_snode_find_with_type(lora_mnode_t *mnode, elora_sn_type_t type);
static lora_snode_t* _lora_snode_find_with_addr(lora_mnode_t *mnode, elora_sn_type_t type, int addr);
static void _lora_snode_rm_one(lora_mnode_t *mnode, elora_sn_type_t type);
static void _lora_snode_rm_all(lora_mnode_t *mnode);
static lora_snode_t* _lora_snode_insert(lora_mnode_t *mnode, lora_std_info_t *info);

// 初始化
void lora_std_init(void)
{
    s_lora_mnode = RT_NULL;
    if (rt_mutex_init(&s_lora_std_mutex, "lorastd", RT_IPC_FLAG_PRIO) != RT_EOK) {
        rt_kprintf("init lorastd mutex failed\n");
    }
}

// 匹配子节点
static lora_snode_t* _lora_snode_find_with_type(lora_mnode_t *mnode, elora_sn_type_t type)
{
    if (mnode) {
        lora_snode_t *node = mnode->snode;
        while (node) {
            if (node->type == type) {
                return node;
            }
            node = node->next;
        }
    }

    return RT_NULL;
}

// 匹配子节点
static lora_snode_t* _lora_snode_find_with_addr(lora_mnode_t *mnode, elora_sn_type_t type, int addr)
{
    if (mnode) {
        lora_snode_t *node = mnode->snode;
        while (node) {
            if (node->type == type && node->cnt > 0 && node->lst) {
                for (int i = 0; i < node->cnt; i++) {
                    if (node->lst[i].addr_8 == addr) {
                        return node;
                    }
                }
            }
            node = node->next;
        }
    }

    return RT_NULL;
}

// 移除子节点匹配项
static void _lora_snode_rm_one(lora_mnode_t *mnode, elora_sn_type_t type)
{
    if (mnode) {
        lora_snode_t *node = mnode->snode;
        while (node) {
            lora_snode_t *prev = node->prev;
            lora_snode_t *next = node->next;
            if (node->type == type) {
                if (prev) {
                    prev->next = next;
                } else {    // 头结点后移
                    mnode->snode = next;
                }
                if (next) next->prev = prev;
                rt_free(node->lst);
                rt_free(node);
                break;
            }
            node = next;
        }
    }
}

// 移除子节点
static void _lora_snode_rm_all(lora_mnode_t *mnode)
{
    if (mnode) {
        lora_snode_t *node = mnode->snode;
        while (node) {
            lora_snode_t *next = node->next;
            rt_free(node->lst);
            rt_free(node);
            node = next;
        }
    }
    mnode->snode = RT_NULL;
}

static lora_snode_t* _lora_snode_insert(lora_mnode_t *mnode, lora_std_info_t *info)
{
    if (mnode) {
        lora_snode_t *last = RT_NULL;
        lora_snode_t *node = rt_calloc(1, sizeof(lora_snode_t));
        uint8_t *data = info->data;

        _lora_snode_rm_one(mnode, info->type);
        node->type = info->type;
        node->cnt = info->cnt;
        node->lst = rt_calloc(info->cnt, sizeof(lora_sn_val_t));
        node->prev = RT_NULL;
        node->next = RT_NULL;

        switch (info->type) {
        case LORA_SN_T_MODBUS_RTU: case LORA_SN_T_MODBUS_ASCII:
            for (int i = 0; i < info->cnt; i++) {
                memcpy(&node->lst[i].addr_8, data, 1);
                data += 1;
            }
            break;
        }

        last = mnode->snode;
        while (last && last->next) last = last->next;
        if (last) {
            last->next = node;
            node->prev = last;
        } else {
            mnode->snode = node;
        }

        return node;
    }

    return RT_NULL;
}

static uint32_t _snode_num_with_type(lora_mnode_t *mnode, elora_sn_type_t type)
{
    uint32_t num = 0;
    if (mnode) {
        lora_snode_t *node = mnode->snode;
        while (node) {
            if (type == node->type) {
                num += node->cnt;
            }
            node = node->next;
        }
    }
    return num;
}

static uint32_t _snode_num_all(lora_mnode_t *mnode)
{
    uint32_t num = 0;
    if (mnode) {
        lora_snode_t *node = mnode->snode;
        while (node) {
            num += node->cnt;
            node = node->next;
        }
    }
    return num;
}

// 查找节点
lora_mnode_t* lora_mnode_find_with_netid(uint32_t netid)
{
    lora_mnode_t *retnode = RT_NULL;
    rt_mutex_take(&s_lora_std_mutex, RT_WAITING_FOREVER);
    lora_mnode_t *node = s_lora_mnode;
    while (node) {
        if (node->netid == netid) {
            retnode = node;
            break;
        }
        node = node->next;
    }
    rt_mutex_release(&s_lora_std_mutex);
    return retnode;
}

// 查找节点
lora_mnode_t* lora_mnode_find_with_addr(elora_sn_type_t type, int addr)
{
    lora_mnode_t *retnode = RT_NULL;
    rt_mutex_take(&s_lora_std_mutex, RT_WAITING_FOREVER);
    lora_mnode_t *node = s_lora_mnode;
    while (node) {
        if (_lora_snode_find_with_addr(node, type, addr)) {
            retnode = node;
            break;
        }
        node = node->next;
    }
    rt_mutex_release(&s_lora_std_mutex);
    return retnode;
}

// 设置目标节点
rt_bool_t lora_set_dst_node(elora_sn_type_t type, int addr)
{
    rt_bool_t ret = RT_FALSE;
    rt_mutex_take(&s_lora_std_mutex, RT_WAITING_FOREVER);
    lora_mnode_t *mnode = lora_mnode_find_with_addr(type, addr);
    if (mnode != RT_NULL && mnode->online) {
        ret = (0 == lora_set_dst_addr(mnode->netid));
    }
    rt_mutex_release(&s_lora_std_mutex);

    return ret;
}

// 根据网络地址移除
void lora_mnode_rm_with_id(uint32_t netid)
{
    rt_mutex_take(&s_lora_std_mutex, RT_WAITING_FOREVER);
    lora_mnode_t *node = s_lora_mnode;

    while (node) {
        lora_mnode_t *prev = node->prev;
        lora_mnode_t *next = node->next;
        if (node->netid == netid) {
            if (prev) {
                prev->next = next;
            } else {    // 头结点后移
                s_lora_mnode = next;
            }
            if (next) next->prev = prev;
            _lora_snode_rm_all(node);
            rt_free(node);
            break;
        }
        node = next;
    }
    rt_mutex_release(&s_lora_std_mutex);
}

// 移除所有节点
void lora_mnode_rm_all(void)
{
    rt_mutex_take(&s_lora_std_mutex, RT_WAITING_FOREVER);
    lora_mnode_t *node = s_lora_mnode;
    while (node) {
        lora_mnode_t *next = node->next;
        _lora_snode_rm_all(node);
        rt_free(node);
        node = next;
    }
    s_lora_mnode = RT_NULL;
    rt_mutex_release(&s_lora_std_mutex);
}

// 插入数据
lora_mnode_t* lora_mnode_insert(lora_std_info_t *info, uint8_t rssi)
{
    lora_mnode_t *retnode = RT_NULL;
    rt_mutex_take(&s_lora_std_mutex, RT_WAITING_FOREVER);
    if (info) {
        lora_mnode_t *node = lora_mnode_find_with_netid(info->netid);

        if (RT_NULL == node) {
            lora_mnode_t *last = RT_NULL;

            node = rt_calloc(1, sizeof(lora_mnode_t));
            node->next = RT_NULL;
            node->prev = RT_NULL;
            node->workmode = info->workmode;
            node->netid = info->netid;
            node->id = info->id;
            node->online = RT_TRUE;
            node->rssi = rssi;
            node->rssi_dB = INVALID_RSSI;
            node->snr = INVALID_SNR;
            node->uptime = time(0);
            node->lasttime = node->uptime;
            node->last_hrt_time = node->lasttime;
            node->offtime = 0;
            node->snode = RT_NULL;

            last = s_lora_mnode;
            while (last && last->next) last = last->next;
            if (last) {
                last->next = node;
                node->prev = last;
            } else {
                s_lora_mnode = node;
            }
        } else {
            node->workmode = info->workmode;
            node->netid = info->netid;
            node->rssi = rssi;
            node->lasttime = time(0);
            node->last_hrt_time = node->lasttime;
            if (!node->online) {
                node->online = RT_TRUE;
                node->uptime = node->lasttime;
                node->offtime = 0;
            }
            if (node->uptime < 1546272000) node->uptime = node->lasttime;
        }

        _lora_snode_insert(node, info);

        retnode = node;
    }
    rt_mutex_release(&s_lora_std_mutex);

    return retnode;
}

lora_mnode_t* lora_mnode_online(uint32_t netid, uint8_t rssi)
{
    lora_mnode_t *node = RT_NULL;
    rt_mutex_take(&s_lora_std_mutex, RT_WAITING_FOREVER);
    node = lora_mnode_find_with_netid(netid);

    if (node) {
        node->rssi = rssi;
        node->lasttime = time(0);
        node->last_hrt_time = node->lasttime;
        if (!node->online) {
            node->online = RT_TRUE;
            node->uptime = node->lasttime;
            node->offtime = 0;
        }
        if (node->uptime < 1546272000) node->uptime = node->lasttime;
    }
    rt_mutex_release(&s_lora_std_mutex);

    return node;
}

lora_mnode_t* lora_mnode_check_hrt(void)
{
    lora_mnode_t *node = s_lora_mnode;
    rt_mutex_take(&s_lora_std_mutex, RT_WAITING_FOREVER);
    
    while (node) {
        if (node->online && time(0) - node->last_hrt_time > 15) {
            node->last_hrt_time = time(0);
            break;
        }
        node = node->next;
    }
    rt_mutex_release(&s_lora_std_mutex);

    return node;
}

lora_mnode_t *lora_mnode_update_rssi(lora_std_rssi_t *rssi, uint32_t netid, uint8_t rssi_p)
{
    lora_mnode_t *node = RT_NULL;
    rt_mutex_take(&s_lora_std_mutex, RT_WAITING_FOREVER);
    node = lora_mnode_find_with_netid(netid);

    if (node) {
        node->rssi = rssi_p;
        node->snr = rssi->snr;
        node->rssi_dB = rssi->rssi;
        node->lasttime = time(0);
        node->last_hrt_time = node->lasttime;
        node->last_rssi_time = node->lasttime;
        if (!node->online) {
            node->online = RT_TRUE;
            node->uptime = node->lasttime;
            node->offtime = 0;
        }
        if (node->uptime < 1546272000) node->uptime = node->lasttime;
    }
    rt_mutex_release(&s_lora_std_mutex);

    return node;
}

lora_mnode_t* lora_mnode_check_rssi(void)
{
    lora_mnode_t *node = s_lora_mnode;
    rt_mutex_take(&s_lora_std_mutex, RT_WAITING_FOREVER);
    
    while (node) {
        if (node->online && time(0) - node->last_rssi_time > 60) {
            node->last_rssi_time = time(0);
            break;
        }
        node = node->next;
    }
    rt_mutex_release(&s_lora_std_mutex);

    return node;
}

uint32_t lora_mnode_num(void)
{
    uint32_t num = 0;
    rt_mutex_take(&s_lora_std_mutex, RT_WAITING_FOREVER);
    lora_mnode_t *node = s_lora_mnode;
    while (node) {
        node = node->next;
        num++;
    }
    rt_mutex_release(&s_lora_std_mutex);
    return num;
}

void lora_check_online(void)
{
    rt_mutex_take(&s_lora_std_mutex, RT_WAITING_FOREVER);
    lora_mnode_t *node = s_lora_mnode;
    uint32_t nowtime = time(0);

    while (node) {
        lora_mnode_t *prev = node->prev;
        lora_mnode_t *next = node->next;
        // 判断下线
        if (node->online) {
            if (node->lasttime < 1546272000) node->lasttime = nowtime;
            if (node->uptime < 1546272000) node->uptime = node->lasttime;
            if (node->lasttime > nowtime || nowtime - node->lasttime >= (g_lora_cfg.learnstep * 4)) {
                if (node->online) {
                    node->online = RT_FALSE;
                    node->offtime = nowtime - (g_lora_cfg.learnstep * 3);
                }
            }
            // 移除下线节点
        } else if (nowtime - node->offtime >= (g_lora_cfg.learnstep * 10)) {
            if (prev) {
                prev->next = next;
            } else {    // 头结点后移
                s_lora_mnode = next;
            }
            if (next) next->prev = prev;
            _lora_snode_rm_all(node);
            rt_free(node);
        }
        node = next;
    }
    rt_mutex_release(&s_lora_std_mutex);
}

int lora_std_lock(void)
{
    return rt_mutex_take(&s_lora_std_mutex, RT_WAITING_FOREVER);
}

int lora_std_trylock(void)
{
    return rt_mutex_try_take(&s_lora_std_mutex);
}

void lora_std_unlock(void)
{
    rt_mutex_release(&s_lora_std_mutex);
}

static void json_lora_mnode(lora_mnode_t *mnode, cJSON *pItem)
{
    char buf[64];
    struct tm lt;

    if (mnode) {
        memset(buf, 0, sizeof(buf));
        memcpy(buf, mnode->id.sn, 16);
        cJSON_AddStringToObject(pItem, "id", buf);
        cJSON_AddNumberToObject(pItem, "type0", mnode->id.type0);
        cJSON_AddNumberToObject(pItem, "type1", mnode->id.type1);
        cJSON_AddNumberToObject(pItem, "mode", mnode->workmode);
        rt_sprintf(buf, "%08X", mnode->netid);
        cJSON_AddStringToObject(pItem, "netid", buf);
        cJSON_AddNumberToObject(pItem, "on", mnode->online);
        cJSON_AddNumberToObject(pItem, "rssi", mnode->rssi);
        cJSON_AddNumberToObject(pItem, "rssi_db", mnode->rssi_dB);
        cJSON_AddNumberToObject(pItem, "snr", mnode->snr);
        memset(buf, 0, sizeof(buf));
        if (mnode->uptime > 0) {
            das_localtime_r((time_t *)&mnode->uptime, &lt);
            sprintf(buf, "%u/%02d/%02d %02d:%02d:%02d",
                    lt.tm_year + 1900,
                    lt.tm_mon + 1,
                    lt.tm_mday,
                    lt.tm_hour,
                    lt.tm_min,
                    lt.tm_sec
                   );
        }
        cJSON_AddStringToObject(pItem, "upt", buf);
        memset(buf, 0, sizeof(buf));
        if (mnode->offtime > 0) {
            das_localtime_r((time_t *)&mnode->offtime, &lt);
            sprintf(buf, "%u/%02d/%02d %02d:%02d:%02d",
                    lt.tm_year + 1900,
                    lt.tm_mon + 1,
                    lt.tm_mday,
                    lt.tm_hour,
                    lt.tm_min,
                    lt.tm_sec
                   );
        }
        cJSON_AddStringToObject(pItem, "offt", buf);

        if (mnode->snode && mnode->snode->cnt > 0) {
            char *szlst = rt_calloc(1, mnode->snode->cnt * 4 + 12);
            if(szlst) {
                rt_bool_t first = RT_TRUE;
                //lora_mnode_t *node = mnode;
                strcat(szlst, "[");
                for (int i = 0; i < mnode->snode->cnt; i++) {
                    char sznum[4] = { 0 };
                    if (!first) strcat(szlst, ",");
                    first = RT_FALSE;
                    sprintf(sznum, "%d", mnode->snode->lst[i].addr_8);
                    strcat(szlst, sznum);
                }
                strcat(szlst, "]");
                cJSON_AddStringToObject(pItem, "adlst", szlst);
                rt_free(szlst);
            }
        } else {
            cJSON_AddStringToObject(pItem, "adlst", "");
        }
    }
}

void xfer_json_lora_mnode(cJSON *pItem)
{
    lora_mnode_t *mnode = s_lora_mnode;
    if (mnode) {
        cJSON_AddNumberToObject(pItem, "n", PROTO_DEV_ZIGBEE);
        cJSON_AddNumberToObject(pItem, "en", mnode->snode->cnt>0?1:0);
        cJSON_AddNumberToObject(pItem, "cnt", mnode->snode->cnt);
        if(mnode->snode->cnt > 0) {
            char *szlst = rt_calloc(1, mnode->snode->cnt * 4 + 12);
            if(szlst) {
                rt_bool_t first = RT_TRUE;
                for (int i = 0; i < mnode->snode->cnt; i++) {
                    char sznum[4] = { 0 };
                    if (!first) strcat(szlst, ",");
                    first = RT_FALSE;
                    sprintf(sznum, "%u", mnode->snode->lst[i].addr_8);
                    strcat(szlst, sznum);
                }
                cJSON_AddStringToObject(pItem, "addrs", szlst);
                rt_free(szlst);
            }
        } else {
            cJSON_AddStringToObject(pItem, "addrs", "");
        }
    }
}


DEF_CGI_HANDLER(getLoRaList)
{
    char *szRetJSON = RT_NULL;
    lora_mnode_t *node = s_lora_mnode;

    WEBS_PRINTF("{\"ret\":0,\"list\":[");

    rt_bool_t first = RT_TRUE;
    while (node) {
        cJSON *pItem = cJSON_CreateObject();
        if(pItem) {
            if (!first) WEBS_PRINTF(",");
            first = RT_FALSE;
            json_lora_mnode(node, pItem);
            szRetJSON = cJSON_PrintUnformatted(pItem);
            if(szRetJSON) {
                WEBS_PRINTF(szRetJSON);
                rt_free(szRetJSON);
            }
        }
        cJSON_Delete(pItem);
        node = node->next;
    }
    WEBS_PRINTF("]}");
    WEBS_DONE(200);
}

