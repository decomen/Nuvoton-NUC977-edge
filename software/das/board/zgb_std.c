
/*
 * File      : zgb_std.c
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

static zgb_mnode_t *s_zgb_mnode = RT_NULL;
static pthread_mutex_t s_zgb_std_mutex;

static zgb_snode_t* _snode_find_with_type(zgb_mnode_t *mnode, ezgb_sn_type_t type);
static zgb_snode_t* _snode_find_with_addr(zgb_mnode_t *mnode, ezgb_sn_type_t type, int addr);
static void _snode_rm_one(zgb_mnode_t *mnode, ezgb_sn_type_t type);
static void _snode_rm_all(zgb_mnode_t *mnode);
static zgb_snode_t* _snode_insert(zgb_mnode_t *mnode, zgb_std_scan_t *info);

// 初始化
void zgb_std_init(void)
{
    s_zgb_mnode = RT_NULL;
    if (rt_mutex_init(&s_zgb_std_mutex, "zgbstd", RT_IPC_FLAG_PRIO) != RT_EOK) {
        rt_kprintf("init zgbstd mutex failed\n");
    }
}

// 匹配子节点
static zgb_snode_t* _snode_find_with_type(zgb_mnode_t *mnode, ezgb_sn_type_t type)
{
    if (mnode) {
        zgb_snode_t *node = mnode->snode;
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
static zgb_snode_t* _snode_find_with_addr(zgb_mnode_t *mnode, ezgb_sn_type_t type, int addr)
{
    if (mnode) {
        zgb_snode_t *node = mnode->snode;
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
static void _snode_rm_one(zgb_mnode_t *mnode, ezgb_sn_type_t type)
{
    if (mnode) {
        zgb_snode_t *node = mnode->snode;
        while (node) {
            zgb_snode_t *prev = node->prev;
            zgb_snode_t *next = node->next;
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
static void _snode_rm_all(zgb_mnode_t *mnode)
{
    if (mnode) {
        zgb_snode_t *node = mnode->snode;
        while (node) {
            zgb_snode_t *next = node->next;
            rt_free(node->lst);
            rt_free(node);
            node = next;
        }
    }
    mnode->snode = RT_NULL;
}

static zgb_snode_t* _snode_insert(zgb_mnode_t *mnode, zgb_std_scan_t *info)
{
    if (mnode) {
        zgb_snode_t *last = RT_NULL;
        zgb_snode_t *node = rt_calloc(1, sizeof(zgb_snode_t));
        rt_uint8_t *data = info->data;

        _snode_rm_one(mnode, info->type);
        node->type = info->type;
        node->cnt = info->cnt;
        node->lst = rt_calloc(info->cnt, sizeof(zgb_sn_val_t));
        node->prev = RT_NULL;
        node->next = RT_NULL;

        switch (info->type) {
        case ZGB_SN_T_MODBUS_RTU: case ZGB_SN_T_MODBUS_ASCII:
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

static rt_uint32_t _snode_num_with_type(zgb_mnode_t *mnode, ezgb_sn_type_t type)
{
    rt_uint32_t num = 0;
    if (mnode) {
        zgb_snode_t *node = mnode->snode;
        while (node) {
            if (type == node->type) {
                num += node->cnt;
            }
            node = node->next;
        }
    }
    return num;
}

static rt_uint32_t _snode_num_all(zgb_mnode_t *mnode)
{
    rt_uint32_t num = 0;
    if (mnode) {
        zgb_snode_t *node = mnode->snode;
        while (node) {
            num += node->cnt;
            node = node->next;
        }
    }
    return num;
}

// 查找节点
zgb_mnode_t* zgb_mnode_find_with_netid(rt_uint16_t netid)
{
    zgb_mnode_t *retnode = RT_NULL;
    rt_mutex_take(&s_zgb_std_mutex, RT_WAITING_FOREVER);
    zgb_mnode_t *node = s_zgb_mnode;
    while (node) {
        if (node->netid == netid) {
            retnode = node;
            break;
        }
        node = node->next;
    }
    rt_mutex_release(&s_zgb_std_mutex);
    return retnode;
}

// 查找节点
zgb_mnode_t* zgb_mnode_find_with_addr(ezgb_sn_type_t type, int addr)
{
    zgb_mnode_t *retnode = RT_NULL;
    rt_mutex_take(&s_zgb_std_mutex, RT_WAITING_FOREVER);
    zgb_mnode_t *node = s_zgb_mnode;
    while (node) {
        if (_snode_find_with_addr(node, type, addr)) {
            retnode = node;
            break;
        }
        node = node->next;
    }
    rt_mutex_release(&s_zgb_std_mutex);
    return retnode;
}

// 设置目标节点
rt_bool_t zgb_set_dst_node(ezgb_sn_type_t type, int addr)
{
    rt_bool_t ret = RT_FALSE;
    rt_mutex_take(&s_zgb_std_mutex, RT_WAITING_FOREVER);
    zgb_mnode_t *mnode = zgb_mnode_find_with_addr(type, addr);
    if (mnode != RT_NULL && mnode->online) {
        ret = (0 == ucZigbeeSetDstAddr(mnode->netid));
    }
    rt_mutex_release(&s_zgb_std_mutex);

    return ret;
}

// 根据网络地址移除
void zgb_mnode_rm_with_id(rt_uint16_t netid)
{
    rt_mutex_take(&s_zgb_std_mutex, RT_WAITING_FOREVER);
    zgb_mnode_t *node = s_zgb_mnode;

    while (node) {
        zgb_mnode_t *prev = node->prev;
        zgb_mnode_t *next = node->next;
        if (node->netid == netid) {
            if (prev) {
                prev->next = next;
            } else {    // 头结点后移
                s_zgb_mnode = next;
            }
            if (next) next->prev = prev;
            _snode_rm_all(node);
            rt_free(node);
            break;
        }
        node = next;
    }
    rt_mutex_release(&s_zgb_std_mutex);
}

// 移除所有节点
void zgb_mnode_rm_all(void)
{
    rt_mutex_take(&s_zgb_std_mutex, RT_WAITING_FOREVER);
    zgb_mnode_t *node = s_zgb_mnode;
    while (node) {
        zgb_mnode_t *next = node->next;
        _snode_rm_all(node);
        rt_free(node);
        node = next;
    }
    s_zgb_mnode = RT_NULL;
    rt_mutex_release(&s_zgb_std_mutex);
}

// 插入数据
zgb_mnode_t* zgb_mnode_insert(zgb_std_scan_t *info)
{
    zgb_mnode_t *retnode = RT_NULL;
    rt_mutex_take(&s_zgb_std_mutex, RT_WAITING_FOREVER);
    if (info) {
        zgb_mnode_t *node = zgb_mnode_find_with_netid(info->netid);

        if (RT_NULL == node) {
            zgb_mnode_t *last = RT_NULL;

            node = rt_calloc(1, sizeof(zgb_mnode_t));
            node->next = RT_NULL;
            node->prev = RT_NULL;
            node->workmode = info->workmode;
            node->netid = info->netid;
            node->mac = info->mac;
            node->online = RT_TRUE;
            node->rssi = info->rssi;
            node->uptime = time(0);
            node->lasttime = node->uptime;
            node->offtime = 0;
            node->snode = RT_NULL;

            last = s_zgb_mnode;
            while (last && last->next) last = last->next;
            if (last) {
                last->next = node;
                node->prev = last;
            } else {
                s_zgb_mnode = node;
            }
        } else {
            node->workmode = info->workmode;
            node->netid = info->netid;
            node->rssi = info->rssi;
            node->lasttime = time(0);
            if (!node->online) {
                node->online = RT_TRUE;
                node->uptime = node->lasttime;
                node->offtime = 0;
            }
        }

        _snode_insert(node, info);

        retnode = node;
    }
    rt_mutex_release(&s_zgb_std_mutex);

    return retnode;
}

rt_uint32_t zgb_mnode_num(void)
{
    rt_uint32_t num = 0;
    rt_mutex_take(&s_zgb_std_mutex, RT_WAITING_FOREVER);
    zgb_mnode_t *node = s_zgb_mnode;
    while (node) {
        node = node->next;
        num++;
    }
    rt_mutex_release(&s_zgb_std_mutex);
    return num;
}

void zgb_check_online(void)
{
    rt_mutex_take(&s_zgb_std_mutex, RT_WAITING_FOREVER);
    zgb_mnode_t *node = s_zgb_mnode;
    rt_uint32_t nowtime = time(0);

    while (node) {
        zgb_mnode_t *prev = node->prev;
        zgb_mnode_t *next = node->next;
        // 判断下线
        if (node->online) {
            if (node->lasttime > nowtime || nowtime - node->lasttime >= (g_zigbee_cfg.ulLearnStep * 4)) {
                if (node->online) {
                    node->online = RT_FALSE;
                    node->offtime = nowtime - (g_zigbee_cfg.ulLearnStep * 3);
                }
            }
            // 移除下线节点
        } else if (nowtime - node->offtime >= (g_zigbee_cfg.ulLearnStep * 10)) {
            if (prev) {
                prev->next = next;
            } else {    // 头结点后移
                s_zgb_mnode = next;
            }
            if (next) next->prev = prev;
            _snode_rm_all(node);
            rt_free(node);
        }
        node = next;
    }
    rt_mutex_release(&s_zgb_std_mutex);
}

zgb_mnode_t* zgb_std_parse_buffer(zgb_std_head_t *head, rt_uint8_t buffer[])
{
    zgb_mnode_t *retnode = RT_NULL;
    rt_mutex_take(&s_zgb_std_mutex, RT_WAITING_FOREVER);
    if (ZGB_STD_FA_RSP == head->packtype) {
        if (ZGB_STD_MSG_SCAN == head->msgtype) {
            zgb_std_scan_t *info = (zgb_std_scan_t *)buffer;
            retnode = zgb_mnode_insert(info);
        }
    }
    rt_mutex_release(&s_zgb_std_mutex);

    return retnode;
}

static void json_zgb_mnode(zgb_mnode_t *mnode, cJSON *pItem)
{
    char buf[64];
    struct tm lt;

    if (mnode) {
        rt_sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
                   mnode->mac.mac[0], mnode->mac.mac[1], mnode->mac.mac[2], mnode->mac.mac[3],
                   mnode->mac.mac[4], mnode->mac.mac[5], mnode->mac.mac[6], mnode->mac.mac[7]);
        cJSON_AddNumberToObject(pItem, "mode", mnode->workmode);
        cJSON_AddStringToObject(pItem, "mac", buf);
        cJSON_AddNumberToObject(pItem, "netid", mnode->netid);
        cJSON_AddNumberToObject(pItem, "on", mnode->online);
        cJSON_AddNumberToObject(pItem, "rssi", mnode->rssi);
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
                //zgb_mnode_t *node = mnode;
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

void xfer_json_zgb_mnode(cJSON *pItem)
{
    zgb_mnode_t *mnode = s_zgb_mnode;
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


DEF_CGI_HANDLER(getZigbeeList)
{
    char *szRetJSON = RT_NULL;
    zgb_mnode_t *node = s_zgb_mnode;

    WEBS_PRINTF("{\"ret\":0,\"list\":[");

    rt_bool_t first = RT_TRUE;
    while (node) {
        cJSON *pItem = cJSON_CreateObject();
        if(pItem) {
            if (!first) WEBS_PRINTF(",");
            first = RT_FALSE;
            json_zgb_mnode(node, pItem);
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

