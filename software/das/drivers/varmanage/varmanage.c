#include <board.h>
#include <stdio.h>
#include "obmodbus.h"
#include "modbus.h"
#include "dlt645.h"
#include "dlt645_1997.h"
#include "mbus_603.h"
#include "sdccp_dh.h"
#include "net_helper.h"
#include "sdccp_dust.h"
#include "sdccp_smf.h"
#include "modbus_helper.h"
#include "obmodbus_helper.h"

typedef struct group_node {
    int use;
    int dev_type;
    int dev_num;
    int dev_type_0;
    int dev_type_1;
    char sn[32];
} group_node_t;

static int group_search_dev_type = 0;
static int group_search_dev_num = 0;
static int group_search_flag = 0;
static int group_search_slave = 0;

const int g_var_type_sz[255] = VAR_TYPE_SIZE_ARRAY;

typedef struct varmanage_run {
    var_uint16_t    dev_type;
    var_uint16_t    dev_num;
    rt_thread_t     thread;
    int             use;
} varmanage_run_t;

static varmanage_run_t s_varmanage_run[BOARD_COLL_MAX];

// 1024-4096 采集变量地址映射表
var_uint16_t        *g_xExtDataRegs = &g_modbus_regs[USER_REG_EXT_DATA_START];
var_bool_t          g_xExtDataRegsFlag[USER_REG_EXT_DATA_SIZE];

static rt_tick_t s_com_last_tick[BOARD_UART_MAX] = {0};
static rt_tick_t s_net_last_tick[BOARD_TCPIP_MAX] = {0};

// 同帧地址链表 2级链表

#define _SYNC_EXT_MAX_SIZE          (100)    //同帧链最多每次读取32个寄存器

typedef struct _sync_ext_t {
    union {
        struct {
            var_uint16_t    addr;           // 映射地址
        } self;
        struct {
            var_uint16_t    addr;           // 升序
            var_uint16_t    ofs;            // 偏移
            var_uint8_t     size;           // 寄存器数目
        } modbus;
        struct {
            var_uint16_t    addr;           // 映射地址
            var_uint32_t    op;             // 功能码
        } dlt645;
        struct {
            var_uint16_t    addr;           // 映射地址
            var_uint8_t     op;             // 功能码
        } dust;
        struct {
            var_uint16_t    addr;           // 映射地址
            var_uint8_t     op;             // 功能码
        } mbus603;
        struct {
            var_uint16_t    addr;           // 映射地址
            var_uint8_t     op;             // 功能码
        } smf;
        struct {
            var_uint16_t    addr;           // 映射地址
            var_uint16_t    ofs;            // 偏移
            var_uint8_t     size;           // 寄存器数目
        } obmodbus;
        struct {
            var_uint16_t    addr;           // 映射地址
            dh_key_t        key;            //
        } dh;
        // ...
    } param;
    struct _sync_ext_t *next,*prev; // 链表管理
} sync_ext_t;

typedef struct _sync_slave_t {
    var_uint8_t     dev_type;       // 关联硬件
    var_uint8_t     dev_num;        // 关联硬件编号
    var_uint8_t     proto_type;     // 关联协议编号
    union {
        // modbus 按 从机地址+同帧地址 分组
        struct {
            var_uint8_t     op;             // 功能码
            var_uint8_t     slave_addr;     // 从机地址
            var_int8_t      fa_addr;        // 同帧地址
        } modbus;
        // dlt645 按 设备地址 分组
        struct {
            dlt645_addr_t   addr;           // 设备地址
        } dlt645;
        struct {
            mbus603_addr_t      addr;           // 设备地址
        } mbus603;
        struct {
            var_uint8_t     op;             // 功能码
            var_uint8_t     slave_addr;     // 从机地址
        } obmodbus;
        // dh 按 sid, type 分组
        struct {
            var_uint32_t    sid;            //
            var_uint32_t    type;           //
        } dh;
        // ...
    } param;
    struct _sync_ext_t  *ext_list;  // 采集链(按地址升序)
    struct _sync_slave_t *next;     // 链表管理
} sync_slave_t;

typedef struct _sync_node_t {
    var_uint8_t     dev_type;       // 关联硬件
    var_uint8_t     dev_num;        // 关联硬件编号
    var_uint8_t     proto_type;     // 关联协议编号
    union {
        struct {
            var_uint16_t    addr;           // 映射地址
        } self;
        struct {
            var_uint8_t     op;             // 功能码
            var_uint8_t     slave_addr;     // 从机地址
            var_uint16_t    ext_addr;       // 升序
            var_uint16_t    ext_ofs;        // 偏移
            var_uint8_t     ext_size;       // 寄存器数目
        } modbus;
        struct {
            dlt645_addr_t   dlt645_addr;    // 设备地址
            var_uint16_t    addr;           // 映射地址
            var_uint32_t    op;             // 功能码
        } dlt645;
        struct {
            mbus603_addr_t      mbus_addr;      // 设备地址
            var_uint16_t    addr;           // 映射地址
            var_uint32_t    op;             // 功能码
        } mbus603;
        struct {
            var_uint16_t    addr;           // 映射地址
            var_uint8_t     op;             // 功能码
        } dust;
        struct {
            var_uint16_t    addr;           // 映射地址
            var_uint8_t     op;             // 功能码
        } smf;
        struct {
            var_uint8_t     op;             // 功能码
            var_uint8_t     slave_addr;     // 从机地址
            var_uint16_t    ext_addr;       // 寄存器地址
            var_uint16_t    ext_ofs;        // 偏移
            var_uint8_t     ext_size;       // 寄存器数目
        } obmodbus;
        struct {
            var_uint32_t    sid;            //
            var_uint32_t    type;           //
            dh_key_t        key;            //
            var_uint16_t    addr;           // 映射地址
        } dh;
        // ...
    } param;
} sync_node_t;

static sync_slave_t     *s_psync_slave = VAR_NULL;

static ExtDataList_t    s_xExtDataList;

static pthread_mutex_t s_varmanage_mutex;

// storage_cfg
rt_bool_t board_cfg_varext_loadlist( ExtDataList_t *pList );
rt_bool_t board_cfg_varext_del_all(void);
rt_bool_t board_cfg_varext_del(const char *name);
rt_bool_t board_cfg_varext_update( const char *name, ExtData_t *data);
rt_bool_t board_cfg_varext_add(ExtData_t *data);

var_uint16_t var_htons(var_uint16_t n)
{
    return ((n & 0xff) << 8) | ((n & 0xff00) >> 8);
}

var_uint32_t var_htonl(var_uint32_t n)
{
    return ((n & 0xff) << 24) |
           ((n & 0xff00) << 8) |
           ((n & 0xff0000UL) >> 8) |
           ((n & 0xff000000UL) >> 24);
}

static void __RefreshRegsFlag(void);
static void __sort_extdata_link(void);
static var_bool_t __creat_sync_slave_link(void);

static void prvSetExtDataCfgDefault(void)
{
    s_xExtDataList.n = 0;
    s_xExtDataList.pList = VAR_NULL;
}

void vVarManage_ExtDataInit(void)
{
    s_xExtDataList.n = 0;
    s_xExtDataList.pList = VAR_NULL;

    for(int i = 0; i < BOARD_UART_MAX; i++) {
        s_com_last_tick[i] = (rt_tick_t)-1;
    }
    
    for(int i = 0; i < BOARD_TCPIP_MAX; i++) {
        s_net_last_tick[i] = (rt_tick_t)-1;
    }

    if( !board_cfg_varext_loadlist(&s_xExtDataList) ) {
        prvSetExtDataCfgDefault();
    }

    __sort_extdata_link();
    __RefreshRegsFlag();
    __creat_sync_slave_link();
    
    if (rt_mutex_init(&s_varmanage_mutex, "varmtx", RT_IPC_FLAG_FIFO) != RT_EOK) {
        rt_kprintf("init varmanage mutex failed\n");
    }
}

// add by jay 2016/12/27   考虑第三方协议节点

static var_bool_t __proto_is_rtu_self(int dev_type, int dev_num)
{
    return (PROTO_DEV_RTU_SELF == dev_type && dev_num < 8);
}

static var_bool_t __proto_is_rtu_self_mid(int dev_type)
{
    return (PROTO_DEV_RTU_SELF_MID == dev_type);
}

static var_bool_t __proto_is_modbus(int dev_type, int proto_type)
{
    if (PROTO_DEV_IS_RS(dev_type)) {
        return (proto_type <= PROTO_MODBUS_ASCII);
    } else if (PROTO_DEV_IS_NET(dev_type) || PROTO_DEV_IS_GPRS(dev_type)) {
        return (proto_type <= PROTO_MODBUS_TCP || proto_type == PROTO_MODBUS_RTU_OVER_TCP);
    } else if (PROTO_DEV_IS_ZIGBEE(dev_type)) {
        return (proto_type <= PROTO_MODBUS_ASCII);
    } else if (PROTO_DEV_IS_LORA(dev_type)) {
        return (proto_type <= PROTO_MODBUS_ASCII);
    }

    return VAR_FALSE;
}

static var_bool_t __proto_is_obmodbus(int dev_type, int proto_type)
{
    if (PROTO_DEV_IS_RS(dev_type)) {
        return (proto_type == PROTO_OBMODBUS_RTU);
    }

    return VAR_FALSE;
}

static var_bool_t __proto_is_dlt645_2007(int dev_type, int proto_type)
{
    return (PROTO_DEV_IS_RS(dev_type) && proto_type == PROTO_DLT645);
}

static var_bool_t __proto_is_dlt645_1997(int dev_type, int proto_type)
{
    return (dev_type <= PROTO_DEV_RS_MAX && proto_type == PROTO_DLT645_1997);
}

static var_bool_t __proto_is_dlt645(int dev_type, int proto_type)
{
    return __proto_is_dlt645_2007(dev_type, proto_type) || __proto_is_dlt645_1997(dev_type, proto_type);
}

static var_bool_t __proto_is_mbus603(int dev_type, int proto_type)
{
    return (dev_type <= PROTO_DEV_RS_MAX && proto_type == PROTO_MBUS603);
}

static var_bool_t __proto_is_dust(int dev_type, int proto_type)
{
    return (dev_type <= PROTO_DEV_RS_MAX && proto_type == PROTO_DUST);
}

static var_bool_t __proto_is_smf(int dev_type, int proto_type)
{
    return (PROTO_DEV_IS_NET(dev_type) || PROTO_DEV_IS_GPRS(dev_type)) && (proto_type == PROTO_SMF);
}

static var_bool_t __proto_is_dh(int dev_type, int proto_type)
{
    return (PROTO_DEV_IS_NET(dev_type) || PROTO_DEV_IS_GPRS(dev_type)) && (proto_type == PROTO_DH);
}

// add by jay 2016/11/29   暂时未考虑第三方自定义协议节点
static void __free_sync_slave_link(void)
{
    while (s_psync_slave) {
        sync_slave_t *next_slave = s_psync_slave->next;
        sync_ext_t *list_ext = s_psync_slave->ext_list;
        while (list_ext) {
            sync_ext_t *next_ext = list_ext->next;
            VAR_MANAGE_FREE(list_ext);
            list_ext = next_ext;
        }
        VAR_MANAGE_FREE(s_psync_slave);
        s_psync_slave = next_slave;
    }
}

static void __release_node_io(ExtData_t *node)
{
    // io
    VAR_MANAGE_FREE(node->xIo.szProtoName);
    if (node->xIo.exp_type == IO_EXP_TYPE_EXP) {
        VAR_MANAGE_FREE(node->xIo.exp.szExp);
    } else if (node->xIo.exp_type == IO_EXP_TYPE_RULE) {
        VAR_MANAGE_FREE(node->xIo.exp.rule.name);
        VAR_MANAGE_FREE(node->xIo.exp.rule.p_in);
        VAR_MANAGE_FREE(node->xIo.exp.rule.p_out);
    }
}

static void __release_node_up(ExtData_t *node)
{
    // up
    VAR_MANAGE_FREE(node->xUp.szNid);
    VAR_MANAGE_FREE(node->xUp.szFid);
    VAR_MANAGE_FREE(node->xUp.szUnit);
    VAR_MANAGE_FREE(node->xUp.szProtoName);
    VAR_MANAGE_FREE(node->xUp.szDesc);
}

static void __release_node(ExtData_t *node)
{
    __release_node_io(node);
    __release_node_up(node);
}

void varmanage_free_all(void)
{
    rt_enter_critical();
    ExtData_t *node = s_xExtDataList.pList;
    while (node) {
        ExtData_t *next = node->next;
        if (node) {
            __release_node(node);

            //node
            VAR_MANAGE_FREE(node);
        }
        node = next;
    }
    s_xExtDataList.n = 0;
    s_xExtDataList.pList = VAR_NULL;
    rt_exit_critical();
    __free_sync_slave_link();
}

static sync_slave_t *__add_sync_slave_node(ExtData_t *data)
{
    sync_slave_t *node = VAR_MANAGE_CALLOC(1, sizeof(sync_slave_t));
    if (node) {
        sync_slave_t *last = s_psync_slave;
        while (last && last->next) last = last->next;
        if (last) {
            last->next = node;
        } else {
            s_psync_slave = node;
        }
        node->dev_type = data->xIo.usDevType;
        node->dev_num = data->xIo.usDevNum;
        node->proto_type = data->xIo.usProtoType;
        node->ext_list = VAR_MANAGE_CALLOC(1, sizeof(sync_ext_t));
        if (__proto_is_rtu_self(node->dev_type, node->dev_num)) {
            if (node->ext_list) {
                node->ext_list->param.self.addr = data->xIo.usAddr;
            }
        } else if (__proto_is_rtu_self_mid(node->dev_type)) {
            if (node->ext_list) {
                node->ext_list->param.self.addr = data->xIo.usAddr;
            }
        } else if (__proto_is_modbus(node->dev_type, node->proto_type)) {
            node->param.modbus.op = data->xIo.param.modbus.btOpCode;
            node->param.modbus.slave_addr = data->xIo.param.modbus.btSlaveAddr;
            node->param.modbus.fa_addr = data->xIo.param.modbus.nSyncFAddr;
            if (node->ext_list) {
                node->ext_list->param.modbus.addr = data->xIo.param.modbus.usExtAddr;
                node->ext_list->param.modbus.ofs = data->xIo.param.modbus.usAddrOfs;
                node->ext_list->param.modbus.size = (data->xIo.btInVarSize+1)/2;
            }
        } else if (__proto_is_dlt645(node->dev_type, node->proto_type)) {
            node->param.dlt645.addr = data->xIo.param.dlt645.addr;
            if (node->ext_list) {
                node->ext_list->param.dlt645.addr = data->xIo.usAddr;
                node->ext_list->param.dlt645.op = data->xIo.param.dlt645.op;
            }
        } else if (__proto_is_mbus603(node->dev_type, node->proto_type)) {
            node->param.mbus603.addr = data->xIo.param.mbus603.addr;
            if (node->ext_list) {
                node->ext_list->param.mbus603.addr = data->xIo.usAddr;
                node->ext_list->param.mbus603.op = data->xIo.param.mbus603.op;
            }
        } else if (__proto_is_dust(node->dev_type, node->proto_type)) {
            if (node->ext_list) {
                node->ext_list->param.dust.addr = data->xIo.usAddr;
                node->ext_list->param.dust.op = data->xIo.param.dust.op;
            }
        } else if (__proto_is_smf(node->dev_type, node->proto_type)) {
            if (node->ext_list) {
                node->ext_list->param.smf.addr = data->xIo.usAddr;
                node->ext_list->param.smf.op = data->xIo.param.smf.op;
            }
        } else if (__proto_is_obmodbus(node->dev_type, node->proto_type)) {
            node->param.obmodbus.op = data->xIo.param.modbus.btOpCode;
            node->param.obmodbus.slave_addr = data->xIo.param.modbus.btSlaveAddr;
            if (node->ext_list) {
                node->ext_list->param.obmodbus.addr = data->xIo.param.modbus.usExtAddr;
                node->ext_list->param.obmodbus.ofs = data->xIo.param.modbus.usAddrOfs;
                node->ext_list->param.obmodbus.size = data->xIo.btInVarSize;
            }
        } else if (__proto_is_dh(node->dev_type, node->proto_type)) {
            node->param.dh.sid = data->xIo.param.dh.sid;
            node->param.dh.type = data->xIo.param.dh.type;
            if (node->ext_list) {
                node->ext_list->param.dh.addr = data->xIo.usAddr;
                node->ext_list->param.dh.key = data->xIo.param.dh.key;
            }
        }
        return node;
    }

    return VAR_NULL;
}

static sync_slave_t *__find_sync_slave_node(ExtData_t *data)
{
    sync_slave_t *node = s_psync_slave;
    if (__proto_is_rtu_self(data->xIo.usDevType, data->xIo.usDevNum)) {
        while (node) {
            if (node->dev_type == data->xIo.usDevType && 
                node->dev_num == data->xIo.usDevNum) 
            {
                return node;
            }
            node = node->next;
        }
    } else if (__proto_is_rtu_self_mid(data->xIo.usDevType)) {
        while (node) {
            if (node->dev_type == data->xIo.usDevType) {
                return node;
            }
            node = node->next;
        }
    } else if (__proto_is_modbus(data->xIo.usDevType, data->xIo.usProtoType)) {
        while (node) {
            if (node->param.modbus.op == data->xIo.param.modbus.btOpCode && 
                node->param.modbus.fa_addr == data->xIo.param.modbus.nSyncFAddr && 
                node->param.modbus.slave_addr == data->xIo.param.modbus.btSlaveAddr &&
                node->dev_type == data->xIo.usDevType && 
                node->dev_num == data->xIo.usDevNum && 
                node->proto_type == data->xIo.usProtoType) 
            {
                return node;
            }
            node = node->next;
        }
    } else if (__proto_is_dlt645(data->xIo.usDevType, data->xIo.usProtoType)) {
        while (node) {
            if ( 0 == memcmp(&node->param.dlt645.addr, &data->xIo.param.dlt645.addr, sizeof(dlt645_addr_t)) && 
                node->dev_type == data->xIo.usDevType && 
                node->dev_num == data->xIo.usDevNum && 
                node->proto_type == data->xIo.usProtoType) 
            {
                return node;
            }
            node = node->next;
        }
    } else if (__proto_is_mbus603(data->xIo.usDevType, data->xIo.usProtoType)) {
        while (node) {
            if ( 0 == memcmp(&node->param.mbus603.addr, &data->xIo.param.mbus603.addr, sizeof(mbus603_addr_t)) && 
                node->dev_type == data->xIo.usDevType && 
                node->dev_num == data->xIo.usDevNum && 
                node->proto_type == data->xIo.usProtoType) 
            {
                return node;
            }
            node = node->next;
        }
    } else if (__proto_is_dust(data->xIo.usDevType, data->xIo.usProtoType) || __proto_is_smf(data->xIo.usDevType, data->xIo.usProtoType)) {
        while (node) {
            if (node->dev_type == data->xIo.usDevType && 
                node->dev_num == data->xIo.usDevNum && 
                node->proto_type == data->xIo.usProtoType) 
            {
                return node;
            }
            node = node->next;
        }
    } else if (__proto_is_obmodbus(data->xIo.usDevType, data->xIo.usProtoType)) {
        while (node) {
            if (node->param.obmodbus.op == data->xIo.param.modbus.btOpCode && 
                node->param.obmodbus.slave_addr == data->xIo.param.modbus.btSlaveAddr &&
                node->dev_type == data->xIo.usDevType && 
                node->dev_num == data->xIo.usDevNum && 
                node->proto_type == data->xIo.usProtoType) 
            {
                return node;
            }
            node = node->next;
        }
    } else if (__proto_is_dh(data->xIo.usDevType, data->xIo.usProtoType)) {
        while (node) {
            if (node->param.dh.sid == data->xIo.param.dh.sid &&
                node->param.dh.type == data->xIo.param.dh.type && 
                node->dev_type == data->xIo.usDevType && 
                node->dev_num == data->xIo.usDevNum && 
                node->proto_type == data->xIo.usProtoType) 
            {
                return node;
            }
            node = node->next;
        }
    }
    return VAR_NULL;
}

// 按addr排序
static sync_ext_t *__add_sync_ext_node(ExtData_t *data)
{
    sync_slave_t *node_slave = __find_sync_slave_node(data);
    if (node_slave) {
        sync_ext_t *node_ext = node_slave->ext_list;
        sync_ext_t *node_new = VAR_MANAGE_CALLOC(1, sizeof(sync_ext_t));
        if (node_new) {
            if (__proto_is_rtu_self( node_slave->dev_type, node_slave->dev_num)) {
                node_new->param.self.addr = data->xIo.usAddr;
            } else if (__proto_is_rtu_self_mid( node_slave->dev_type)) {
                node_new->param.self.addr = data->xIo.usAddr;
            } else if (__proto_is_modbus( node_slave->dev_type, node_slave->proto_type)) {
                node_new->param.modbus.addr = data->xIo.param.modbus.usExtAddr;
                node_new->param.modbus.ofs = data->xIo.param.modbus.usAddrOfs;
                node_new->param.modbus.size = (data->xIo.btInVarSize+1)/2;
                while (node_ext) {
                    // 插入链表, 直接返回
                    if (node_new->param.modbus.addr < node_ext->param.modbus.addr) {
                        sync_ext_t *prev = node_ext->prev;
                        if (prev) {
                            node_new->prev = prev;
                            prev->next = node_new;
                        } else {    // 头结点后移
                            node_slave->ext_list = node_new;
                        }
                        node_ext->prev = node_new;
                        node_new->next = node_ext;
                        return node_new;
                    } else if (
                        node_new->param.modbus.addr == node_ext->param.modbus.addr && 
                        node_new->param.modbus.ofs == node_ext->param.modbus.ofs && 
                        node_new->param.modbus.size == node_ext->param.modbus.size) {
                        VAR_MANAGE_FREE(node_new);
                        return node_ext;
                    }
                    node_ext = node_ext->next;
                }
            } else if (__proto_is_dlt645( node_slave->dev_type, node_slave->proto_type)) {
                node_new->param.dlt645.addr = data->xIo.usAddr;
                node_new->param.dlt645.op = data->xIo.param.dlt645.op;
            } else if (__proto_is_mbus603( node_slave->dev_type, node_slave->proto_type)) {
                node_new->param.mbus603.addr = data->xIo.usAddr;
                node_new->param.mbus603.op = data->xIo.param.mbus603.op;
            } else if (__proto_is_dust( node_slave->dev_type, node_slave->proto_type)) {
                node_new->param.dust.addr = data->xIo.usAddr;
                node_new->param.dust.op = data->xIo.param.dust.op;
            } else if (__proto_is_smf( node_slave->dev_type, node_slave->proto_type)) {
                node_new->param.smf.addr = data->xIo.usAddr;
                node_new->param.smf.op = data->xIo.param.smf.op;
            } else if (__proto_is_obmodbus( node_slave->dev_type, node_slave->proto_type)) {
                node_new->param.obmodbus.addr = data->xIo.param.modbus.usExtAddr;
                node_new->param.obmodbus.ofs = data->xIo.param.modbus.usAddrOfs;
                node_new->param.obmodbus.size = data->xIo.btInVarSize;
            } else if (__proto_is_dh( node_slave->dev_type, node_slave->proto_type)) {
                VAR_MANAGE_FREE(node_new);
                return node_ext;
            }
            // 连接到最后
            node_ext = node_slave->ext_list;
            while (node_ext && node_ext->next) node_ext = node_ext->next;
            node_ext->next = node_new;
            node_new->prev = node_ext;
            return node_new;
        }
    } else {
        node_slave = __add_sync_slave_node(data);
        return node_slave->ext_list;
    }

    return VAR_NULL;
}

// 连续地址节点合并
// change by jay 2016/12/26  目前仅有modbus需要合并地址
static void __trim_sync_slave_link(void)
{
    sync_slave_t *node_slave = s_psync_slave;
    while (node_slave) {
        sync_ext_t *node_ext = node_slave->ext_list;
        if (__proto_is_modbus( node_slave->dev_type, node_slave->proto_type)) {
            while (node_ext) {
                sync_ext_t *next = node_ext->next;
                if (next) {
                    // 可串接
                    // add by jay 2014/04/24 相等才串接(可以处理同一寄存器地址不会采集的问题)
                    if (node_ext->param.modbus.addr + node_ext->param.modbus.size >= next->param.modbus.addr ) {
                        var_uint16_t addr_a = node_ext->param.modbus.addr + node_ext->param.modbus.size;
                        var_uint16_t addr_b = next->param.modbus.addr + next->param.modbus.size;
                        if (addr_a < addr_b) {    //new size
                            // cmp size
                            if ((addr_b-node_ext->param.modbus.addr)<=_SYNC_EXT_MAX_SIZE) {
                                node_ext->param.modbus.size = addr_b-node_ext->param.modbus.addr;
                            } else {
                                node_ext = node_ext->next;
                                continue;
                            }
                        }
                        // del next
                        node_ext->next = next->next;
                        if (next->next) next->next->prev = node_ext;
                        VAR_MANAGE_FREE(next);
                    } else {
                        node_ext = node_ext->next;
                    }
                } else {
                    break;
                }
            }
        }
        node_slave = node_slave->next;
    }
}

static var_bool_t __isexist_sync_ext(sync_slave_t *slave, sync_ext_t *ext)
{
    sync_slave_t *node_slave = s_psync_slave;
    while (node_slave) {
        if (slave==node_slave) {
            sync_ext_t *node_ext = node_slave->ext_list;
            while (node_ext) {
                if (ext==node_ext) {
                    return VAR_TRUE;
                }
                node_ext = node_ext->next;
            }
            return VAR_FALSE;
        }
        node_slave = node_slave->next;
    }
    return VAR_FALSE;
}

// 查找下一个节点, 临界区, 防止链表结构中途变化导致异常
// 通过node返回, 使之线程访问安全
static var_bool_t __get_next_sync_ext_node(sync_slave_t **slave, sync_ext_t **ext, sync_node_t *node)
{
    var_bool_t ret = VAR_FALSE;
    
    rt_enter_critical();
    {
        sync_slave_t *node_slave = *slave;
        sync_ext_t *node_ext = *ext;
        
        if (node_slave && node_ext && __isexist_sync_ext(node_slave, node_ext)) {
            if (node_ext->next) {
                node_ext = node_ext->next;
            } else {
                node_slave = node_slave->next;
                if (node_slave) node_ext = node_slave->ext_list;
            }
        } else {
            node_ext = VAR_NULL;
            node_slave = s_psync_slave;
            if (node_slave) node_ext = node_slave->ext_list;
        }
        if (node_slave && node_ext) {
            node->dev_type = node_slave->dev_type;
            node->dev_num = node_slave->dev_num;
            node->proto_type = node_slave->proto_type;
            if (__proto_is_rtu_self( node_slave->dev_type, node_slave->dev_num)) {
                node->param.self.addr = node_ext->param.self.addr;
            } else if (__proto_is_rtu_self_mid( node_slave->dev_type)) {
                node->param.self.addr = node_ext->param.self.addr;
            } else if (__proto_is_modbus( node_slave->dev_type, node_slave->proto_type)) {
                node->param.modbus.op = node_slave->param.modbus.op;
                node->param.modbus.slave_addr = node_slave->param.modbus.slave_addr;
                node->param.modbus.ext_addr = node_ext->param.modbus.addr;
                node->param.modbus.ext_ofs = node_ext->param.modbus.ofs;
                node->param.modbus.ext_size = node_ext->param.modbus.size;
            } else if (__proto_is_dlt645( node_slave->dev_type, node_slave->proto_type)) {
                node->param.dlt645.dlt645_addr = node_slave->param.dlt645.addr;
                node->param.dlt645.addr = node_ext->param.dlt645.addr;
                node->param.dlt645.op = node_ext->param.dlt645.op;
            } else if (__proto_is_mbus603( node_slave->dev_type, node_slave->proto_type)) {
                node->param.mbus603.mbus_addr = node_slave->param.mbus603.addr;
                node->param.mbus603.addr = node_ext->param.mbus603.addr;
                node->param.mbus603.op = node_ext->param.mbus603.op;
            } else if (__proto_is_dust( node_slave->dev_type, node_slave->proto_type)) {
                node->param.dust.addr = node_ext->param.dust.addr;
                node->param.dust.op = node_ext->param.dust.op;
            } else if (__proto_is_smf( node_slave->dev_type, node_slave->proto_type)) {
                node->param.smf.addr = node_ext->param.smf.addr;
                node->param.smf.op = node_ext->param.smf.op;
            } else if (__proto_is_obmodbus( node_slave->dev_type, node_slave->proto_type)) {
                node->param.obmodbus.op = node_slave->param.modbus.op;
                node->param.obmodbus.slave_addr = node_slave->param.modbus.slave_addr;
                node->param.obmodbus.ext_addr = node_ext->param.modbus.addr;
                node->param.obmodbus.ext_ofs = node_ext->param.modbus.ofs;
                node->param.obmodbus.ext_size = node_ext->param.modbus.size;
            } else if (__proto_is_dh( node_slave->dev_type, node_slave->proto_type)) {
                node->param.dh.sid = node_slave->param.dh.sid;
                node->param.dh.type = node_slave->param.dh.type;
                node->param.dh.key = node_ext->param.dh.key;
                node->param.dh.addr = node_ext->param.dh.addr;
            }
            ret = VAR_TRUE;
        }
        *slave = node_slave;
        *ext = node_ext;
    }
    rt_exit_critical();

    return ret;
}

static var_bool_t __creat_sync_slave_link(void)
{
    var_bool_t ret = VAR_FALSE;
    
    rt_enter_critical();
    {
        for(int i = 0; i < BOARD_UART_MAX; i++) {
            s_com_last_tick[i] = (rt_tick_t)-1;
        }
        
        for(int i = 0; i < BOARD_TCPIP_MAX; i++) {
            s_net_last_tick[i] = (rt_tick_t)-1;
        }
        
        __free_sync_slave_link();

        if (s_xExtDataList.pList) {
            ExtData_t *node = s_xExtDataList.pList;
            while (node) {
                if (node->bEnable) {
                    __add_sync_ext_node(node);
                }
                node = node->next;
            }
            ret = VAR_TRUE;
        }

        __trim_sync_slave_link();
    }
    rt_exit_critical();

    return ret;
}

static void __sort_extdata_link(void)
{
    rt_enter_critical();
    {
        if (s_xExtDataList.pList) {
            ExtData_t *top = s_xExtDataList.pList;
            ExtData_t *next, *node, *tmp;
            node = top;
            while (node) {
                next = node->next;
                if(next) {
                    for(tmp = top; tmp != next; tmp = tmp->next) {
                        if(tmp->prev) {
                            if(next->xIo.usAddr >= tmp->prev->xIo.usAddr && next->xIo.usAddr <= tmp->xIo.usAddr) {
                                node->next = next->next;
                                if(next->next) next->next->prev = node;
                                next->next = tmp;
                                next->prev = tmp->prev;
                                tmp->prev->next = next;
                                if(tmp->next) tmp->next->prev = next;
                                break;
                            }
                        } else {
                            if(next->xIo.usAddr <= tmp->xIo.usAddr) {
                                node->next = next->next;
                                if(next->next) next->next->prev = node;
                                next->next = tmp;
                                next->prev = tmp->prev;
                                if(tmp->next) tmp->next->prev = next;
                                s_xExtDataList.pList = next;
                                top = s_xExtDataList.pList;
                                break;
                            }
                        }
                    }
                }
                node = node->next;
            }
        }
    }
    rt_exit_critical();
}

static ExtData_t* __GetExtDataWithName(const char *name)
{
    ExtData_t *ret = VAR_NULL;
    if (name && s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (0 == strcmp(name, node->xName.szName)) {
                ret = node;
                break;
            }
            node = node->next;
        }
    }
    return ret;
}

void vVarManage_InsertNode(ExtData_t *data)
{
    rt_enter_critical();
    {
        ExtData_t *last = s_xExtDataList.pList;
        while (last && last->next) last = last->next;
        if (last) {
            last->next = data;
            data->prev = last;
        } else {
            s_xExtDataList.pList = data;
        }
        s_xExtDataList.n++;
    }
    rt_exit_critical();
}

ExtData_t *pVarManage_CopyData( ExtData_t *in )
{
    ExtData_t *out = VAR_NULL;
    rt_enter_critical();
    {
        if( in ) {
            out = VAR_MANAGE_CALLOC(1, sizeof(ExtData_t));
            if (out) {
                memcpy( out, in, sizeof(ExtData_t) );

                // io
                out->xIo.szProtoName = rt_strdup(in->xIo.szProtoName);
            
                if (out->xIo.exp_type == IO_EXP_TYPE_EXP) {
                    out->xIo.exp.szExp = rt_strdup(in->xIo.exp.szExp);
                } else if (out->xIo.exp_type == IO_EXP_TYPE_RULE) {
                    out->xIo.exp.rule.name = rt_strdup(in->xIo.exp.rule.name);
                    out->xIo.exp.rule.p_in = rt_strdup(in->xIo.exp.rule.p_in);
                    out->xIo.exp.rule.p_out = rt_strdup(in->xIo.exp.rule.p_out);
                }

                // up
                out->xUp.szNid = rt_strdup(in->xUp.szNid);
                out->xUp.szFid = rt_strdup(in->xUp.szFid);
                out->xUp.szUnit = rt_strdup(in->xUp.szUnit);
                out->xUp.szProtoName = rt_strdup(in->xUp.szProtoName);
                out->xUp.szDesc = rt_strdup(in->xUp.szDesc);
            }
        }
    }
    rt_exit_critical();

    return out;
}

// add by jay 2016/11/25
// *data must heap point
void vVarManage_FreeData( ExtData_t *data )
{
    if(data) {
        __release_node(data);
        //free node
        VAR_MANAGE_FREE(data);
    }
}

void vVarManage_RemoveNodeWithName(const char *name)
{
    rt_enter_critical();
    {
        ExtData_t *data = __GetExtDataWithName(name);
        if (data) {
            ExtData_t *prev = data->prev;
            ExtData_t *next = data->next;
            if (prev) {
                prev->next = next;
            } else {    // 头结点后移
                s_xExtDataList.pList = next;
            }
            if (next) next->prev = prev;
            s_xExtDataList.n--;

            __release_node(data);
            //free node
            VAR_MANAGE_FREE(data);

            if (0 == s_xExtDataList.n) {
                s_xExtDataList.pList = VAR_NULL;
            }
        }
    }
    rt_exit_critical();
}

// 调用后, 必须进行释放
ExtData_t* __GetExtDataWithIndex(int n)
{
    ExtData_t *ret = VAR_NULL;
    rt_enter_critical();
    {
        if (s_xExtDataList.pList) {
            ExtData_t *node = s_xExtDataList.pList;
            while (node && n--) {
                node = node->next;
            }
            ret = pVarManage_CopyData(node);
            
        }
    }
    rt_exit_critical();
    return ret;
}

static var_bool_t __GetExtValue(var_int8_t type, VarValue_v *var, var_double_t *value)
{
    var_bool_t ret = VAR_TRUE;
    switch (type) {
    case E_VAR_BIT:
        *value = var->val_bit;
        break;
    case E_VAR_INT8:
        *value = var->val_i8;
        break;
    case E_VAR_UINT8:
        *value = var->val_u8;
        break;
    case E_VAR_INT16:
        *value = var->val_i16;
        break;
    case E_VAR_UINT16:
        *value = var->val_u16;
        break;
    case E_VAR_INT32:
        *value = var->val_i32;
        break;
    case E_VAR_UINT32:
        *value = var->val_u32;
        break;
    case E_VAR_FLOAT:
        *value = var->val_f;
        break;
    case E_VAR_DOUBLE:
        *value = var->val_db;
        break;
    default:
        ret = VAR_FALSE;
    }

    return ret;
}

static void __RefreshRegsFlag(void)
{
    rt_enter_critical();
    {
        memset(g_xExtDataRegsFlag, 0, sizeof(g_xExtDataRegsFlag));
        if (s_xExtDataList.pList) {
            ExtData_t *node = s_xExtDataList.pList;
            while (node) {
                if (node->bEnable) {
                    for(int i = 0; i < (node->xIo.btOutVarSize+1)/2; i++) {
                        int addr = node->xIo.usAddr+i;
                        if( EXT_DATA_REG_CHECK(addr) ) {
                            g_xExtDataRegsFlag[addr-USER_REG_EXT_DATA_START] = VAR_TRUE;
                        }
                    }
                }
                node = node->next;
            }
        }
    }
    rt_exit_critical();
}

var_bool_t bVarManage_CheckExtValue(ExtData_t *data, VarValue_v *var)
{
    var_double_t value = 0;
    if (__GetExtValue(data->xIo.btOutVarType, var, &value)) {
        if (data->xIo.bUseMax && value > data->xIo.fMax) {
            return VAR_FALSE;
        }
        if (data->xIo.bUseMin && value < data->xIo.fMin) {
            return VAR_FALSE;
        }
    }

    return VAR_TRUE;
}

var_bool_t bVarManage_GetExtValue(ExtData_t *data, var_int8_t type, var_double_t *value)
{
    var_bool_t ret = VAR_TRUE;
    if (data) {
        return __GetExtValue(type, &data->xIo.xValue, value);
    }

    return ret;
}

void bVarManage_UpdateAvgValue(VarAvg_t *varavg, var_double_t value)
{
    if(varavg) {
        varavg->val_avg += value;
        varavg->val_cur = value;
        varavg->count++;
        // 最大最小值, 初值
        if(1 == varavg->count) {
            varavg->val_min = value;
            varavg->val_max = value;
        } else {
            // 更新最大最小值
            if(value < varavg->val_min) {
                varavg->val_min = value;
            }
            if(value > varavg->val_max) {
                varavg->val_max = value;
            }
        }
    }
}

var_bool_t bVarManage_GetExtValueWithName(const char *name, var_double_t *value)
{
    var_bool_t ret = VAR_TRUE;
    rt_enter_critical();
    {
        ExtData_t *data = __GetExtDataWithName(name);
        ret = bVarManage_GetExtValue(data, data ? data->xIo.btOutVarType : 0, value);
    }
    rt_exit_critical();
    return ret;
}

static void __update_value(ExtData_t *data)
{
    var_double_t value = 0;
    if (data && bVarManage_GetExtValue(data, data->xIo.btOutVarType, &value)) {
        data->xStorage.xAvgReal.val_cur = value;
        data->xStorage.xAvgMin.val_cur = value;
        data->xStorage.xAvgHour.val_cur = value;
        //data->xUp.xAvgUp.val_cur = value;
        data->xStorage.xAvgReal.val_avg += value;
        data->xStorage.xAvgMin.val_avg += value;
        data->xStorage.xAvgHour.val_avg += value;
        //data->xUp.xAvgUp.val_avg += value;
        data->xStorage.xAvgReal.count++;
        data->xStorage.xAvgMin.count++;
        data->xStorage.xAvgHour.count++;
        //data->xUp.xAvgUp.count++;
    }
}

static var_bool_t __doexp_update_val(ExtData_t *data, double exp_val)
{
    switch (data->xIo.btOutVarType) {
    case E_VAR_BIT:
        data->xIo.xValue.val_bit = ((exp_val!=0)?1:0);
        break;
    case E_VAR_INT8:
        data->xIo.xValue.val_i8 = (var_int8_t)exp_val;
        break;
    case E_VAR_UINT8:
        data->xIo.xValue.val_u8 = (var_uint8_t)exp_val;
        break;
    case E_VAR_INT16:
        data->xIo.xValue.val_i16 = (var_int16_t)exp_val;
        break;
    case E_VAR_UINT16:
        data->xIo.xValue.val_u16 = (var_uint16_t)exp_val;
        break;
    case E_VAR_INT32:
        data->xIo.xValue.val_i32 = (var_int32_t)exp_val;
        break;
    case E_VAR_UINT32:
        data->xIo.xValue.val_u32 = (var_uint32_t)exp_val;
        break;
    case E_VAR_FLOAT:
        data->xIo.xValue.val_f = (var_float_t)exp_val;
        break;
    case E_VAR_DOUBLE:
        data->xIo.xValue.val_db = (var_double_t)exp_val;
        break;
    default:
        return VAR_FALSE;
    }

    return VAR_TRUE;
}

static var_bool_t __doexp_exp(ExtData_t *data)
{
    var_bool_t ret = VAR_FALSE;
    double value = 0;
    if (data && data->xIo.exp.szExp && data->xIo.exp.szExp[0] && bVarManage_GetExtValue(data, data->xIo.btOutVarType, &value)) {
        int is_error = 1;
        double exp_val = evaluate(data->xIo.exp.szExp, data->xName.szName, &is_error);
        if (is_error) {
            rt_kprintf("['%s']->error exp:%s\n", data->xName.szName, data->xIo.exp.szExp );
        } else {
            ret = (exp_val!=value);
            if (ret) {
                ret = __doexp_update_val(data, exp_val);
            }
        }
    }
    return ret;
}

static var_bool_t __doexp_rule(ExtData_t *data)
{
    if (data->xIo.exp.rule.name && data->xIo.exp.rule.name[0]) {
        char exp[2048];
        var_bool_t ret = VAR_FALSE;
        struct rule_node *rule = rule_get(data->xIo.exp.rule.name);
        if (!rule) return RT_FALSE;

        if (rule->type == RULE_TYPE_NONE) {
            sprintf(exp, "%s(%s)", data->xIo.exp.rule.name, data->xIo.exp.rule.p_in);
        } else if (rule->type == RULE_TYPE_CTRL) {
            sprintf(exp, "if (%s(%s)) { return (%s_o(%s)); } else { return 0; }", 
                data->xIo.exp.rule.name, data->xIo.exp.rule.p_in ? data->xIo.exp.rule.p_in : "", 
                data->xIo.exp.rule.name, data->xIo.exp.rule.p_out ? data->xIo.exp.rule.p_out : "");
        }

        {
            double value = 0;
            if (data && bVarManage_GetExtValue(data, data->xIo.btOutVarType, &value)) {
                int is_error = 1;
                double exp_val = evaluate(exp, data->xName.szName, &is_error);
                if (is_error) {
                    rt_kprintf("['%s']->error rule call:%s\n", data->xName.szName, exp);
                } else {
                    ret = (exp_val!=value);
                    if (ret) {
                        ret = __doexp_update_val(data, exp_val);
                    }
                }
            }

            return ret;
        }
    }
    return RT_FALSE;
}

static var_bool_t __doexp_value(ExtData_t *data)
{
    if (data->xIo.exp_type == IO_EXP_TYPE_EXP) {
        return __doexp_exp(data);
    } else if (data->xIo.exp_type == IO_EXP_TYPE_RULE) {
        return __doexp_rule(data);
    }
}

var_bool_t bVarManage_CheckContAddr(var_uint16_t usAddr, var_uint16_t usNRegs)
{
    var_bool_t ret = VAR_TRUE;
    rt_enter_critical();
    while(usNRegs) {
        if( !EXT_DATA_REG_CHECK(usAddr) || !g_xExtDataRegsFlag[usAddr-USER_REG_EXT_DATA_START] ) {
            ret = VAR_FALSE;
            break;
        }
        usNRegs--;
        usAddr++;
    }
    rt_exit_critical();
    return ret;
}

static var_int_t __GetExtDataIoInfoWithAddr(
    var_uint16_t    usAddress, 
    var_uint16_t    *usDevType, 
    var_uint16_t    *usDevNum, 
    var_uint8_t     *btSlaveAddr, 
    var_uint16_t    *usExtAddr, 
    var_uint8_t     *btOpCode
)
{
    var_int_t nregs = 0;
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (node->bEnable &&
                usAddress >= node->xIo.usAddr &&
                usAddress < (node->xIo.btOutVarSize+1)/2 + node->xIo.usAddr) {
                *usDevType = node->xIo.usDevType;
                *usDevNum = node->xIo.usDevNum;
                *btSlaveAddr = node->xIo.param.modbus.btSlaveAddr;
                *usExtAddr = node->xIo.param.modbus.usExtAddr+(usAddress-node->xIo.usAddr);
                *btOpCode = node->xIo.param.modbus.btOpCode;
                nregs = (node->xIo.btOutVarSize+1)/2-(usAddress-node->xIo.usAddr);
                break;
            }
            node = node->next;
        }
    }
    rt_exit_critical();
    return nregs;
}

static void __do_set_out_value(ExtData_t *data, var_int8_t type)
{
    double value = 0;
    if (data && type != data->xIo.btOutVarType && bVarManage_GetExtValue(data, type, &value)) {
        switch (data->xIo.btOutVarType) {
        case E_VAR_BIT:
            data->xIo.xValue.val_bit = ((value!=0)?1:0);
            break;
        case E_VAR_INT8:
            data->xIo.xValue.val_i8 = (var_int8_t)value;
            break;
        case E_VAR_UINT8:
            data->xIo.xValue.val_u8 = (var_uint8_t)value;
            break;
        case E_VAR_INT16:
            data->xIo.xValue.val_i16 = (var_int16_t)value;
            break;
        case E_VAR_UINT16:
            data->xIo.xValue.val_u16 = (var_uint16_t)value;
            break;
        case E_VAR_INT32:
            data->xIo.xValue.val_i32 = (var_int32_t)value;
            break;
        case E_VAR_UINT32:
            data->xIo.xValue.val_u32 = (var_uint32_t)value;
            break;
        case E_VAR_FLOAT:
            data->xIo.xValue.val_f = (var_float_t)value;
            break;
        case E_VAR_DOUBLE:
            data->xIo.xValue.val_db = (var_double_t)value;
            break;
        default:
            ;
        }
    }
}

static var_bool_t __calc_adc_value(ExtData_t *node, double *result)
{
    eRangeType_t hw_range = Range_0_20MA;
    AdcValue_t adc = {0, 0, 0, 0, 0};
    double adc_value = 0;
    ExtData_AI_cfg_t *cfg = &node->xCfg.ai;
    if (bVarManage_GetExtValue(node, node->xIo.btOutVarType, &adc_value)) {
        switch (cfg->range) {
        case Range_0_20MA: {
            if (Range_0_20MA == hw_range) {
            }
            break;
        }
        case Range_4_20MA: {
            if (Range_0_20MA == hw_range) {
                if (adc_value < 0x7FFFFF / 10.0 * 4.0) {
                    adc_value = 0;
                }
            }
            break;
        }
        case Range_1_5V: {
            break;
        }
        case Range_0_5V: {
            break;
        }
        default: break;
        }

        adc.eng_unit = (var_uint32_t)adc_value;
        adc.binary_unit = adc.eng_unit;
        
        switch (cfg->range) {
        case Range_0_20MA:
        case Range_4_20MA: {
            adc.measure = (var_float_t)(adc_value * 20.0 / 0xFFFFFF);
            if (cfg->range == Range_4_20MA) {
                if (adc.measure >= 4.0) {
                    adc.percent_unit = (adc.measure - 4) / (20 - 4) * 100.0f;
                    adc.meter_unit = \
                            (cfg->ext_range_max - cfg->ext_range_min) * (adc.measure - 4) / (20 - 4) + \
                            cfg->ext_range_min + cfg->ext_factor;
                } else {
                    adc.percent_unit = 0;
                    adc.meter_unit = cfg->ext_range_min + cfg->ext_factor;
                }
                if (adc.measure < 4.0) adc.measure = 4.0;
            } else if (cfg->range == Range_0_20MA) {
                adc.percent_unit = (float)(adc.measure / 20.0f * 100.0f);
                adc.meter_unit = (cfg->ext_range_max - cfg->ext_range_min) * adc.measure / 20.0f + cfg->ext_range_min + cfg->ext_factor;
            }
        break;
        }
        case Range_0_5V:
            break;
        case Range_1_5V:
            break;
        default: break;
        }

        switch (cfg->unit) {
        case Unit_Eng:
            *result = adc.eng_unit;
            break;
        case Unit_Binary:
            *result = adc.binary_unit;
            break;
        case Unit_Percent:
            *result = adc.percent_unit;
            break;
        case Unit_Meter:
            *result = adc.meter_unit;
            break;
        default: break;
        }

        node->xAttach.ai = adc;

        return VAR_TRUE;
    }

    return VAR_FALSE;
}

static void __try_refresh_ai_value(ExtData_t *node)
{
    if (node && node->eAttr == E_VAR_ATTR_AI) {
        double ai_result = 0;
        if (__calc_adc_value(node, &ai_result)) {
            switch (node->xIo.btOutVarType) {
            case E_VAR_BIT:
                node->xIo.xValue.val_bit = ((ai_result!=0)?1:0);
                break;
            case E_VAR_INT8:
                node->xIo.xValue.val_i8 = (var_int8_t)ai_result;
                break;
            case E_VAR_UINT8:
                node->xIo.xValue.val_u8 = (var_uint8_t)ai_result;
                break;
            case E_VAR_INT16:
                node->xIo.xValue.val_i16 = (var_int16_t)ai_result;
                break;
            case E_VAR_UINT16:
                node->xIo.xValue.val_u16 = (var_uint16_t)ai_result;
                break;
            case E_VAR_INT32:
                node->xIo.xValue.val_i32 = (var_int32_t)ai_result;
                break;
            case E_VAR_UINT32:
                node->xIo.xValue.val_u32 = (var_uint32_t)ai_result;
                break;
            case E_VAR_FLOAT:
                node->xIo.xValue.val_f = (var_float_t)ai_result;
                break;
            case E_VAR_DOUBLE:
                node->xIo.xValue.val_db = (var_double_t)ai_result;
                break;
            default:
                ;
            }
        }
    }
}

extern void vLittleEdianExtData(var_uint8_t btType, var_uint8_t btRule, VarValue_v *in, VarValue_v *out);
extern void vBigEdianExtData(var_uint8_t btType, var_uint8_t btRule, VarValue_v *in, VarValue_v *out);
static void __SlaveRefreshExtDataWithRegs(var_uint16_t usAddress)
{
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (node->bEnable &&
                usAddress >= node->xIo.usAddr &&
                usAddress < (node->xIo.btOutVarSize+1)/2 + node->xIo.usAddr) {
                VarValue_v xValue, xValue_bak;
                vLittleEdianExtData(node->xIo.btOutVarType, node->xIo.btOutVarRuleType, (VarValue_v *)&g_xExtDataRegs[usAddress-USER_REG_EXT_DATA_START], &xValue );
                xValue_bak = node->xIo.xValue;
                node->xIo.xValue = xValue;
                __try_refresh_ai_value(node);
                if (__doexp_value(node)) {
                    if (!bVarManage_CheckExtValue(node, &xValue) ) {
                        node->xIo.xValue = xValue_bak;
                    }
                }
                vBigEdianExtData(node->xIo.btOutVarType, node->xIo.btOutVarRuleType, &node->xIo.xValue, (VarValue_v *)&g_xExtDataRegs[usAddress-USER_REG_EXT_DATA_START]);
                __update_value(node);
                node->xIo.btErrNum = 0;
                break;
            }
            node = node->next;
        }
    }
    rt_exit_critical();
}

static void __MasterRefreshExtDataWithRegs(var_uint16_t usAddress)
{
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (node->bEnable &&
                usAddress >= node->xIo.usAddr &&
                usAddress < (node->xIo.btOutVarSize+1)/2 + node->xIo.usAddr) {
                VarValue_v xValue, xValue_bak;
                vLittleEdianExtData( node->xIo.btInVarType, node->xIo.btInVarRuleType, (VarValue_v *)&g_xExtDataRegs[usAddress-USER_REG_EXT_DATA_START], &xValue );
                xValue_bak = node->xIo.xValue;
                node->xIo.xValue = xValue;
                node->xIo.bErrFlag = RT_FALSE;
                __do_set_out_value(node, node->xIo.btInVarType);
                __try_refresh_ai_value(node);
                if (__doexp_value(node)) {
                    if (!bVarManage_CheckExtValue(node, &xValue) ) {
                        node->xIo.xValue = xValue_bak;
                    }
                }
                vBigEdianExtData(node->xIo.btOutVarType, node->xIo.btOutVarRuleType, &node->xIo.xValue, (VarValue_v *)&g_xExtDataRegs[usAddress-USER_REG_EXT_DATA_START]);
                __update_value(node);
                node->xIo.btErrNum = 0;
                break;
            }
            node = node->next;
        }
    }
    rt_exit_critical();
}

var_bool_t bVarManage_RefreshExtDataWithModbusSlave(var_uint16_t usAddress, var_uint16_t usNRegs)
{
    var_bool_t      ret = VAR_FALSE;
    var_uint16_t    usDevType;
    var_uint16_t    usDevNum;
    var_uint8_t     btSlaveAddr;
    var_uint16_t    usExtAddr;
    var_uint8_t     btOpCode;
    while( usNRegs ) {
        var_int_t nregs = __GetExtDataIoInfoWithAddr(usAddress, &usDevType, &usDevNum, &btSlaveAddr, &usExtAddr, &btOpCode);
        if( nregs > 0 ) {
            var_int8_t _p = -1;
            var_uint16_t *regs = &g_xExtDataRegs[usAddress-USER_REG_EXT_DATA_START];
            if (PROTO_DEV_IS_RS(usDevType)) {
                _p = nUartGetInstance(usDevType);
            } else if (PROTO_DEV_IS_ZIGBEE(usDevType)) {
                extern rt_bool_t g_zigbee_init;
                if (g_zigbee_init) {
                    if (zgb_set_dst_node(ZGB_SN_T_MODBUS_RTU, btSlaveAddr)) {
                        _p = BOARD_ZGB_UART;
                    }
                }
            } else if (PROTO_DEV_IS_LORA(usDevType)) {
                extern rt_bool_t g_lora_init;
                if (g_lora_init) {
                    if (lora_set_dst_node(LORA_SN_T_MODBUS_RTU, btSlaveAddr)) {
                        _p = BOARD_LORA_UART;
                    }
                }
            } else if (PROTO_DEV_IS_NET(usDevType)) {
                _p = usDevNum;
            } else if (PROTO_DEV_IS_GPRS(usDevType)) {
                _p = usDevNum + BOARD_ENET_TCPIP_NUM;
            }
            
            if(_p >= 0) {
                switch(btOpCode) {
                case MODBUS_FC_READ_HOLDING_REGISTERS:
                    modbus_write_registers_with(usDevType, _p, btSlaveAddr, usExtAddr, usNRegs, (const uint16_t *)regs);
                    break;
                }
            }
            __SlaveRefreshExtDataWithRegs(usAddress);
            usAddress+=nregs;
            usNRegs = ((usNRegs>nregs)?(usNRegs-nregs):0);
            ret = VAR_TRUE;
        } else {
            // skip one reg
            usAddress++;
            usNRegs--;
        }
    }
    return ret;
}

void __refresh_regs(var_uint8_t *regbuf, var_uint16_t usAddr, int nregs)
{
    var_uint16_t *regs = &g_xExtDataRegs[usAddr-USER_REG_EXT_DATA_START];
    int iRegIndex = 0;
    while (nregs > 0) {
        regs[iRegIndex] = *regbuf++;
        regs[iRegIndex] |= *regbuf++ << 8;
        iRegIndex++;
        nregs--;
    }
}

void __clear_regs(var_uint16_t usAddr, int nregs)
{
    //var_uint16_t *regs = &g_xExtDataRegs[usAddr-USER_REG_EXT_DATA_START];
    memset(&g_xExtDataRegs[usAddr-USER_REG_EXT_DATA_START], 0, 2*nregs);
}

static void vVarManage_ModbusMasterTryClearExtDataWithErr(
    var_uint16_t usDevType, 
    var_uint16_t usDevNum,
    var_uint8_t btSlaveAddr, 
    var_uint16_t usAddress, 
    var_uint16_t usNRegs
)
{
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            int nregs = (node->xIo.btOutVarSize+1)/2;
            if (node->bEnable &&
                node->xIo.usDevType == usDevType &&
                node->xIo.usDevNum == usDevNum &&
                node->xIo.param.modbus.btSlaveAddr == btSlaveAddr &&
                node->xIo.param.modbus.usExtAddr >= usAddress &&
                nregs + node->xIo.param.modbus.usExtAddr <= usAddress + usNRegs) {
                if(E_ERR_OP_CLEAR == node->xIo.btErrOp) {
                    if(node->xIo.btErrNum >= node->xIo.btErrCnt) {
                        __clear_regs(node->xIo.usAddr, nregs);
                        memset(&node->xIo.xValue, 0, sizeof(node->xIo.xValue));
                        node->xIo.btErrNum = 0;
                    }
                    node->xIo.btErrNum++;
                }
                node->xIo.bErrFlag = RT_TRUE;
            }
            node = node->next;
        }
    }
    rt_exit_critical();
}

static void vVarManage_ObModbusMasterTryClearExtDataWithErr(
    var_uint16_t usDevType, 
    var_uint16_t usDevNum,
    var_uint8_t btSlaveAddr, 
    var_uint16_t usAddress, 
    var_uint16_t usNRegs
)
{
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            int nregs = (node->xIo.btOutVarSize+1)/2;
            if (node->bEnable &&
                node->xIo.usDevType == usDevType &&
                node->xIo.usDevNum == usDevNum &&
                node->xIo.param.modbus.btSlaveAddr == btSlaveAddr &&
                node->xIo.param.modbus.usExtAddr >= usAddress &&
                nregs + node->xIo.param.modbus.usExtAddr <= usAddress + (usNRegs + 1 / 2)) {
                if(E_ERR_OP_CLEAR == node->xIo.btErrOp) {
                    if(node->xIo.btErrNum >= node->xIo.btErrCnt) {
                        __clear_regs(node->xIo.usAddr, nregs);
                        memset(&node->xIo.xValue, 0, sizeof(node->xIo.xValue));
                        node->xIo.btErrNum = 0;
                    }
                    node->xIo.btErrNum++;
                }
                node->xIo.bErrFlag = RT_TRUE;
            }
            node = node->next;
        }
    }
    rt_exit_critical();
}

var_bool_t bVarManage_RefreshExtDataWithModbusMaster(
    var_uint16_t usDevType, 
    var_uint16_t usDevNum, 
    var_uint8_t btSlaveAddr, 
    var_uint16_t usAddress, 
    var_uint16_t usNRegs, 
    var_uint8_t *pucRegBuffer
)
{
    var_bool_t ret = VAR_FALSE;
    {
        rt_enter_critical();
        if (s_xExtDataList.pList) {
            ExtData_t *node = s_xExtDataList.pList;
            while (node) {
                int nregs = (node->xIo.btInVarSize+1)/2;
                if (node->bEnable &&
                    node->xIo.usDevType == usDevType &&
                    node->xIo.usDevNum == usDevNum &&
                    node->xIo.param.modbus.btSlaveAddr == btSlaveAddr &&
                    node->xIo.param.modbus.usExtAddr >= usAddress &&
                    nregs + node->xIo.param.modbus.usExtAddr <= usAddress + usNRegs) {
                    int ofs = node->xIo.param.modbus.usExtAddr - usAddress;
                    __refresh_regs(pucRegBuffer + (ofs << 1), node->xIo.usAddr, nregs);
                    __MasterRefreshExtDataWithRegs(node->xIo.usAddr);
                }
                node = node->next;
            }
        }
        rt_exit_critical();
        ret = VAR_TRUE;
    }

    return ret;
}

void __read_regs(var_uint8_t *regbuf, var_uint16_t usAddr, int nregs)
{
    var_uint16_t *regs = &g_xExtDataRegs[usAddr-USER_REG_EXT_DATA_START];
    int iRegIndex = 0;
    while (nregs > 0) {
        *regbuf++ = (UCHAR)(regs[iRegIndex] & 0xFF);
        *regbuf++ = (UCHAR)(regs[iRegIndex] >> 8 & 0xFF);
        iRegIndex++;
        nregs--;
    }
}

var_bool_t bVarManage_RefreshExtDataWithObModbusMaster(
    var_uint16_t usDevType, 
    var_uint16_t usDevNum, 
    var_uint8_t btSlaveAddr, 
    var_uint16_t usAddress, 
    var_uint16_t usNRegs, 
    var_uint8_t *pucRegBuffer
)
{
    var_bool_t ret = VAR_FALSE;
    {
        rt_enter_critical();
        if (s_xExtDataList.pList) {
            ExtData_t *node = s_xExtDataList.pList;
            while (node) {
                int nregs = (node->xIo.btInVarSize+1)/2;
                if (node->bEnable &&
                    node->xIo.usDevType == usDevType &&
                    node->xIo.usDevNum == usDevNum &&
                    node->xIo.param.modbus.btSlaveAddr == btSlaveAddr &&
                    node->xIo.param.modbus.usExtAddr >= usAddress &&
                    nregs + node->xIo.param.modbus.usExtAddr <= usAddress + (usNRegs + 1) / 2) {
                    int ofs = node->xIo.param.modbus.usExtAddr - usAddress;
                    __refresh_regs(pucRegBuffer + (ofs << 1), node->xIo.usAddr, nregs);
                    __MasterRefreshExtDataWithRegs(node->xIo.usAddr);
                }
                node = node->next;
            }
        }
        rt_exit_critical();
        ret = VAR_TRUE;
    }

    return ret;
}


static var_int_t __GetExtDataIoInfoWithDlt645Addr(
    dlt645_addr_t   *dlt645_addr, 
    var_uint16_t    addr, 
    var_uint32_t    op, 
    var_int8_t      *var_type
)
{
    var_int_t nregs = 0;
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (node->bEnable &&
                0 == memcmp(&node->xIo.param.dlt645.addr, dlt645_addr, sizeof(dlt645_addr_t)) &&
                node->xIo.usAddr == addr &&
                node->xIo.param.dlt645.op == op ) {
                *var_type = node->xIo.btInVarType;
                nregs = (node->xIo.btInVarSize+1)/2;
                break;
            }
            node = node->next;
        }
    }
    rt_exit_critical();
    return nregs;
}

//var_uint8_t btSlaveAddr, var_uint16_t usAddress, var_uint16_t usNRegs, var_uint8_t *pucRegBuffer
var_bool_t bVarManage_RefreshExtDataWithDlt645(dlt645_addr_t *dlt645_addr, var_uint16_t addr, mdUINT32 op, float val)
{
    var_bool_t ret = VAR_FALSE;
    var_int8_t var_type = -1;
    var_int_t nregs = __GetExtDataIoInfoWithDlt645Addr(dlt645_addr, addr, op, &var_type);
    if( nregs > 0 ) {
        var_uint8_t f_buf[8] = {0};
        long intval = (long)val;
        if (var_type <= E_VAR_BIT) {
            f_buf[0] = intval?1:0;
        } else if (var_type <= E_VAR_UINT8) {
            f_buf[0] = intval&0xFF;
        } else if (var_type <= E_VAR_UINT16) {
            f_buf[0] = (intval>>8)&0xFF;
            f_buf[1] = intval&0xFF;
        } else if (var_type <= E_VAR_UINT32) {
            f_buf[0] = (intval>>24)&0xFF;
            f_buf[1] = (intval>>16)&0xFF;
            f_buf[2] = (intval>>8)&0xFF;
            f_buf[3] = intval&0xFF;
        } else if (var_type <= E_VAR_FLOAT) {
            var_uint8_t tmp;
            memcpy(f_buf, &val, 4);
            tmp = f_buf[0]; f_buf[0] = f_buf[3]; f_buf[3] = tmp;
            tmp = f_buf[1]; f_buf[1] = f_buf[2]; f_buf[2] = tmp;
        } else if (var_type == E_VAR_DOUBLE) {
            var_uint8_t tmp;
            double dbval = (double)val;
            memcpy(f_buf, &dbval, 8);
            tmp = f_buf[0]; f_buf[0] = f_buf[7]; f_buf[7] = tmp;
            tmp = f_buf[1]; f_buf[1] = f_buf[6]; f_buf[6] = tmp;
            tmp = f_buf[2]; f_buf[2] = f_buf[5]; f_buf[5] = tmp;
            tmp = f_buf[3]; f_buf[3] = f_buf[4]; f_buf[4] = tmp;
        }
        rt_enter_critical();
        {
            __refresh_regs(f_buf, addr, nregs>4?4:nregs);
        }
        rt_exit_critical();
        __MasterRefreshExtDataWithRegs(addr);
    }

    return ret;
}

static void vVarManage_Dlt645TryClearExtDataWithErr(dlt645_addr_t *dlt645_addr, var_uint16_t addr, var_uint32_t op)
{
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (node->bEnable &&
                0 == memcmp(&node->xIo.param.dlt645.addr, dlt645_addr, sizeof(dlt645_addr_t)) &&
                node->xIo.usAddr == addr &&
                node->xIo.param.dlt645.op == op ) {
                if(E_ERR_OP_CLEAR == node->xIo.btErrOp) {
                    if(node->xIo.btErrNum >= node->xIo.btErrCnt) {
                        __clear_regs(node->xIo.usAddr, (node->xIo.btOutVarSize+1)/2);
                        memset(&node->xIo.xValue, 0, sizeof(node->xIo.xValue));
                        node->xIo.btErrNum = 0;
                    }
                    node->xIo.btErrNum++;
                }
                node->xIo.bErrFlag = RT_TRUE;
                break;
            }
            node = node->next;
        }
    }
    rt_exit_critical();
}

static var_int_t __GetExtDataIoInfoWithDust(
    var_uint16_t    addr, 
    var_uint8_t     op, 
    var_int8_t      *var_type
)
{
    var_int_t nregs = 0;
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (node->bEnable &&
                node->xIo.usAddr == addr &&
                node->xIo.param.dust.op == op ) {
                *var_type = node->xIo.btInVarType;
                nregs = (node->xIo.btInVarSize+1)/2;
                break;
            }
            node = node->next;
        }
    }
    rt_exit_critical();
    return nregs;
}

var_bool_t bVarManage_RefreshExtDataWithDust(var_uint16_t addr, var_uint8_t op, Dust_ResultData_t *result)
{
    var_bool_t ret = VAR_FALSE;
    var_int8_t var_type = -1;
    var_int_t nregs = __GetExtDataIoInfoWithDust(addr, op, &var_type);
    if( nregs > 0 ) {
        var_uint8_t f_buf[8] = {0};
        long intval = 0;
        switch(op) {
        case DUST_OP_PM25: intval = result->PM25; break;
        case DUST_OP_PM10: intval = result->PM10; break;
        case DUST_OP_PM25_SUM: intval = result->PM25Sum; break;
        case DUST_OP_PM10_SUM: intval = result->PM10Sum; break;
        case DUST_OP_PM25_NUM: intval = result->PM25Num; break;
        case DUST_OP_PM10_NUM: intval = result->PM10Num; break;
        case DUST_OP_T: intval = result->T; break;
        case DUST_OP_H: intval = result->H; break;
        default: ret = VAR_FALSE; goto end;
        }
        if (var_type <= E_VAR_BIT) {
            f_buf[0] = intval?1:0;
        } else if (var_type <= E_VAR_UINT8) {
            f_buf[0] = intval&0xFF;
        } else if (var_type <= E_VAR_UINT16) {
            f_buf[0] = (intval>>8)&0xFF;
            f_buf[1] = intval&0xFF;
        } else if (var_type <= E_VAR_UINT32) {
            f_buf[0] = (intval>>24)&0xFF;
            f_buf[1] = (intval>>16)&0xFF;
            f_buf[2] = (intval>>8)&0xFF;
            f_buf[3] = intval&0xFF;
        }
        rt_enter_critical();
        {
            __refresh_regs(f_buf, addr, nregs>2?2:nregs);
        }
        rt_exit_critical();
        __MasterRefreshExtDataWithRegs(addr);
    }
end:
    return ret;
}

static void vVarManage_DustTryClearExtDataWithErr(var_uint16_t addr, var_uint8_t op)
{
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (node->bEnable &&
                node->xIo.usAddr == addr &&
                node->xIo.param.dust.op == op ) {
                if(E_ERR_OP_CLEAR == node->xIo.btErrOp) {
                    if(node->xIo.btErrNum >= node->xIo.btErrCnt) {
                        __clear_regs(node->xIo.usAddr, (node->xIo.btOutVarSize+1)/2);
                        memset(&node->xIo.xValue, 0, sizeof(node->xIo.xValue));
                        node->xIo.btErrNum = 0;
                    }
                    node->xIo.btErrNum++;
                }
                break;
            }
            node = node->next;
        }
    }
    rt_exit_critical();
}

static var_int_t __GetExtDataIoInfoWithMbus603(
    var_uint16_t    addr, 
    var_uint8_t     op, 
    var_int8_t      *var_type
)
{
    var_int_t nregs = 0;
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (node->bEnable &&
                node->xIo.usAddr == addr &&
                node->xIo.param.mbus603.op == op ) {
                *var_type = node->xIo.btInVarType;
                nregs = (node->xIo.btInVarSize+1)/2;
                break;
            }
            node = node->next;
        }
    }
    rt_exit_critical();
    return nregs;
}

var_bool_t bVarManage_RefreshExtDataWithMbus603(var_uint16_t addr, var_uint8_t op, S_Mbus603_Result_t *result)
{
    var_bool_t ret = VAR_FALSE;
    var_int8_t var_type = -1;
    var_int_t nregs = __GetExtDataIoInfoWithMbus603(addr, op, &var_type);
    if( nregs > 0 ) {
        var_uint8_t f_buf[8] = {0};
        long intval = 0;
        switch(op) {
        case MBUS603_OP_SUM_E1: intval = result->sum_e1; break;
        case MBUS603_OP_E3: intval = result->e3; break;
        case MBUS603_OP_E8: intval = result->e8; break;
        case MBUS603_OP_E9: intval = result->e9; break;
        case MBUS603_OP_V1: intval = result->v1; break;
        case MBUS603_OP_TIME_SUM: intval = result->time_sum; break;
        case MBUS603_OP_T1: intval = result->T1; break;
        case MBUS603_OP_T2: intval = result->T2; break;
        case MBUS603_OP_DT: intval = result->dt; break;
        case MBUS603_OP_E1_E3: intval = result->e1_e3; break;
        case MBUS603_OP_MAX_MONTH_POWER: intval = result->max_month_power; break;
        case MBUS603_OP_F_RATE: intval = result->f_rate; break;
        default: ret = VAR_FALSE; goto end;
        }
        if (var_type <= E_VAR_BIT) {
            f_buf[0] = intval?1:0;
        } else if (var_type <= E_VAR_UINT8) {
            f_buf[0] = intval&0xFF;
        } else if (var_type <= E_VAR_UINT16) {
            f_buf[0] = (intval>>8)&0xFF;
            f_buf[1] = intval&0xFF;
        } else if (var_type <= E_VAR_UINT32) {
            f_buf[0] = (intval>>24)&0xFF;
            f_buf[1] = (intval>>16)&0xFF;
            f_buf[2] = (intval>>8)&0xFF;
            f_buf[3] = intval&0xFF;
        }
        rt_enter_critical();
        {
            __refresh_regs(f_buf, addr, nregs>2?2:nregs);
        }
        rt_exit_critical();
        __MasterRefreshExtDataWithRegs(addr);
    }
end:
    return ret;
}


static var_int_t __GetExtDataIoInfoWithSmf(
    var_uint16_t    addr, 
    var_uint8_t     op, 
    var_int8_t      *var_type
)
{
    var_int_t nregs = 0;
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (node->bEnable &&
                node->xIo.usAddr == addr &&
                node->xIo.param.smf.op == op ) {
                *var_type = node->xIo.btInVarType;
                nregs = (node->xIo.btInVarSize+1)/2;
                break;
            }
            node = node->next;
        }
    }
    rt_exit_critical();
    return nregs;
}

var_bool_t bVarManage_RefreshExtDataWithSmf(var_uint16_t addr, var_uint8_t op, s_instrumentData_t *result)
{
    var_bool_t ret = VAR_FALSE;
    var_int8_t var_type = -1;
    var_int_t nregs = __GetExtDataIoInfoWithSmf(addr, op, &var_type);
    if( nregs > 0 ) {
        var_uint8_t f_buf[8] = {0};
        float val = 0.0f;
        switch(op) {
        case SMF_OP_NO2: val = result->Sensor[SENSOR_NO2_INDEX].fval; break;
        case SMF_OP_O3: val = result->Sensor[SENSOR_O3_INDEX].fval; break;
        case SMF_OP_SO2: val = result->Sensor[SENSOR_SO2_INDEX].fval; break;
        case SMF_OP_CO: val = result->Sensor[SENSOR_CO_INDEX].fval; break;
        case SMF_OP_PM2_5: val = result->PM2_5.fval; break;
        case SMF_OP_PM10: val = result->PM10.fval; break;
        case SMF_OP_T: val = result->inter_temp.fval; break;
        case SMF_OP_H: val = result->RH.fval; break;
        case SMF_OP_P: val = result->press.fval; break;
        case SMF_OP_ST: val = result->sensor_temp.fval; break;
        default: ret = VAR_FALSE; goto end;
        }
        long intval = (long)val;
        if (var_type <= E_VAR_BIT) {
            f_buf[0] = intval?1:0;
        } else if (var_type <= E_VAR_UINT8) {
            f_buf[0] = intval&0xFF;
        } else if (var_type <= E_VAR_UINT16) {
            f_buf[0] = (intval>>8)&0xFF;
            f_buf[1] = intval&0xFF;
        } else if (var_type <= E_VAR_UINT32) {
            f_buf[0] = (intval>>24)&0xFF;
            f_buf[1] = (intval>>16)&0xFF;
            f_buf[2] = (intval>>8)&0xFF;
            f_buf[3] = intval&0xFF;
        } else if (var_type <= E_VAR_FLOAT) {
            var_uint8_t tmp;
            memcpy(f_buf, &val, 4);
            tmp = f_buf[0]; f_buf[0] = f_buf[3]; f_buf[3] = tmp;
            tmp = f_buf[1]; f_buf[1] = f_buf[2]; f_buf[2] = tmp;
        } else if (var_type == E_VAR_DOUBLE) {
            var_uint8_t tmp;
            double dbval = (double)val;
            memcpy(f_buf, &dbval, 8);
            tmp = f_buf[0]; f_buf[0] = f_buf[7]; f_buf[7] = tmp;
            tmp = f_buf[1]; f_buf[1] = f_buf[6]; f_buf[6] = tmp;
            tmp = f_buf[2]; f_buf[2] = f_buf[5]; f_buf[5] = tmp;
            tmp = f_buf[3]; f_buf[3] = f_buf[4]; f_buf[4] = tmp;
        }
        rt_enter_critical();
        {
            __refresh_regs(f_buf, addr, nregs>2?2:nregs);
        }
        rt_exit_critical();
        __MasterRefreshExtDataWithRegs(addr);
    }
end:
    return ret;
}

static void vVarManage_SmfTryClearExtDataWithErr(var_uint16_t addr, var_uint8_t op)
{
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (node->bEnable &&
                node->xIo.usAddr == addr &&
                node->xIo.param.smf.op == op ) {
                if(E_ERR_OP_CLEAR == node->xIo.btErrOp) {
                    if(node->xIo.btErrNum >= node->xIo.btErrCnt) {
                        __clear_regs(node->xIo.usAddr, (node->xIo.btOutVarSize+1)/2);
                        memset(&node->xIo.xValue, 0, sizeof(node->xIo.xValue));
                        node->xIo.btErrNum = 0;
                    }
                    node->xIo.btErrNum++;
                }
                break;
            }
            node = node->next;
        }
    }
    rt_exit_critical();
}

static var_int_t __GetExtDataIoInfoWithDh(
    var_uint16_t    addr, 
    var_uint32_t    sid, 
    var_uint32_t    type, 
    dh_key_t        *key, 
    var_int8_t      *var_type
)
{
    var_int_t nregs = 0;
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (node->bEnable &&
                node->xIo.usAddr == addr &&
                node->xIo.param.dh.sid == sid &&
                node->xIo.param.dh.type == type &&
                strcmp(node->xIo.param.dh.key.key, key->key) == 0) {
                *var_type = node->xIo.btInVarType;
                nregs = (node->xIo.btInVarSize+1)/2;
                break;
            }
            node = node->next;
        }
    }
    rt_exit_critical();
    return nregs;
}

static void vVarManage_DhTryClearOneExtDataWithErr(var_uint16_t addr, var_uint32_t sid, var_uint32_t type)
{
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (node->bEnable &&
                node->xIo.usAddr == addr &&
                node->xIo.param.dh.sid == sid &&
                node->xIo.param.dh.type == type) {
                if (E_ERR_OP_CLEAR == node->xIo.btErrOp) {
                    if(node->xIo.btErrNum >= node->xIo.btErrCnt) {
                        __clear_regs(node->xIo.usAddr, (node->xIo.btOutVarSize+1)/2);
                        memset(&node->xIo.xValue, 0, sizeof(node->xIo.xValue));
                        node->xIo.btErrNum = 0;
                    }
                    node->xIo.btErrNum++;
                }
                break;
            }
            node = node->next;
        }
    }
    rt_exit_critical();
}

static void vVarManage_DhTryClearExtDataWithErr(var_uint32_t sid, var_uint32_t type)
{
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (node->bEnable &&
                node->xIo.param.dh.sid == sid &&
                node->xIo.param.dh.type == type) {
                vVarManage_DhTryClearOneExtDataWithErr(node->xIo.usAddr, sid, type);
            }
            node = node->next;
        }
    }
    rt_exit_critical();
}

var_bool_t bVarManage_RefreshExtDataWithDh(var_uint16_t addr, var_uint32_t sid, var_uint32_t type, dh_key_t *key, cJSON *rsp)
{
    var_bool_t ret = VAR_FALSE;
    var_int8_t var_type = -1;
    var_int_t nregs = __GetExtDataIoInfoWithDh(addr, sid, type, key, &var_type);
    if( nregs > 0 ) {
        var_uint8_t f_buf[8] = {0};
        double val = (double)dh_get_rsp_value(rsp, sid, type, key);
        printf("var_type = %d\n", var_type);
        if (!isnan(val)) {
            long intval = (long)val;
            if (var_type <= E_VAR_BIT) {
                f_buf[0] = intval?1:0;
            } else if (var_type <= E_VAR_UINT8) {
                f_buf[0] = intval&0xFF;
            } else if (var_type <= E_VAR_UINT16) {
                f_buf[0] = (intval>>8)&0xFF;
                f_buf[1] = intval&0xFF;
            } else if (var_type <= E_VAR_UINT32) {
                f_buf[0] = (intval>>24)&0xFF;
                f_buf[1] = (intval>>16)&0xFF;
                f_buf[2] = (intval>>8)&0xFF;
                f_buf[3] = intval&0xFF;
            } else if (var_type <= E_VAR_FLOAT) {
                var_uint8_t tmp;
                float fbval = (float)val;
                memcpy(f_buf, &fbval, 4);
                tmp = f_buf[0]; f_buf[0] = f_buf[3]; f_buf[3] = tmp;
                tmp = f_buf[1]; f_buf[1] = f_buf[2]; f_buf[2] = tmp;
            } else if (var_type == E_VAR_DOUBLE) {
                var_uint8_t tmp;
                double dbval = (double)val;
                memcpy(f_buf, &dbval, 8);
                tmp = f_buf[0]; f_buf[0] = f_buf[7]; f_buf[7] = tmp;
                tmp = f_buf[1]; f_buf[1] = f_buf[6]; f_buf[6] = tmp;
                tmp = f_buf[2]; f_buf[2] = f_buf[5]; f_buf[5] = tmp;
                tmp = f_buf[3]; f_buf[3] = f_buf[4]; f_buf[4] = tmp;
            }
            rt_enter_critical();
            {
                __refresh_regs(f_buf, addr, nregs>2?2:nregs);
            }
            rt_exit_critical();
            __MasterRefreshExtDataWithRegs(addr);
        } else {
            vVarManage_DhTryClearOneExtDataWithErr(addr, sid, type);
        }
    }
end:
    return ret;
}

var_bool_t bVarManage_RefreshExtDataListWithDh(var_uint32_t sid, var_uint32_t type, cJSON *rsp)
{
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (node->bEnable &&
                node->xIo.param.dh.sid == sid &&
                node->xIo.param.dh.type == type) {
                bVarManage_RefreshExtDataWithDh(node->xIo.usAddr, sid, type, &node->xIo.param.dh.key, rsp);
            }
            node = node->next;
        }
    }
    rt_exit_critical();
}

static var_bool_t prvCheckExeDataEnable(ExtData_t *pData)
{
    return (pData->bEnable &&
            pData->xIo.btInVarType <= E_VAR_ARRAY && pData->xIo.btOutVarType <= E_VAR_ARRAY &&
            pData->xIo.usAddr >= USER_REG_EXT_DATA_START && pData->xIo.usAddr <= USER_REG_EXT_DATA_END &&
            PROTO_DEV_CHECK(pData->xIo.usDevType));
}

void vVarManage_RefreshExtDataStorageAvg(ExtData_t *data)
{
    rt_enter_critical();
    {
        ExtData_t *node = __GetExtDataWithName(data->xName.szName);
        if( node ) {
            data->xStorage.xAvgReal = node->xStorage.xAvgReal;
            data->xStorage.xAvgMin = node->xStorage.xAvgMin;
            data->xStorage.xAvgHour = node->xStorage.xAvgHour;
        }
    }
    rt_exit_critical();
}

void vVarManage_ClearExtDataStorageAvgReal(ExtData_t *data)
{
    rt_enter_critical();
    {
        ExtData_t *node = __GetExtDataWithName(data->xName.szName);
        if( node ) {
            node->xStorage.xAvgReal.val_avg = 0;
            node->xStorage.xAvgReal.count = 0;
            node->xStorage.xAvgReal.time = rt_tick_get();
        }
    }
    rt_exit_critical();
}

void vVarManage_ClearExtDataStorageAvgMin(ExtData_t *data)
{
    rt_enter_critical();
    {
        ExtData_t *node = __GetExtDataWithName(data->xName.szName);
        if( node ) {
            node->xStorage.xAvgMin.val_avg = 0;
            node->xStorage.xAvgMin.count = 0;
            node->xStorage.xAvgMin.time = rt_tick_get();
        }
    }
    rt_exit_critical();
}

void vVarManage_ClearExtDataStorageAvgHour(ExtData_t *data)
{
    rt_enter_critical();
    {
        ExtData_t *node = __GetExtDataWithName(data->xName.szName);
        if( node ) {
            node->xStorage.xAvgHour.val_avg = 0;
            node->xStorage.xAvgHour.count = 0;
            node->xStorage.xAvgHour.time = rt_tick_get();
        }
    }
    rt_exit_critical();
}

static void vVarManage_RefreshExtDataUpAvg(ExtData_t *data)
{
    rt_enter_critical();
    {
        ExtData_t *node = __GetExtDataWithName(data->xName.szName);
        if( node ) {
            data->xUp.xAvgUp = node->xUp.xAvgUp;
        }
    }
    rt_exit_critical();
}

static void vVarManage_ClearExtDataUp(ExtData_t *data)
{
    rt_enter_critical();
    {
        ExtData_t *node = __GetExtDataWithName(data->xName.szName);
        if( node ) {
            node->xUp.xAvgUp.val_avg = 0;
            node->xUp.xAvgUp.count = 0;
            node->xUp.xAvgUp.time = rt_tick_get();
        }
    }
    rt_exit_critical();
}

// add by jay 2016/11/25
// 不安全函数, 必须在 rt_enter_critical rt_exit_critical 中调用
ExtData_t* pVarManage_GetExtDataWithUpProto(
    ExtData_t       *first,
    var_uint16_t    usDevType,
    var_uint16_t    usDevNum,
    var_uint16_t    usProtoType
    )
{
    ExtData_t *ret = VAR_NULL;
    if (s_xExtDataList.pList) {
        ExtData_t *node = (first ? first->next : s_xExtDataList.pList);
        while (node) {
            if (node->bEnable && node->xUp.bEnable &&
                usDevType == node->xUp.usDevType &&
                usDevNum == node->xUp.usDevNum &&
                usProtoType == node->xUp.usProtoType) {
                ret = node;
                break;
            }
            node = node->next;
        }
    }
    return ret;
}

rt_bool_t cfg_recover_busy(void);

void vVarManage_ExtDataRefreshTask(void *parameter)
{
    rt_thread_delay(5*RT_TICK_PER_SECOND);
    int index = (int)(long)parameter;
    while (1) {
        sync_slave_t *slave = VAR_NULL;
        sync_ext_t *ext = VAR_NULL;
        var_bool_t com_flag[BOARD_UART_MAX] = {0};
        for(int i = PROTO_DEV_RS1; i <= PROTO_DEV_RS_MAX; i++) {
            com_flag[i] = VAR_TRUE;
        }
        com_flag[BOARD_LORA_UART] = VAR_TRUE;
        var_bool_t net_flag[BOARD_TCPIP_MAX] = {0};
        for(int i = 0; i <= BOARD_TCPIP_MAX; i++) {
            net_flag[i] = VAR_TRUE;
        }
        while (!cfg_recover_busy()) {
            rt_thddog_feed("");
            sync_node_t sync_node;
            rt_tick_t now_tick = rt_tick_get();
            rt_tick_t diff_tick = 0;
            if (__get_next_sync_ext_node(&slave, &ext, &sync_node)) {
                if (s_varmanage_run[index].dev_type != sync_node.dev_type || 
                    s_varmanage_run[index].dev_num != sync_node.dev_num) {
                    continue;
                }
                if (group_search_flag && group_search_dev_type == sync_node.dev_type && group_search_dev_num == sync_node.dev_num) {
                    while (group_search_flag) {
                        rt_thddog_feed("wait group search");
                        rt_thread_delay(100);
                    }
                }
                if (__proto_is_rtu_self(sync_node.dev_type, sync_node.dev_num)) {
                    /*if (sync_node.dev_num < 8) {
                        // AI 1个通道占用两个寄存器
                        rt_enter_critical();
                        {
                            var_uint16_t reg = g_xAIResultReg.regs[sync_node.dev_num*2];
                            g_xExtDataRegs[sync_node.param.self.addr-USER_REG_EXT_DATA_START+1] = (reg << 8) | (reg >> 8);
                            reg = g_xAIResultReg.regs[sync_node.dev_num*2+1];
                            g_xExtDataRegs[sync_node.param.self.addr-USER_REG_EXT_DATA_START] = (reg << 8) | (reg >> 8);
                        }
                        rt_exit_critical();
                        rt_thddog_suspend("__proto_is_rtu_self");
                        __RefreshExtDataWithRegs(sync_node.param.self.addr, 2);
                        rt_thddog_resume();
                    }*/
                } else if (__proto_is_rtu_self_mid(sync_node.dev_type)) {
                    //中间变量1个占用四个寄存器
                    rt_thddog_suspend("__proto_is_rtu_self_mid");
                    __SlaveRefreshExtDataWithRegs(sync_node.param.self.addr);
                    rt_thddog_resume();
                } else if (__proto_is_modbus(sync_node.dev_type, sync_node.proto_type)) {
                    var_int8_t _p = -1;
                    var_uint8_t _sa = sync_node.param.modbus.slave_addr;
                    var_uint16_t _ea = sync_node.param.modbus.ext_addr;
                    var_uint8_t _es = sync_node.param.modbus.ext_size;
                    var_bool_t _flag = VAR_TRUE;
                    //int _lock = -1;
                    if (PROTO_DEV_IS_RS(sync_node.dev_type)) {
                        rt_thddog_suspend("__proto_is_modbus com");
                        _p = nUartGetInstance(sync_node.dev_type);
                        diff_tick = rt_tick_from_millisecond(g_uart_cfgs[_p].interval);
                        _flag = var_check_diff(now_tick, s_com_last_tick[_p], diff_tick);
                        if(com_flag[_p]) com_flag[_p] = _flag;
                    } else if (PROTO_DEV_IS_ZIGBEE(sync_node.dev_type)) {
                        extern rt_bool_t g_zigbee_init;
                        if (g_zigbee_init) {
                            if (zgb_set_dst_node(g_zigbee_cfg.nProtoType, sync_node.param.modbus.slave_addr)) {
                                rt_thddog_suspend("__proto_is_modbus zigbee");
                                _p = BOARD_ZGB_UART;
                            }
                        }
                    } else if (PROTO_DEV_IS_LORA(sync_node.dev_type)) {
                        extern rt_bool_t g_lora_init;
                        if (g_lora_init) {
                            if (lora_set_dst_node(g_lora_cfg.proto_type, sync_node.param.modbus.slave_addr)) {
                                rt_thddog_suspend("__proto_is_modbus lora");
                                _p = BOARD_LORA_UART;
                                diff_tick = rt_tick_from_millisecond(g_uart_cfgs[_p].interval);
                                _flag = var_check_diff(now_tick, s_com_last_tick[_p], diff_tick);
                                if(com_flag[_p]) com_flag[_p] = _flag;
                            }
                        }
                    } else if (PROTO_DEV_IS_NET(sync_node.dev_type)) { 
                        rt_thddog_suspend("__proto_is_modbus net");
                        _p = sync_node.dev_num;
                    } else if (PROTO_DEV_IS_GPRS(sync_node.dev_type)) { 
                        rt_thddog_suspend("__proto_is_modbus gprs");
                        _p = sync_node.dev_num + BOARD_ENET_TCPIP_NUM;
                    }

                    if(_p >= 0 && _flag) {
                        int rc = 0;
                        switch(sync_node.param.modbus.op) {
                        case MODBUS_FC_READ_HOLDING_REGISTERS:
                            rc = modbus_read_registers_with(sync_node.dev_type, _p, sync_node.dev_num, _sa, _ea, _es);
                            break;
                        case MODBUS_FC_READ_INPUT_REGISTERS:
                            rc = modbus_read_input_registers_with(sync_node.dev_type, _p, sync_node.dev_num, _sa, _ea, _es);
                            break;
                        }
                        if(rc < 0) {
                            vVarManage_ModbusMasterTryClearExtDataWithErr(sync_node.dev_type, sync_node.dev_num, _sa, _ea, _es);
                        }
                    }
                    if(_p >= 0 && _flag) {
                        rt_thread_delay(20);
					}
                    rt_thddog_resume();
                } else if (__proto_is_dlt645_2007(sync_node.dev_type, sync_node.proto_type)) {
                    S_DLT645_A_t dlt645_addr;
                    S_DLT645_Result_t dlt645_result;
                    rt_tick_t start_tick = rt_tick_get();
                    memcpy(&dlt645_addr, &sync_node.param.dlt645.dlt645_addr, sizeof(S_DLT645_A_t));
                    rt_thddog_suspend("__proto_is_dlt645_2007->bDlt645ReadData");
                    if( bDlt645ReadData(sync_node.dev_type, dlt645_addr, sync_node.param.dlt645.op, 1000, &dlt645_result) ) {
                        rt_thddog_suspend("__proto_is_dlt645_2007->RefreshExtDataWithDlt645");
                        bVarManage_RefreshExtDataWithDlt645(&sync_node.param.dlt645.dlt645_addr, sync_node.param.dlt645.addr, dlt645_result.op, dlt645_result.val);
                    } else {
                        vVarManage_Dlt645TryClearExtDataWithErr(&sync_node.param.dlt645.dlt645_addr, sync_node.param.dlt645.addr, sync_node.param.dlt645.op);
                    }
                    rt_thddog_resume();
                    int remain_tick = rt_tick_from_millisecond(500) - (rt_tick_get() - start_tick);
                    if (remain_tick > 0) rt_thread_delay(remain_tick);
                } else if (__proto_is_dlt645_1997(sync_node.dev_type, sync_node.proto_type)) {
                    S_DLT645_1997_A_t dlt645_addr;
                    S_DLT645_1997_Result_t dlt645_result;
                    rt_tick_t start_tick = rt_tick_get();
                    memcpy(&dlt645_addr, &sync_node.param.dlt645.dlt645_addr, sizeof(S_DLT645_1997_A_t));
                    rt_thddog_suspend("__proto_is_dlt645_1997->bDlt645ReadData");
                    if( bDlt645_1997ReadData(sync_node.dev_type, dlt645_addr, sync_node.param.dlt645.op, 1000, &dlt645_result) ) {
                        rt_thddog_suspend("__proto_is_dlt645_1997->RefreshExtDataWithDlt645");
                        bVarManage_RefreshExtDataWithDlt645(&sync_node.param.dlt645.dlt645_addr, sync_node.param.dlt645.addr, dlt645_result.op, dlt645_result.val);
                    } else {
                        vVarManage_Dlt645TryClearExtDataWithErr(&sync_node.param.dlt645.dlt645_addr, sync_node.param.dlt645.addr, sync_node.param.dlt645.op);
                    }
                    rt_thddog_resume();
                    int remain_tick = rt_tick_from_millisecond(500) - (rt_tick_get() - start_tick);
                    if (remain_tick > 0) rt_thread_delay(remain_tick);
                } else if (__proto_is_mbus603(sync_node.dev_type, sync_node.proto_type)) {
                    var_int8_t _p = -1;
                    var_bool_t _flag = VAR_TRUE;
                    S_MBUS_A_t mbus_addr;
                    S_Mbus603_Result_t mbus_result;
                    memcpy(&mbus_addr, &sync_node.param.mbus603.mbus_addr, sizeof(S_MBUS_A_t));

                    _p = nUartGetInstance(sync_node.dev_type);
                    diff_tick = rt_tick_from_millisecond(g_uart_cfgs[_p].interval);
                    _flag = var_check_diff(now_tick, s_com_last_tick[_p], diff_tick);
                    if(com_flag[_p]) com_flag[_p] = _flag;

                    if(_flag) {
                        // bDustStartSample(sync_node.dev_type, 1000);
                        rt_kprintf("bMbus603SampleData 0x%x\r\n",mbus_addr);
                        bMbus603SampleData(sync_node.dev_type, mbus_addr, 5000);  
                    }
                    
                    rt_thddog_suspend("__proto_is_mbus603->bMbus603ReadData");
                    if( bMbus603ReadData(sync_node.dev_type, mbus_addr, 1000, &mbus_result) ) {
                        rt_thddog_suspend("__proto_is_mbus603->RefreshExtDataWithMbus603");
                       // rt_kprintf("bMbus603ReadData: %d,op:%d\r\n",mbus_result.sum_e1,sync_node.param.mbus603.op);
                        bVarManage_RefreshExtDataWithMbus603(sync_node.param.mbus603.addr, sync_node.param.mbus603.op, &mbus_result);
                    } else {
                        //vVarManage_Dlt645TryClearExtDataWithErr(&sync_node.param.dlt645.dlt645_addr, sync_node.param.dlt645.addr, sync_node.param.dlt645.op);
                    }
                    rt_thddog_resume();
                } else if (__proto_is_dust(sync_node.dev_type, sync_node.proto_type)) {
                    var_int8_t _p = -1;
                    var_bool_t _flag = VAR_TRUE;
                    Dust_ResultData_t dust_data;

                    _p = nUartGetInstance(sync_node.dev_type);
                    diff_tick = rt_tick_from_millisecond(g_uart_cfgs[_p].interval);
                    _flag = var_check_diff(now_tick, s_com_last_tick[_p], diff_tick);
                    if(com_flag[_p]) com_flag[_p] = _flag;

                    if(_flag) {
                        bDustStartSample(sync_node.dev_type, 1000);
                    }
                    
                    rt_thddog_suspend("__proto_is_dust->bDust_ReadData");
                    if (bDust_ReadData(sync_node.dev_type, 0, &dust_data)) {
                        rt_thddog_suspend("__proto_is_dust->RefreshExtDataWithDust");
                        bVarManage_RefreshExtDataWithDust(sync_node.param.dust.addr, sync_node.param.dust.op, &dust_data);
                    } else {
                        //vVarManage_DustTryClearExtDataWithErr(sync_node.param.dust.addr, sync_node.param.dust.op);
                    }
                    rt_thddog_resume();
                } else if (__proto_is_smf(sync_node.dev_type, sync_node.proto_type)) {
                    var_int8_t _p = -1;
                    var_bool_t _flag = VAR_TRUE;
                    s_instrumentData_t smf_data;
                    
                    if (PROTO_DEV_IS_NET(sync_node.dev_type)) { 
                        rt_thddog_suspend("__proto_is_modbus net");
                        _p = sync_node.dev_num;
                    } else if (PROTO_DEV_IS_GPRS(sync_node.dev_type)) { 
                        rt_thddog_suspend("__proto_is_modbus gprs");
                        _p = sync_node.dev_num + BOARD_ENET_TCPIP_NUM;
                    }

                    if(_p >= 0) {
                        rt_thddog_suspend("__proto_is_smf->smf_read_data");
                        if (smf_read_data(_p, 0, &smf_data)) {
                            rt_thddog_suspend("__proto_is_smf->RefreshExtDataWithSmf");
                            //printf("%d\n", sync_node.param.smf.op);
                            bVarManage_RefreshExtDataWithSmf(sync_node.param.smf.addr, sync_node.param.smf.op, &smf_data);
                        } else {
                            //vVarManage_SmfTryClearExtDataWithErr(sync_node.param.smf.addr, sync_node.param.smf.op);
                        }
                        rt_thddog_resume();
                    }
                } else if (__proto_is_obmodbus(sync_node.dev_type, sync_node.proto_type)) {
                    var_int8_t _p = -1;
                    var_uint8_t _sa = sync_node.param.obmodbus.slave_addr;
                    var_uint16_t _ea = sync_node.param.obmodbus.ext_addr;
                    var_uint8_t _es = sync_node.param.obmodbus.ext_size;
                    var_bool_t _flag = VAR_TRUE;
                    //int _lock = -1;
                    if (PROTO_DEV_IS_RS(sync_node.dev_type)) {
                        rt_thddog_suspend("__proto_is_obmodbus com");
                        _p = nUartGetInstance(sync_node.dev_type);
                        diff_tick = rt_tick_from_millisecond(g_uart_cfgs[_p].interval);
                        _flag = var_check_diff(now_tick, s_com_last_tick[_p], diff_tick);
                        if(com_flag[_p]) com_flag[_p] = _flag;
                    }

                    if(_p >= 0 && _flag) {
                        int rc = obmodbus_read_registers_with(
                            sync_node.dev_type, _p, sync_node.dev_num, 
                            _sa, sync_node.param.obmodbus.op, _ea, _es);
                        
                        if(rc < 0) {
                            vVarManage_ObModbusMasterTryClearExtDataWithErr(sync_node.dev_type, sync_node.dev_num, _sa, _ea, _es);
                        }
                    }
                    if(_p >= 0 && _flag) {
                        rt_thread_delay(20);
					}
                    rt_thddog_resume();
                } else if (__proto_is_dh(sync_node.dev_type, sync_node.proto_type)) {
                    var_int8_t _p = sync_node.dev_num;
                    var_bool_t _flag = VAR_TRUE;
                    cJSON *rsp = NULL;
                    diff_tick = rt_tick_from_millisecond(g_tcpip_cfgs[_p].interval);
                    _flag = var_check_diff(now_tick, s_net_last_tick[_p], diff_tick);
                    if(net_flag[_p]) net_flag[_p] = _flag;

                    if (_flag) {
                        rt_thddog_suspend("__proto_is_dh->bDlt645ReadData");
                        rsp = dh_get_rsp(sync_node.dev_num, g_tcpip_cfgs[_p].interval, sync_node.param.dh.sid, sync_node.param.dh.type);
                        if (rsp && !dh_is_rsp_error(rsp)) {
                            rt_thddog_suspend("__proto_is_dh->RefreshExtDataWithDh");
                            bVarManage_RefreshExtDataListWithDh(sync_node.param.dh.sid, sync_node.param.dh.type, rsp);
                        } else {
                            //vVarManage_DhTryClearExtDataWithErr(sync_node.param.dh.sid, sync_node.param.dh.type);
                        }
                    }
                    dh_free_rsp(rsp);
                    int remain_tick = rt_tick_from_millisecond(500) - (rt_tick_get() - now_tick);
                    if (remain_tick > 0) rt_thread_delay(remain_tick);
                    rt_thddog_resume();
                }
            } else {
                if (s_varmanage_run[index].dev_type <= PROTO_DEV_RS_MAX) {
                    rt_thread_delay(100);
                } else if (s_varmanage_run[index].dev_type <= PROTO_DEV_NET) {
                    rt_thread_delay(100);
                } else if (s_varmanage_run[index].dev_type <= PROTO_DEV_ZIGBEE) {
                    rt_thread_delay(500);
                } else if (s_varmanage_run[index].dev_type <= PROTO_DEV_GPRS) {
                    rt_thread_delay(500);
                }
                break;
            }
        }
        if (PROTO_DEV_IS_NET(s_varmanage_run[index].dev_type)) { 
            smf_end_read_data(s_varmanage_run[index].dev_num);
        } else if (PROTO_DEV_IS_GPRS(s_varmanage_run[index].dev_type)) { 
            smf_end_read_data(s_varmanage_run[index].dev_num + BOARD_ENET_TCPIP_NUM);
        }
        if(!cfg_recover_busy()) {
            for(int _type = PROTO_DEV_RS1; _type <= PROTO_DEV_RS_MAX; _type++ ) {
                int _p = nUartGetInstance(_type);
                rt_tick_t now_tick = rt_tick_get();
                rt_tick_t diff_tick = rt_tick_from_millisecond(g_uart_cfgs[_p].interval);
                if (s_varmanage_run[index].dev_type != _type) {
                    continue;
                }
                if(var_check_diff(now_tick, s_com_last_tick[_p], diff_tick) && com_flag[_p]) {
                    s_com_last_tick[_p] = now_tick;
                }
            }
            {
                int _p = BOARD_LORA_UART;
                if (s_varmanage_run[index].dev_type == PROTO_DEV_LORA) {
                    rt_tick_t now_tick = rt_tick_get();
                    rt_tick_t diff_tick = rt_tick_from_millisecond(g_uart_cfgs[_p].interval);
                    if(var_check_diff(now_tick, s_com_last_tick[_p], diff_tick) && com_flag[_p]) {
                        s_com_last_tick[_p] = now_tick;
                    }
                }
            }
            if (PROTO_DEV_IS_NET(s_varmanage_run[index].dev_type)) {
                int _p = s_varmanage_run[index].dev_num;
                rt_tick_t now_tick = rt_tick_get();
                rt_tick_t diff_tick = rt_tick_from_millisecond(g_tcpip_cfgs[_p].interval);
                if(var_check_diff(now_tick, s_net_last_tick[_p], diff_tick) && net_flag[_p]) {
                    s_net_last_tick[_p] = now_tick;
                }
            }

            {
                ExtData_t *node = s_xExtDataList.pList;
                int num = s_xExtDataList.n;
                for (int n = 0; n < num; n++) {
                    node = __GetExtDataWithIndex(n);
                    if (!node) break;
                    if (s_varmanage_run[index].dev_type == node->xIo.usDevType &&
                        s_varmanage_run[index].dev_num == node->xIo.usDevNum && 
                        prvCheckExeDataEnable(node)) {
                        //vVarManage_RefreshExtDataStorageAvg(node);
                        if (g_storage_cfg.bEnable && node->xStorage.bEnable) {
                            // real
                            if (rt_tick_get() - node->xStorage.xAvgReal.time >= rt_tick_from_millisecond(node->xStorage.ulStep * 60 * 1000)) {
                                if (node->xStorage.xAvgReal.count > 0) {
                                    var_double_t value = (node->xStorage.xAvgReal.val_avg / node->xStorage.xAvgReal.count);
                                    rt_thddog_suspend("bStorageAddData real");
                                    bStorageAddData(ST_T_RT_DATA, node->xName.szName, value, node->xAlias.szAlias);
                                    rt_thddog_resume();
                                    vVarManage_ClearExtDataStorageAvgReal(node);
                                }
                            }
                            //min
                            if (rt_tick_get() - node->xStorage.xAvgMin.time >= rt_tick_from_millisecond(60 * 1000)) {
                                if (node->xStorage.xAvgMin.count > 0) {
                                    var_double_t value = (node->xStorage.xAvgMin.val_avg / node->xStorage.xAvgMin.count);
                                    rt_thddog_suspend("bStorageAddData min");
                                    bStorageAddData(ST_T_MINUTE_DATA, node->xName.szName, value, node->xAlias.szAlias);
                                    rt_thddog_resume();
                                    vVarManage_ClearExtDataStorageAvgMin(node);
                                }
                            }
                            //hour
                            if (rt_tick_get() - node->xStorage.xAvgHour.time >= rt_tick_from_millisecond(60 * 60 * 1000)) {
                                if (node->xStorage.xAvgHour.count > 0) {
                                    var_double_t value = (node->xStorage.xAvgHour.val_avg / node->xStorage.xAvgHour.count);
                                    rt_thddog_suspend("bStorageAddData hour");
                                    bStorageAddData(ST_T_HOURLY_DATA, node->xName.szName, value, node->xAlias.szAlias);
                                    rt_thddog_resume();
                                    vVarManage_ClearExtDataStorageAvgHour(node);
                                }
                            }
                        }
                    }
                    vVarManage_FreeData(node);
                }
            }
        }
        rt_thddog_feed("");
        rt_thread_delay(RT_TICK_PER_SECOND / 10);
    }
    rt_thddog_exit();
}

static int __varmanage_run_get(    var_uint16_t     dev_type, var_uint16_t dev_num)
{
    int n;
    for (n = 0; n < BOARD_COLL_MAX; n++) {
        if (!s_varmanage_run[n].use) {
            return n;
        } else if (s_varmanage_run[n].use && 
            s_varmanage_run[n].dev_type == dev_type && 
            s_varmanage_run[n].dev_num == dev_num) {
            return n;
        }
    }
    return -1;
}

void varmanage_update(void)
{
    rt_enter_critical();
    {
        rt_bool_t first = RT_TRUE;
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            int index = __varmanage_run_get(node->xIo.usDevType, node->xIo.usDevNum);
            if (index >= 0 && !s_varmanage_run[index].use) {
                s_varmanage_run[index].dev_type = node->xIo.usDevType;
                s_varmanage_run[index].dev_num = node->xIo.usDevNum;
                BOARD_CREAT_NAME(run_name, "col_refresh[%d:%d]", node->xIo.usDevType, node->xIo.usDevNum);
                s_varmanage_run[index].thread = \
                    rt_thread_create(run_name, vVarManage_ExtDataRefreshTask, (void *)(long)index, 0, 0, 0);
                if (s_varmanage_run[index].thread) {
                    s_varmanage_run[index].use = 1;
                    rt_thddog_register(s_varmanage_run[index].thread, 60);
                    rt_thread_startup(s_varmanage_run[index].thread);
                }
            }
            node = node->next;
        }
    }
    rt_exit_critical();
}

void varmanage_start(void)
{
    varmanage_update();
}

/*
void *vVarManage_ExtDataUpTask(void *parameter)
{
    while (1) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (prvCheckExeDataEnable(node)) {
                if (node->xUp.bEnable) {
                    var_int32_t up_interval = ulVarManage_GetExtDataUpInterval(node);
                    //up
                    if (up_interval > 0 && rt_tick_get() - node->xUp.xAvgUp.time >= rt_tick_from_millisecond(up_interval * 1000)) {
                        if (node->xUp.xAvgUp.count > 0) {
                            var_double_t value = (node->xUp.xAvgUp.val_avg / node->xUp.xAvgUp.count);
                            node->xUp.xAvgUp.val_avg = 0;
                            node->xUp.xAvgUp.count = 0;
                            node->xUp.xAvgUp.time = rt_tick_get();
                            //bStorageAddData(ST_T_HOURLY_DATA, node->xName.szName, value, node->xAlias.szAlias);

                        }
                    }
                }
            }
            node = node->next;
        }
        rt_thread_delay(RT_TICK_PER_SECOND / 10);
    }
}
*/

// S 为单位
var_int32_t lVarManage_GetExtDataUpInterval( var_uint16_t usDevType, var_uint16_t usDevNum )
{
    switch (usDevType) {
    case PROTO_DEV_RS1:
        return g_uart_cfgs[0].interval;
    case PROTO_DEV_RS2:
        return g_uart_cfgs[1].interval;
    case PROTO_DEV_NET:
        return g_tcpip_cfgs[usDevNum].interval;
    case PROTO_DEV_GPRS:
        return g_tcpip_cfgs[usDevNum + BOARD_ENET_TCPIP_NUM].interval;
    case PROTO_DEV_ZIGBEE:
        return -1;
    case PROTO_DEV_LORA:
        return -1;
    }
    return -1;
}

extern void prvSaveUartCfgsToFs(void);
extern void prvSaveTcpipCfgsToFs(void);

void vVarManage_SetExtDataUpInterval( var_uint16_t usDevType, var_uint16_t usDevNum, var_uint32_t interval )
{
    switch (usDevType) {
    case PROTO_DEV_RS1:
    case PROTO_DEV_RS2:
        g_uart_cfgs[usDevType].interval = interval;
        prvSaveUartCfgsToFs();
        break;
    case PROTO_DEV_NET:
        g_tcpip_cfgs[usDevNum].interval = interval;
        prvSaveTcpipCfgsToFs();
        break;
    case PROTO_DEV_ZIGBEE:
        break;
    case PROTO_DEV_GPRS:
        g_tcpip_cfgs[usDevNum + BOARD_ENET_TCPIP_NUM].interval = interval;
        prvSaveTcpipCfgsToFs();
        break;
    case PROTO_DEV_LORA:
        break;
    }
}

var_bool_t jsonParseExtDataInfo(cJSON *pItem, ExtData_t *data)
{
    if ( pItem && data) {
        cJSON *pIO = cJSON_GetObjectItem(pItem,"io");
        cJSON *pAlarm = cJSON_GetObjectItem(pItem,"alarm");
        cJSON *pStorage = cJSON_GetObjectItem(pItem,"storage");
        cJSON *pUp = cJSON_GetObjectItem(pItem,"up");
        cJSON *pCfg = cJSON_GetObjectItem(pItem,"cfg");
        int itmp;
        //double dtmp;
        const char *str;

        itmp = cJSON_GetInt(pItem, "en", -1);
        if (itmp >= 0) data->bEnable = (itmp != 0 ? VAR_TRUE : VAR_FALSE);
        str = cJSON_GetString(pItem, "gp", VAR_NULL);
        if (str) {
            memset(&data->xGroup, 0, sizeof(VarGroup_t)); strncpy(data->xGroup.szGroup, str, sizeof(VarGroup_t) - 1);
        }
        str = cJSON_GetString(pItem, "na", VAR_NULL);
        if (str) {
            memset(&data->xName, 0, sizeof(VarName_t)); strncpy(data->xName.szName, str, sizeof(VarName_t) - 1);
        }
        str = cJSON_GetString(pItem, "al", VAR_NULL);
        if (str) {
            memset(&data->xAlias, 0, sizeof(VarAlias_t)); strncpy(data->xAlias.szAlias, str, sizeof(VarAlias_t) - 1);
        }
        data->eAttr = cJSON_GetInt(pItem, "attr", data->eAttr);
        if (pIO) {
            data->xIo.btErrOp = cJSON_GetInt(pIO, "eop", data->xIo.btErrOp);
            data->xIo.btErrCnt = cJSON_GetInt(pIO, "ecnt", data->xIo.btErrCnt);
            data->xIo.btRWType = cJSON_GetInt(pIO, "rw", data->xIo.btRWType);
            data->xIo.btInVarType = cJSON_GetInt(pIO, "vt", data->xIo.btInVarType);
            data->xIo.btInVarSize = g_var_type_sz[data->xIo.btInVarType];
            
            //data->xIo.btInVarSize = cJSON_GetInt(pIO, "vs", data->xIo.btInVarSize);
            data->xIo.btOutVarType = cJSON_GetInt(pIO, "ovt", data->xIo.btInVarType);
            data->xIo.btOutVarSize = g_var_type_sz[data->xIo.btOutVarType];
            
            //data->xIo.btOutVarSize = cJSON_GetInt(pIO, "ovs", data->xIo.btInVarSize);
            data->xIo.btInVarRuleType = cJSON_GetInt(pIO, "vrl", data->xIo.btInVarRuleType);
            if(data->xIo.btInVarRuleType >= 4) data->xIo.btInVarRuleType = 0;
            data->xIo.btOutVarRuleType = cJSON_GetInt(pIO, "ovrl", data->xIo.btInVarRuleType);
            if(data->xIo.btOutVarRuleType >= 4) data->xIo.btOutVarRuleType = 0;
            data->xIo.bUseMax = cJSON_GetObjectItem(pIO, "vma") ? RT_TRUE : RT_FALSE;
            data->xIo.bUseMin = cJSON_GetObjectItem(pIO, "vmi") ? RT_TRUE : RT_FALSE;
            data->xIo.bUseInit = cJSON_GetObjectItem(pIO, "vii") ? RT_TRUE : RT_FALSE;
            data->xIo.bUseRatio = cJSON_GetObjectItem(pIO, "vrt") ? RT_TRUE : RT_FALSE;
            if (data->xIo.bUseMax) data->xIo.fMax = (var_float_t)cJSON_GetDouble(pIO, "vma", data->xIo.fMax);
            if (data->xIo.bUseMin) data->xIo.fMin = (var_float_t)cJSON_GetDouble(pIO, "vmi", data->xIo.fMin);
            if (data->xIo.bUseInit) data->xIo.fInit = (var_float_t)cJSON_GetDouble(pIO, "vii", data->xIo.fInit);
            if (data->xIo.bUseRatio) data->xIo.fRatio = (var_float_t)cJSON_GetDouble(pIO, "vrt", data->xIo.fRatio);

            data->xIo.usDevType = cJSON_GetInt(pIO, "dt", data->xIo.usDevType);
            data->xIo.usDevNum = cJSON_GetInt(pIO, "dtn", data->xIo.usDevNum);
            data->xIo.usProtoType = cJSON_GetInt(pIO, "pt", data->xIo.usProtoType);
            str = cJSON_GetString(pIO, "pnm", VAR_NULL);
            if (str) {
                VAR_MANAGE_FREE(data->xIo.szProtoName);
                if (strlen(str) > 0) data->xIo.szProtoName = rt_strdup(str);
            }

            data->xIo.usAddr = cJSON_GetInt(pIO, "ad", data->xIo.usAddr);

            if (__proto_is_modbus(data->xIo.usDevType, data->xIo.usProtoType) || 
                __proto_is_obmodbus(data->xIo.usDevType, data->xIo.usProtoType)) {
                data->xIo.param.modbus.btOpCode = cJSON_GetInt(pIO, "mbop", data->xIo.param.modbus.btOpCode);
                if(0 == data->xIo.param.modbus.btOpCode) data->xIo.param.modbus.btOpCode = 3;
                data->xIo.param.modbus.nSyncFAddr = cJSON_GetInt(pIO, "sfa", data->xIo.param.modbus.nSyncFAddr);
                data->xIo.param.modbus.btSlaveAddr = cJSON_GetInt(pIO, "sa", data->xIo.param.modbus.btSlaveAddr);
                data->xIo.param.modbus.usExtAddr = cJSON_GetInt(pIO, "ea", data->xIo.param.modbus.usExtAddr);
                data->xIo.param.modbus.usAddrOfs = cJSON_GetInt(pIO, "ao", data->xIo.param.modbus.usAddrOfs);
            } else if (__proto_is_dlt645(data->xIo.usDevType, data->xIo.usProtoType)) {
                const char *addr_str = cJSON_GetString(pIO, "dltad", VAR_NULL);
                if (addr_str && strlen(addr_str)==(2*sizeof(dlt645_addr_t))) {
                    for( int i = 0; i < sizeof(dlt645_addr_t); i++ ) {
                		data->xIo.param.dlt645.addr.addr[5-i] = (HEX_CH_TO_NUM(addr_str[2*i]) << 4) + HEX_CH_TO_NUM(addr_str[2*i+1]);
                	}
                }
                data->xIo.param.dlt645.op = cJSON_GetInt(pIO, "dltop", data->xIo.param.dlt645.op);
            } else if (__proto_is_mbus603(data->xIo.usDevType, data->xIo.usProtoType)) {
                data->xIo.param.mbus603.addr.addr =  (mdBYTE)cJSON_GetInt(pIO, "mbus603ad", 0);
                data->xIo.param.mbus603.op = cJSON_GetInt(pIO, "mbus603op", data->xIo.param.mbus603.op);
            } else if (__proto_is_dust(data->xIo.usDevType, data->xIo.usProtoType)) {
                data->xIo.param.dust.op = cJSON_GetInt(pIO, "dustop", data->xIo.param.dust.op);
            } else if (__proto_is_smf(data->xIo.usDevType, data->xIo.usProtoType)) {
                data->xIo.param.smf.op = cJSON_GetInt(pIO, "smfop", data->xIo.param.smf.op);
            } else if (__proto_is_dh(data->xIo.usDevType, data->xIo.usProtoType)) {
                data->xIo.param.dh.sid = cJSON_GetInt(pIO, "dhsid", data->xIo.param.dh.sid);
                data->xIo.param.dh.type = cJSON_GetInt(pIO, "dhtype", data->xIo.param.dh.type);
                const char *key_str = cJSON_GetString(pIO, "dhkey", VAR_NULL);
                if (key_str && strlen(key_str) < 16) {
                    strcpy(data->xIo.param.dh.key.key, key_str);
                }
            }

            //data->xIo.exp_type = cJSON_GetInt(pIO, "exp_t", data->xIo.exp_type);
            data->xIo.exp_type = cJSON_GetInt(pIO, "exp_t", IO_EXP_TYPE_EXP);

            if (data->xIo.exp_type == IO_EXP_TYPE_EXP) {
                str = cJSON_GetString(pIO, "exp", VAR_NULL);
                if (str) {
                    VAR_MANAGE_FREE(data->xIo.exp.szExp);
                    if (strlen(str) > 0) data->xIo.exp.szExp = rt_strdup(str);
                }
            } else if (data->xIo.exp_type == IO_EXP_TYPE_RULE) {
                cJSON *rule = cJSON_GetObjectItem(pIO, "rule");
                if (rule) {
                    str = cJSON_GetString(rule, "name", VAR_NULL);
                    if (str) {
                        VAR_MANAGE_FREE(data->xIo.exp.rule.name);
                        if (str[0]) data->xIo.exp.rule.name = rt_strdup(str);
                    }
                    str = cJSON_GetString(rule, "p_in", VAR_NULL);
                    if (str) {
                        VAR_MANAGE_FREE(data->xIo.exp.rule.p_in);
                        if (str[0]) data->xIo.exp.rule.p_in = rt_strdup(str);
                    }
                    str = cJSON_GetString(rule, "p_out", VAR_NULL);
                    if (str) {
                        VAR_MANAGE_FREE(data->xIo.exp.rule.p_out);
                        if (str[0]) data->xIo.exp.rule.p_out = rt_strdup(str);
                    }
                }
            }
        }

        if (pStorage) {
            itmp = cJSON_GetInt(pStorage, "se", -1);
            if (itmp >= 0) data->xStorage.bEnable = (itmp != 0 ? VAR_TRUE : VAR_FALSE);
            data->xStorage.ulStep = cJSON_GetInt(pStorage, "ss", data->xStorage.ulStep);
            data->xStorage.btType = cJSON_GetInt(pStorage, "st", data->xStorage.btType);
            itmp = cJSON_GetInt(pStorage, "sc", -1);
            if (itmp >= 0) data->xStorage.bCheck = (itmp != 0 ? VAR_TRUE : VAR_FALSE);
            data->xStorage.fMax = cJSON_GetDouble(pStorage, "sma", data->xStorage.fMax);
            data->xStorage.fMin = cJSON_GetDouble(pStorage, "smi", data->xStorage.fMin);
        }

        if (pAlarm) {
            itmp = cJSON_GetInt(pAlarm, "en", -1);
            if (itmp >= 0) data->xAlarm.bEnable = (itmp != 0 ? VAR_TRUE : VAR_FALSE);
        }

        if (pUp) {
            itmp = cJSON_GetInt(pUp, "en", -1);
            if (itmp >= 0) data->xUp.bEnable = (itmp != 0 ? VAR_TRUE : VAR_FALSE);

            str = cJSON_GetString(pUp, "nid", VAR_NULL);
            if (str) {
                VAR_MANAGE_FREE(data->xUp.szNid);
                if (strlen(str) > 0) data->xUp.szNid = rt_strdup(str);
            }
            str = cJSON_GetString(pUp, "fid", VAR_NULL);
            if (str) {
                VAR_MANAGE_FREE(data->xUp.szFid);
                if (strlen(str) > 0) data->xUp.szFid = rt_strdup(str);
            }
            str = cJSON_GetString(pUp, "unit", VAR_NULL);
            if (str) {
                VAR_MANAGE_FREE(data->xUp.szUnit);
                if (strlen(str) > 0) data->xUp.szUnit = rt_strdup(str);
            }
            
            data->xUp.pi = cJSON_GetInt(pUp, "pi", 4);

            data->xUp.usDevType = cJSON_GetInt(pUp, "dt", data->xUp.usDevType);
            data->xUp.usDevNum = cJSON_GetInt(pUp, "dtn", data->xUp.usDevNum);
            data->xUp.usProtoType = cJSON_GetInt(pUp, "pt", data->xUp.usProtoType);
            str = cJSON_GetString(pUp, "pnm", VAR_NULL);
            if (str) {
                VAR_MANAGE_FREE(data->xUp.szProtoName);
                if (strlen(str) > 0) data->xUp.szProtoName = rt_strdup(str);
            }

            str = cJSON_GetString(pUp, "desc", VAR_NULL);
            if (str) {
                VAR_MANAGE_FREE(data->xUp.szDesc);
                if (strlen(str) > 0) data->xUp.szDesc = rt_strdup(str);
            }
        }

        if (pCfg) {
            if (data->eAttr == E_VAR_ATTR_AI) {
                data->xCfg.ai.range = cJSON_GetInt(pCfg, "range", Range_4_20MA);
                data->xCfg.ai.unit = cJSON_GetInt(pCfg, "unit", Unit_Eng);
                data->xCfg.ai.ext_range_min = cJSON_GetDouble(pCfg, "rmin", 0);
                data->xCfg.ai.ext_range_max = cJSON_GetDouble(pCfg, "rmax", 0);
                data->xCfg.ai.ext_factor = cJSON_GetDouble(pCfg, "fact", 0);
            }
        }
        return VAR_TRUE;
    }

    return VAR_FALSE;
}

void cfgSetVarManageExtDataWithJson(cJSON *pCfg)
{
    ExtData_t *data = VAR_MANAGE_CALLOC(sizeof(ExtData_t), 1);
    if(data) {
        jsonParseExtDataInfo(pCfg, data);
        board_cfg_varext_add(data);
        VAR_MANAGE_FREE(data);
    }
}

void setVarManageExtDataWithJson(cJSON *pCfg)
{
    ExtData_t data_bak;
    int n = cJSON_GetInt(pCfg, "n", -1);
    if (n >= 0) {
        ExtData_t *data = RT_NULL;
        ExtData_t *datacopy = RT_NULL;
        var_bool_t add = RT_FALSE;

        // add by jay 2016/11/25
        // 该过程禁止打断, 不能有任何挂起操作
        rt_enter_critical();
        {
            if (s_xExtDataList.n < n+1) {
                add = RT_TRUE;
                data = VAR_MANAGE_CALLOC(sizeof(ExtData_t), 1);
                if(data) {
                    vVarManage_InsertNode(data);
                }
            } else {
                data = s_xExtDataList.pList;
                while (data && n--) {
                    data = data->next;
                }
            }

            if(data) {
                memcpy(&data_bak, data, sizeof(ExtData_t));
                jsonParseExtDataInfo(pCfg, data);

                if (memcmp(&data_bak, data, sizeof(ExtData_t)) != 0) {
                    ExtData_t *node = s_xExtDataList.pList;

                    while (node) {
                        node->xUp.xAvgUp.count = 0;
                        node->xUp.xAvgUp.val_avg = 0;
                        node->xUp.xAvgUp.val_cur = 0;
                        node->xUp.xAvgUp.time = rt_tick_get();
                        node = node->next;
                    }

                    memset(&data->xStorage.xAvgReal, 0, sizeof(VarAvg_t));
                    data->xStorage.xAvgReal.time = rt_tick_get();
                    memset(&data->xStorage.xAvgMin, 0, sizeof(VarAvg_t));
                    data->xStorage.xAvgMin.time = rt_tick_get();
                    memset(&data->xStorage.xAvgHour, 0, sizeof(VarAvg_t));
                    data->xStorage.xAvgHour.time = rt_tick_get();
                }
                datacopy = pVarManage_CopyData( data );
            }
        }
        rt_exit_critical();
        if( datacopy ) {
            if (add) {
                board_cfg_varext_add(datacopy);
            } else {
                board_cfg_varext_update(data_bak.xName.szName, datacopy);
            }
            vVarManage_FreeData(datacopy);
        }
        __sort_extdata_link();
        __RefreshRegsFlag();
        __creat_sync_slave_link();
        varmanage_update();
    }
}

DEF_CGI_HANDLER(setVarManageExtData)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        setVarManageExtDataWithJson(pCfg);
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}

DEF_CGI_HANDLER(delVarManageExtData)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        const char *name = cJSON_GetString(pCfg, "na", VAR_NULL);
        if (name && strlen(name) > 0) {
            if (board_cfg_varext_del(name)) {
                vVarManage_RemoveNodeWithName(name);
            } else {
                err = RT_ERROR;
            }
            __sort_extdata_link();
            __RefreshRegsFlag();
            __creat_sync_slave_link();
        }
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}

// return str len
static int __getValAsStr( var_uint8_t type, VarValue_v *value, char *str, int size )
{
    if (value && str && size>0) {
        switch (type) {
        case E_VAR_BIT:
            snprintf(str, size, "%u", value->val_bit);
            break;
        case E_VAR_INT8:
            snprintf(str, size, "%d", value->val_i8);
            break;
        case E_VAR_UINT8:
            snprintf(str, size, "%u", value->val_u8);
            break;
        case E_VAR_INT16:
            snprintf(str, size, "%d", value->val_i16);
            break;
        case E_VAR_UINT16:
            snprintf(str, size, "%u", value->val_u16);
            break;
        case E_VAR_INT32:
            snprintf(str, size, "%d", value->val_i32);
            break;
        case E_VAR_UINT32:
            snprintf(str, size, "%u", (uint32_t)value->val_u32);
            break;
        case E_VAR_FLOAT:
            snprintf(str, size, "%.4f", value->val_f);
            break;
        case E_VAR_DOUBLE:
            snprintf(str, size, "%.4f", value->val_db);
            break;
        case E_VAR_ARRAY:
            snprintf(str, size, "%s", "ARRAY");
            break;
        }
        return strlen(str);
    }
    
    return 0;
}

var_bool_t jsonFillExtDataInfo(ExtData_t *data, cJSON *pItem)
{
    if (data && pItem) {
        cJSON *pIO = cJSON_CreateObject();
        cJSON *pAlarm = cJSON_CreateObject();
        cJSON *pStorage = cJSON_CreateObject();
        cJSON *pUp = cJSON_CreateObject();
        cJSON *pCfg = cJSON_CreateObject();
        
        cJSON_AddNumberToObject(pItem, "en", data->bEnable ? 1 : 0);
        cJSON_AddStringToObject(pItem, "gp", data->xGroup.szGroup);
        cJSON_AddStringToObject(pItem, "na", data->xName.szName);
        cJSON_AddStringToObject(pItem, "al", data->xAlias.szAlias);
        cJSON_AddNumberToObject(pItem, "attr", data->eAttr);
        cJSON_AddItemToObject(pItem, "io", pIO);
        cJSON_AddItemToObject(pItem, "alarm", pAlarm);
        cJSON_AddItemToObject(pItem, "storage", pStorage);
        cJSON_AddItemToObject(pItem, "up", pUp);
        cJSON_AddItemToObject(pItem, "cfg", pCfg);

        cJSON_AddNumberToObject(pIO, "eop", data->xIo.btErrOp);
        cJSON_AddNumberToObject(pIO, "ecnt", data->xIo.btErrCnt);
        cJSON_AddNumberToObject(pIO, "rw", data->xIo.btRWType);
        cJSON_AddNumberToObject(pIO, "vt", data->xIo.btInVarType);
        cJSON_AddNumberToObject(pIO, "vs", data->xIo.btInVarSize);
        cJSON_AddNumberToObject(pIO, "ovt", data->xIo.btOutVarType);
        cJSON_AddNumberToObject(pIO, "ovs", data->xIo.btOutVarSize);
        cJSON_AddNumberToObject(pIO, "vrl", data->xIo.btInVarRuleType);
        cJSON_AddNumberToObject(pIO, "ovrl", data->xIo.btOutVarRuleType);
        {
            char szStr[24] = { 0 };
            __getValAsStr(data->xIo.btOutVarType, &data->xIo.xValue, szStr, sizeof(szStr));
            if (szStr[0]) {
                cJSON_AddStringToObject(pIO, "va", szStr);
            }
        }

        if (data->xIo.bUseMax) {
            cJSON_AddNumberToObject(pIO, "vma", data->xIo.fMax);
        }
        if (data->xIo.bUseMin) {
            cJSON_AddNumberToObject(pIO, "vmi", data->xIo.fMin);
        }
        if (data->xIo.bUseInit) {
            cJSON_AddNumberToObject(pIO, "vii", data->xIo.fInit);
        }
        if (data->xIo.bUseRatio) {
            cJSON_AddNumberToObject(pIO, "vrt", data->xIo.fRatio);
        }

        cJSON_AddNumberToObject(pIO, "dt", data->xIo.usDevType);
        cJSON_AddNumberToObject(pIO, "dtn", data->xIo.usDevNum);
        cJSON_AddNumberToObject(pIO, "pt", data->xIo.usProtoType);
        cJSON_AddStringToObject(pIO, "pnm", data->xIo.szProtoName ? data->xIo.szProtoName : "");

        cJSON_AddNumberToObject(pIO, "ad", data->xIo.usAddr);
        
        if (__proto_is_modbus(data->xIo.usDevType, data->xIo.usProtoType) ||
            __proto_is_obmodbus(data->xIo.usDevType, data->xIo.usProtoType)) {
            cJSON_AddNumberToObject(pIO, "mbop", data->xIo.param.modbus.btOpCode);
            cJSON_AddNumberToObject(pIO, "sa", data->xIo.param.modbus.btSlaveAddr);
            cJSON_AddNumberToObject(pIO, "sfa", data->xIo.param.modbus.nSyncFAddr);
            cJSON_AddNumberToObject(pIO, "ea", data->xIo.param.modbus.usExtAddr);
            cJSON_AddNumberToObject(pIO, "ao", data->xIo.param.modbus.usAddrOfs);
        } else if (__proto_is_dlt645(data->xIo.usDevType, data->xIo.usProtoType)) {
            {
                char szStr[24] = {0};
                var_uint8_t *addr = data->xIo.param.dlt645.addr.addr;
                sprintf(szStr, "%02X%02X%02X%02X%02X%02X", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
                cJSON_AddStringToObject(pIO, "dltad", szStr);
            }
            cJSON_AddNumberToObject(pIO, "dltop", data->xIo.param.dlt645.op);
        } else if (__proto_is_mbus603(data->xIo.usDevType, data->xIo.usProtoType)) {
            cJSON_AddNumberToObject(pIO, "mbus603ad", data->xIo.param.mbus603.addr.addr);
            cJSON_AddNumberToObject(pIO, "mbus603op", data->xIo.param.mbus603.op);
        } else if (__proto_is_dust(data->xIo.usDevType, data->xIo.usProtoType)) {
            cJSON_AddNumberToObject(pIO, "dustop", data->xIo.param.dust.op);
        } else if (__proto_is_smf(data->xIo.usDevType, data->xIo.usProtoType)) {
            cJSON_AddNumberToObject(pIO, "smfop", data->xIo.param.smf.op);
        } else if (__proto_is_dh(data->xIo.usDevType, data->xIo.usProtoType)) {
            cJSON_AddNumberToObject(pIO, "dhsid", data->xIo.param.dh.sid);
            cJSON_AddNumberToObject(pIO, "dhtype", data->xIo.param.dh.type);
            cJSON_AddStringToObject(pIO, "dhkey", data->xIo.param.dh.key.key);
        }
        
        cJSON_AddNumberToObject(pIO, "exp_t", data->xIo.exp_type);
        if (data->xIo.exp_type == IO_EXP_TYPE_EXP) {
            cJSON_AddStringToObject(pIO, "exp", data->xIo.exp.szExp ? data->xIo.exp.szExp : "");
        } else if (data->xIo.exp_type == IO_EXP_TYPE_RULE) {
            cJSON *rule = cJSON_CreateObject();
            if (rule) {
                cJSON_AddItemToObject(pIO, "rule", rule);
                cJSON_AddStringToObject(rule, "name", data->xIo.exp.rule.name ? data->xIo.exp.rule.name : "");
                cJSON_AddStringToObject(rule, "p_in", data->xIo.exp.rule.p_in ? data->xIo.exp.rule.p_in : "");
                cJSON_AddStringToObject(rule, "p_out", data->xIo.exp.rule.p_out ? data->xIo.exp.rule.p_out : "");
            }
        }

        cJSON_AddNumberToObject(pStorage, "se", data->xStorage.bEnable ? 1 : 0);
        cJSON_AddNumberToObject(pStorage, "ss", data->xStorage.ulStep);
        cJSON_AddNumberToObject(pStorage, "st", data->xStorage.btType);
        cJSON_AddNumberToObject(pStorage, "sc", data->xStorage.bCheck);
        cJSON_AddNumberToObject(pStorage, "sma", data->xStorage.fMax);
        cJSON_AddNumberToObject(pStorage, "smi", data->xStorage.fMin);

        cJSON_AddNumberToObject(pAlarm, "en", data->xAlarm.bEnable);

        cJSON_AddNumberToObject(pUp, "en", data->xUp.bEnable ? 1 : 0);

        cJSON_AddStringToObject(pUp, "nid", data->xUp.szNid ? data->xUp.szNid : "");
        cJSON_AddStringToObject(pUp, "fid", data->xUp.szFid ? data->xUp.szFid : "");
        cJSON_AddStringToObject(pUp, "unit", data->xUp.szUnit ? data->xUp.szUnit : "");
        
        cJSON_AddNumberToObject(pUp, "pi", data->xUp.pi);

        cJSON_AddNumberToObject(pUp, "dt", data->xUp.usDevType);
        cJSON_AddNumberToObject(pUp, "dtn", data->xUp.usDevNum);
        cJSON_AddNumberToObject(pUp, "pt", data->xUp.usProtoType);
        cJSON_AddStringToObject(pUp, "pnm", data->xUp.szProtoName ? data->xUp.szProtoName : "");

        cJSON_AddStringToObject(pUp, "desc", data->xUp.szDesc ? data->xUp.szDesc : "");

        if (data->eAttr == E_VAR_ATTR_AI) {
            cJSON_AddNumberToObject(pCfg, "range", data->xCfg.ai.range);
            cJSON_AddNumberToObject(pCfg, "unit", data->xCfg.ai.unit);
            cJSON_AddNumberToObject(pCfg, "rmin", data->xCfg.ai.ext_range_min);
            cJSON_AddNumberToObject(pCfg, "rmax", data->xCfg.ai.ext_range_max);
            cJSON_AddNumberToObject(pCfg, "fact", data->xCfg.ai.ext_factor);
        }

        return VAR_TRUE;
    }

    return VAR_FALSE;
}

int nExtDataListCnt(void)
{
    return s_xExtDataList.n;
}

void jsonFillExtDataInfoWithNum(int n, cJSON *pItem)
{
    if(pItem && n >= 0 && n < s_xExtDataList.n) {
        ExtData_t *data = __GetExtDataWithIndex(n);
        if(data) {
            cJSON_AddNumberToObject(pItem, "n", n);
            jsonFillExtDataInfo(data, pItem);
            vVarManage_FreeData(data);
        }
    }
}

DEF_CGI_HANDLER(getVarManageExtDataVals)
{
    char *szRetJSON = RT_NULL;
    // add by jay 2016/11/25
    // 该过程禁止打断, 不能有任何挂起操作
    rt_enter_critical();
    {
        rt_bool_t first = RT_TRUE;
        int len = 32;
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            char str[24];
            len += __getValAsStr(node->xIo.btOutVarType, &node->xIo.xValue, str, sizeof(str) );
            len+=3; // add ',', '"', '"'
            node = node->next;
        }
        szRetJSON = VAR_MANAGE_CALLOC(1, len);
        if (szRetJSON) {
            ExtData_t *node = s_xExtDataList.pList;
            char *p = szRetJSON;
            p += sprintf(p, "%s", "{\"ret\":0,\"list\":[");
            while (node) {
                char str[24] = {0};
                __getValAsStr(node->xIo.btOutVarType, &node->xIo.xValue, str, sizeof(str));
                if (!first) *p++ = ',';
                first = RT_FALSE;
                *p++ = '\"';
                p += sprintf(p, "%s", str);
                *p++ = '\"';
                node = node->next;
            }
            p += sprintf(p, "%s", "]}");
        }
    }
    rt_exit_critical();
    if( szRetJSON ) {
        WEBS_PRINTF(szRetJSON);
        rt_free(szRetJSON);
    } else {
        WEBS_PRINTF("{\"ret\":1}");
    }
    WEBS_DONE(200);
}

DEF_CGI_HANDLER(getVarManageExtData)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    char *szRetJSON = RT_NULL;
    rt_bool_t first = RT_TRUE;

    if (pCfg) {
        int all = cJSON_GetInt(pCfg, "all", 0);
        if (all) {
            first = RT_TRUE;
            WEBS_PRINTF("{\"ret\":0,\"list\":[");
            int num = s_xExtDataList.n;
            for( int i = 0; i < num; i++ ) {
                ExtData_t *node = __GetExtDataWithIndex(i);
                cJSON *pItem = cJSON_CreateObject();
                if(pItem && node) {
                    if (!first) WEBS_PRINTF(",");
                    first = RT_FALSE;
                    jsonFillExtDataInfo(node, pItem);
                    szRetJSON = cJSON_PrintUnformatted(pItem);
                    if(szRetJSON) {
                        WEBS_PRINTF(szRetJSON);
                        rt_free(szRetJSON);
                    }
                    vVarManage_FreeData(node);
                }
                cJSON_Delete(pItem);
            }

            // add matser protolist
            WEBS_PRINTF("],\"protolist\":{");

            // rs proto
            WEBS_PRINTF("\"rs\":[");
            first = RT_TRUE;
            for (int n = 0; n < 2; n++) {
                rt_int8_t instance = nUartGetInstance(n);
                if (instance >= 0 && !g_xfer_net_dst_uart_occ[instance]) {
                    if (
                            PROTO_DLT645 == g_uart_cfgs[instance].proto_type || 
                            PROTO_DLT645_1997 == g_uart_cfgs[instance].proto_type || 
                            PROTO_DUST == g_uart_cfgs[instance].proto_type || 
                            PROTO_MBUS603 == g_uart_cfgs[instance].proto_type
                        ) 
                    {
                        g_uart_cfgs[instance].proto_ms = PROTO_MASTER;
                    }
                    if (PROTO_MASTER == g_uart_cfgs[instance].proto_ms) {
                        cJSON *pItem = cJSON_CreateObject();
                        if(pItem) {
                            if (!first) WEBS_PRINTF(",");
                            first = RT_FALSE;
                            cJSON_AddNumberToObject(pItem, "id", PROTO_DEV_RS1 + n);
                            cJSON_AddNumberToObject(pItem, "idn", 0);
                            cJSON_AddNumberToObject(pItem, "po", g_uart_cfgs[instance].proto_type);
                            char *szRetJSON = cJSON_PrintUnformatted(pItem);
                            if(szRetJSON) {
                                WEBS_PRINTF(szRetJSON);
                                rt_free(szRetJSON);
                            }
                        }
                        cJSON_Delete(pItem);
                    }
                }
            }
            WEBS_PRINTF("],");

            // net proto
            first = RT_TRUE;
            WEBS_PRINTF("\"net\":[");
            for (int n = 0; n < BOARD_ENET_TCPIP_NUM; n++) {
                if (g_tcpip_cfgs[n].enable && NET_IS_NORMAL(n) && PROTO_MASTER == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                    cJSON *pItem = cJSON_CreateObject();
                    if(pItem) {
                        if (!first) WEBS_PRINTF(",");
                        first = RT_FALSE;
                        cJSON_AddNumberToObject(pItem, "id", PROTO_DEV_NET);
                        cJSON_AddNumberToObject(pItem, "idn", n);
                        cJSON_AddNumberToObject(pItem, "po", g_tcpip_cfgs[n].cfg.normal.proto_type);
                        char *szRetJSON = cJSON_PrintUnformatted(pItem);
                        if(szRetJSON) {
                            WEBS_PRINTF(szRetJSON);
                            rt_free(szRetJSON);
                        }
                    }
                    cJSON_Delete(pItem);
                }
            }
            WEBS_PRINTF("],");

            // zigbee proto
            first = RT_TRUE;
            WEBS_PRINTF("\"zigbee\":[");
            if (ZIGBEE_WORK_COORDINATOR == g_zigbee_cfg.xInfo.WorkMode && ZGB_TM_GW == g_zigbee_cfg.tmode) {
                cJSON *pItem = cJSON_CreateObject();
                if(pItem) {
                    if (!first) WEBS_PRINTF(",");
                    first = RT_FALSE;
                    cJSON_AddNumberToObject(pItem, "id", PROTO_DEV_ZIGBEE);
                    cJSON_AddNumberToObject(pItem, "idn", 0);
                    cJSON_AddNumberToObject(pItem, "po", PROTO_MODBUS_RTU);
                    char *szRetJSON = cJSON_PrintUnformatted(pItem);
                    if(szRetJSON) {
                        WEBS_PRINTF(szRetJSON);
                        rt_free(szRetJSON);
                    }
                }
                cJSON_Delete(pItem);
            }
            WEBS_PRINTF("],");
            
            // lora proto
            first = RT_TRUE;
            WEBS_PRINTF("\"lora\":[");
            if (LORA_WORK_CENTRAL == g_lora_cfg.work_mode && LORA_TM_GW == g_lora_cfg.tmode) {
                cJSON *pItem = cJSON_CreateObject();
                if(pItem) {
                    if (!first) WEBS_PRINTF(",");
                    first = RT_FALSE;
                    cJSON_AddNumberToObject(pItem, "id", PROTO_DEV_LORA);
                    cJSON_AddNumberToObject(pItem, "idn", 0);
                    cJSON_AddNumberToObject(pItem, "po", PROTO_MODBUS_RTU);
                    char *szRetJSON = cJSON_PrintUnformatted(pItem);
                    if(szRetJSON) {
                        WEBS_PRINTF(szRetJSON);
                        rt_free(szRetJSON);
                    }
                }
                cJSON_Delete(pItem);
            }
            WEBS_PRINTF("],");

            // gprs proto
            first = RT_TRUE;
            WEBS_PRINTF("\"gprs\":[");
            for (int n = BOARD_ENET_TCPIP_NUM; n < BOARD_TCPIP_MAX; n++) {
                if(NET_IS_NORMAL(n) && PROTO_MASTER == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                    cJSON *pItem = cJSON_CreateObject();
                    if(pItem) {
                        if (!first) WEBS_PRINTF(",");
                        first = RT_FALSE;
                        cJSON_AddNumberToObject(pItem, "id", PROTO_DEV_GPRS);
                        cJSON_AddNumberToObject(pItem, "idn", n - BOARD_ENET_TCPIP_NUM);
                        cJSON_AddNumberToObject(pItem, "po", g_tcpip_cfgs[n].cfg.normal.proto_type);
                        char *szRetJSON = cJSON_PrintUnformatted(pItem);
                        if(szRetJSON) {
                            WEBS_PRINTF(szRetJSON);
                            rt_free(szRetJSON);
                        }
                    }
                    cJSON_Delete(pItem);
                }
            }

            // add upload protolist
            WEBS_PRINTF("]},\"upprotolist\":{");

            // rs proto
            WEBS_PRINTF("\"rs\":[");
            WEBS_PRINTF("],");

            // net proto
            first = RT_TRUE;
            WEBS_PRINTF("\"net\":[");
            for (int n = 0; n < BOARD_ENET_TCPIP_NUM; n++) {
                if(g_tcpip_cfgs[n].enable && NET_IS_NORMAL(n) && 
                  (
                    PROTO_CC_BJDC == g_tcpip_cfgs[n].cfg.normal.proto_type || 
                    PROTO_HJT212 == g_tcpip_cfgs[n].cfg.normal.proto_type || 
                    PROTO_DM101 == g_tcpip_cfgs[n].cfg.normal.proto_type || 
                    PROTO_MQTT == g_tcpip_cfgs[n].cfg.normal.proto_type
                  )) {
                    cJSON *pItem = cJSON_CreateObject();
                    if(pItem) {
                        if (!first) WEBS_PRINTF(",");
                        first = RT_FALSE;
                        cJSON_AddNumberToObject(pItem, "id", PROTO_DEV_NET);
                        cJSON_AddNumberToObject(pItem, "idn", n);
                        cJSON_AddNumberToObject(pItem, "po", g_tcpip_cfgs[n].cfg.normal.proto_type);
                        char *szRetJSON = cJSON_PrintUnformatted(pItem);
                        if(szRetJSON) {
                            WEBS_PRINTF(szRetJSON);
                            rt_free(szRetJSON);
                        }
                    }
                    cJSON_Delete(pItem);
                }
            }
            WEBS_PRINTF("],");

            // zigbee proto
            first = RT_TRUE;
            WEBS_PRINTF("\"zigbee\":[");
            WEBS_PRINTF("],");

            // gprs proto
            first = RT_TRUE;
            WEBS_PRINTF("\"gprs\":[");
            for (int n = BOARD_ENET_TCPIP_NUM; n < BOARD_TCPIP_MAX; n++) {
                if(g_tcpip_cfgs[n].enable && NET_IS_NORMAL(n) && 
                  (
                    PROTO_CC_BJDC == g_tcpip_cfgs[n].cfg.normal.proto_type || 
                    PROTO_HJT212 == g_tcpip_cfgs[n].cfg.normal.proto_type || 
                    PROTO_DM101 == g_tcpip_cfgs[n].cfg.normal.proto_type || 
                    PROTO_MQTT == g_tcpip_cfgs[n].cfg.normal.proto_type
                  )) {
                    cJSON *pItem = cJSON_CreateObject();
                    if(pItem) {
                        if (!first) WEBS_PRINTF(",");
                        first = RT_FALSE;
                        cJSON_AddNumberToObject(pItem, "id", PROTO_DEV_GPRS);
                        cJSON_AddNumberToObject(pItem, "idn", n - BOARD_ENET_TCPIP_NUM);
                        cJSON_AddNumberToObject(pItem, "po", g_tcpip_cfgs[n].cfg.normal.proto_type);
                        char *szRetJSON = cJSON_PrintUnformatted(pItem);
                        if(szRetJSON) {
                            WEBS_PRINTF(szRetJSON);
                            rt_free(szRetJSON);
                        }
                    }
                    cJSON_Delete(pItem);
                }
            }
            WEBS_PRINTF("]}}");        // }} end array and end json

        } else {
            int n = cJSON_GetInt(pCfg, "n", -1);
            if (n >= 0 && n < VAR_EXT_DATA_SIZE) {
                ExtData_t *data = __GetExtDataWithIndex(n);
                if (data) {
                    cJSON *pItem = cJSON_CreateObject();
                    if(pItem) {
                        cJSON_AddNumberToObject(pItem, "ret", RT_EOK);
                        jsonFillExtDataInfo(data, pItem);
                        szRetJSON = cJSON_PrintUnformatted(pItem);
                        if(szRetJSON) {
                            WEBS_PRINTF(szRetJSON);
                            rt_free(szRetJSON);
                        }
                    }
                    cJSON_Delete(pItem);
                    vVarManage_FreeData(data);
                }
            }
        }
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    if (err != RT_EOK) {
        WEBS_PRINTF("{\"ret\":%d}", err);
    }
    WEBS_DONE(200);
}

DEF_CGI_HANDLER(delVarManageExtGroup)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        const char *group = cJSON_GetString(pCfg, "gp", VAR_NULL);
        if (group && strlen(group) > 0) {
            int refresh_flag = 0;
            while (1) {
                VarName_t name = {.szName = {0}};
                rt_enter_critical();
                {
                    if (s_xExtDataList.pList) {
                        ExtData_t *node = s_xExtDataList.pList;
                        while (node) {
                            if (0 == strcmp(group, node->xGroup.szGroup)) {
                                name = node->xName;
                                break;
                            }
                            node = node->next;
                        }
                    }
                }
                rt_exit_critical();
                if (name.szName[0] && board_cfg_varext_del(name.szName)) {
                    vVarManage_RemoveNodeWithName(name.szName);
                    refresh_flag = 1;
                } else {
                    break;
                }
            }

            if (refresh_flag) {
                __sort_extdata_link();
                __RefreshRegsFlag();
                __creat_sync_slave_link();
            }
        }
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}

static int __var_has_group(const char *group)
{
    int has_flag = 0;
    rt_enter_critical();
    {
        if (s_xExtDataList.pList) {
            ExtData_t *node = s_xExtDataList.pList;
            while (node) {
                if (0 == strcmp(group, node->xGroup.szGroup)) {
                    has_flag = 1;
                    break;
                }
                node = node->next;
            }
        }
    }
    rt_exit_critical();
    return has_flag;
}

static int __var_has_name(const char *name)
{
    int has_flag = 0;
    rt_enter_critical();
    {
        if (s_xExtDataList.pList) {
            ExtData_t *node = s_xExtDataList.pList;
            while (node) {
                if (0 == strcmp(name, node->xName.szName)) {
                    has_flag = 1;
                    break;
                }
                node = node->next;
            }
        }
    }
    rt_exit_critical();
    return has_flag;
}

static var_uint16_t __var_get_next_addr(void)
{
    var_uint16_t max_addr = USER_REG_EXT_DATA_START;
    var_uint16_t sz = 1;
    rt_enter_critical();
    {
        if (s_xExtDataList.pList) {
            ExtData_t *node = s_xExtDataList.pList;
            while (node) {
                if (node->xIo.usAddr > max_addr) {
                    max_addr = node->xIo.usAddr;
                    sz = node->xIo.btOutVarSize;
                }
                node = node->next;
            }
        }
    }
    rt_exit_critical();
    return max_addr + ((sz + 1) / 2);
}

static int __var_add_ai(const char *id, const char *prefix, int slave_addr, int dev_type, int dev_num)
{
    VarGroup_t group = {.szGroup = {0}};
    snprintf(group.szGroup, sizeof(VarGroup_t), "AI_%s", id);
    if (!__var_has_group((const char *)group.szGroup)) {
        for (int n = 0; n < MB_DEV_AI_DATA_REG_SZ; n++) {
            char name[32];
            snprintf(name, sizeof(name), "%s_AI_%d", prefix, n + 1);
            if (__var_has_name(name)) return WEBNET_ERR_NAME_EXIST;
        }
        var_uint16_t ext_addr = MB_DEV_AI_DATA_REG;
        var_uint16_t addr = __var_get_next_addr();
        for (int n = 0; n < MB_DEV_AI_DATA_REG_SZ; n++) {
            ExtData_t *data = VAR_MANAGE_CALLOC(sizeof(ExtData_t), 1);
            if (data) {
                data->bEnable = VAR_TRUE;
                data->xGroup = group;
                data->eAttr = E_VAR_ATTR_AI;
                snprintf(data->xName.szName, sizeof(VarName_t), "%s_AI_%d", prefix, n + 1);
                data->xIo.btInVarType = E_VAR_FLOAT;
                data->xIo.btInVarSize = g_var_type_sz[E_VAR_FLOAT];
                data->xIo.btOutVarType = E_VAR_FLOAT;
                data->xIo.btOutVarSize = g_var_type_sz[E_VAR_FLOAT];
                data->xIo.usDevType = dev_type;
                data->xIo.usDevNum = dev_num;
                data->xIo.usProtoType = PROTO_MODBUS_RTU;
                data->xIo.usAddr = addr;
                data->xIo.param.modbus.btOpCode = MODBUS_FC_READ_HOLDING_REGISTERS;
                data->xIo.param.modbus.btSlaveAddr = slave_addr;
                data->xIo.param.modbus.usExtAddr = ext_addr;
                /*data->xCfg.ai.range = Range_4_20MA;
                data->xCfg.ai.unit = Unit_Eng;
                data->xCfg.ai.ext_range_min = 0;
                data->xCfg.ai.ext_range_max = 0;*/
                data->xCfg.ai.range = Range_0_20MA;
                data->xCfg.ai.unit = Unit_Meter;
                data->xCfg.ai.ext_range_min = 0.000;
                data->xCfg.ai.ext_range_max = 20.000;
                data->xCfg.ai.ext_factor = 0;
                ext_addr += MB_DEV_AI_DATA_SZ;
                addr += MB_DEV_AI_DATA_SZ;
                vVarManage_InsertNode(data);
                {
                    ExtData_t *datacopy = pVarManage_CopyData(data);
                    if (datacopy) {
                        board_cfg_varext_add(datacopy);
                        vVarManage_FreeData(datacopy);
                    }
                }
            } else {
                return WEBNET_ERR_NO_MEM;
            }
        }
        __sort_extdata_link();
        __RefreshRegsFlag();
        __creat_sync_slave_link();
        varmanage_update();
        return 0;
    } else {
        return WEBNET_ERR_GROUP_EXIST;
    }
}

static int __var_add_ttl(int dev_type_1, const char *id, const char *prefix, int slave_addr, int dev_type, int dev_num)
{
    VarGroup_t group = {.szGroup = {0}};
    snprintf(group.szGroup, sizeof(VarGroup_t), "TTL_%d_%s", VAR_DEV_TYPE1_TTL_4DI_4DO, id);
    if (!__var_has_group((const char *)group.szGroup)) {
        for (int n = 0; n < MB_DEV_TTL_DATA_REG_SZ; n++) {
            char name[32];
            switch (dev_type_1) {
            case VAR_DEV_TYPE1_TTL_4DI_4DO:
                if (n < 4) {
                    snprintf(name, sizeof(name), "%s_TTL_DI_%d", prefix, n + 1);
                } else {
                    snprintf(name, sizeof(name), "%s_TTL_DO_%d", prefix, n - 3);
                }
                break;
            case VAR_DEV_TYPE1_TTL_4DO_4DI:
                if (n < 4) {
                    snprintf(name, sizeof(name), "%s_TTL_DO_%d", prefix, n + 1);
                } else {
                    snprintf(name, sizeof(name), "%s_TTL_DI_%d", prefix, n - 3);
                }
                break;
            case VAR_DEV_TYPE1_TTL_8DI:
                snprintf(name, sizeof(name), "%s_TTL_DI_%d", prefix, n + 1);
                break;
            case VAR_DEV_TYPE1_TTL_8DO:
                snprintf(name, sizeof(name), "%s_TTL_DO_%d", prefix, n + 1);
                break;
            default: return WEBNET_ERR_PARAM;
            }
            if (__var_has_name(name)) return WEBNET_ERR_NAME_EXIST;
        }
    
        var_uint16_t ext_addr = MB_DEV_TTL_DATA_REG;
        var_uint16_t addr = __var_get_next_addr();
        for (int n = 0; n < MB_DEV_TTL_DATA_REG_SZ; n++) {
            ExtData_t *data = VAR_MANAGE_CALLOC(sizeof(ExtData_t), 1);
            if (data) {
                data->bEnable = VAR_TRUE;
                data->xGroup = group;
                switch (dev_type_1) {
                case VAR_DEV_TYPE1_TTL_4DI_4DO:
                    if (n < 4) {
                        data->eAttr = E_VAR_ATTR_DI;
                        snprintf(data->xName.szName, sizeof(VarName_t), "%s_TTL_DI_%d", prefix, n + 1);
                    } else {
                        data->eAttr = E_VAR_ATTR_DO;
                        snprintf(data->xName.szName, sizeof(VarName_t), "%s_TTL_DO_%d", prefix, n - 3);
                    }
                    break;
                case VAR_DEV_TYPE1_TTL_4DO_4DI:
                    if (n < 4) {
                        data->eAttr = E_VAR_ATTR_DO;
                        snprintf(data->xName.szName, sizeof(VarName_t), "%s_TTL_DO_%d", prefix, n + 1);
                    } else {
                        data->eAttr = E_VAR_ATTR_DI;
                        snprintf(data->xName.szName, sizeof(VarName_t), "%s_TTL_DI_%d", prefix, n - 3);
                    }
                    break;
                case VAR_DEV_TYPE1_TTL_8DI:
                    data->eAttr = E_VAR_ATTR_DI;
                    snprintf(data->xName.szName, sizeof(VarName_t), "%s_TTL_DI_%d", prefix, n);
                    break;
                case VAR_DEV_TYPE1_TTL_8DO:
                    data->eAttr = E_VAR_ATTR_DO;
                    snprintf(data->xName.szName, sizeof(VarName_t), "%s_TTL_DO_%d", prefix, n);
                    break;
                default: return WEBNET_ERR_PARAM;
                }
                data->xIo.btInVarType = E_VAR_UINT16;
                data->xIo.btInVarSize = g_var_type_sz[E_VAR_UINT16];
                data->xIo.btOutVarType = E_VAR_UINT16;
                data->xIo.btOutVarSize = g_var_type_sz[E_VAR_UINT16];
                data->xIo.usDevType = dev_type;
                data->xIo.usDevNum = dev_num;
                data->xIo.usProtoType = PROTO_MODBUS_RTU;
                data->xIo.usAddr = addr;
                data->xIo.param.modbus.btOpCode = MODBUS_FC_READ_HOLDING_REGISTERS;
                data->xIo.param.modbus.btSlaveAddr = slave_addr;
                data->xIo.param.modbus.usExtAddr = ext_addr;
                ext_addr += MB_DEV_TTL_DATA_SZ;
                addr += MB_DEV_TTL_DATA_SZ;
                vVarManage_InsertNode(data);
                {
                    ExtData_t *datacopy = pVarManage_CopyData(data);
                    if (datacopy) {
                        board_cfg_varext_add(datacopy);
                        vVarManage_FreeData(datacopy);
                    }
                }
            } else {
                return WEBNET_ERR_NO_MEM;
            }
        }
        __sort_extdata_link();
        __RefreshRegsFlag();
        __creat_sync_slave_link();
        varmanage_update();
        return 0;
    } else {
        return WEBNET_ERR_GROUP_EXIST;
    }
}

// 0: 正常, -1001: 重复添加组, -1002: 重复变量名(需修改前缀), -1003: 内存不足, -1004: 参数错误
static int __var_add_group(int dev_type_0, int dev_type_1, const char *id, const char *prefix, int slave_addr, int dev_type, int dev_num)
{
    VarGroup_t group = {.szGroup = {0}};
    switch (dev_type_0) {
    case VAR_DEV_TYPE0_AI: return __var_add_ai(id, prefix, slave_addr, dev_type, dev_num);
    case VAR_DEV_TYPE0_TTL: return __var_add_ttl(dev_type_1, id, prefix, slave_addr, dev_type, dev_num);
    default: return WEBNET_ERR_PARAM;
    }
}

DEF_CGI_HANDLER(addVarManageExtGroup)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        int dev_type_0 = cJSON_GetInt(pCfg, "dtp0", -1);
        int dev_type_1 = cJSON_GetInt(pCfg, "dtp1", -1);
        const char *sn = cJSON_GetString(pCfg, "sn", VAR_NULL);
        int slave_addr = cJSON_GetInt(pCfg, "sa", -1);
        int dev_type = cJSON_GetInt(pCfg, "dt", -1);
        int dev_num = cJSON_GetInt(pCfg, "dtn", -1);
        const char *prefix = cJSON_GetString(pCfg, "prefix", VAR_NULL);
        if (sn && strlen(sn) > 0 && prefix && strlen(prefix) > 0) {
            err = __var_add_group(dev_type_0, dev_type_1, sn, prefix, slave_addr, dev_type, dev_num);
        } else {
            err = WEBNET_ERR_PARAM;
        }
    } else {
        err = WEBNET_ERR_PARAM;
    }
    cJSON_Delete(pCfg);

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}

void vBigEdianDevType(DevType_t *in)
{
    for (int n = 0; n < 6; n++) {
        in->usIdent[n] = var_htons(in->usIdent[n]);
    }
    in->usType0 = var_htons(in->usType0);
    in->usType1 = var_htons(in->usType1);
    in->usModel = var_htons(in->usModel);
}

static rt_thread_t group_search_thread = RT_NULL;

static group_node_t group_node_list[255];

static void __do_search_group(void *arg)
{
    int dev_type = group_search_dev_type;
    int dev_num = group_search_dev_num;
    group_search_flag = 1;
    memset(group_node_list, 0, sizeof(group_node_list));
    for (group_search_slave = 1; group_search_slave <= 247; group_search_slave++) {
        uint16_t regs[128] = {0};
        int rc = modbus_read_registers_quick_with_uart(
                                                    nUartGetInstance(dev_type), group_search_slave, 
                                                    MB_DEV_IDENT_REG, (MB_DEV_IDENT_REG_SZ + MB_DEV_TYPE_REG_SZ), 
                                                    regs, 50000);
        if (rc >= 0) {
            if (regs[0] == VAR_DEV_IDENT0_REV && 
                regs[1] == VAR_DEV_IDENT1_REV && 
                regs[2] == VAR_DEV_IDENT2_REV && 
                regs[3] == VAR_DEV_IDENT3_REV && 
                regs[4] == VAR_DEV_IDENT4_REV && 
                regs[5] == VAR_DEV_IDENT5_REV) {

                vBigEdianDevType((DevType_t *)regs);
                
                int dev_type_0 = regs[MB_DEV_TYPE_REG - MB_DEV_IDENT_REG];
                int dev_type_1 = regs[MB_DEV_TYPE_REG - MB_DEV_IDENT_REG + 1];
                char sn[32];

                rc = modbus_read_registers_quick_with_uart(
                                                        nUartGetInstance(dev_type), group_search_slave, 
                                                        MB_DEV_SN_REG, MB_DEV_SN_REG_SZ, 
                                                        (uint16_t *)sn, 50000);
                if (rc >= 0) {
                    group_node_list[group_search_slave].use = 1;
                    group_node_list[group_search_slave].dev_type = dev_type;
                    group_node_list[group_search_slave].dev_num = dev_num;
                    group_node_list[group_search_slave].dev_type_0 = dev_type_0;
                    group_node_list[group_search_slave].dev_type_1 = dev_type_1;
                    memcpy(group_node_list[group_search_slave].sn, sn, sizeof(group_node_list[group_search_slave].sn) - 1);
                }
            }
        }
    }
    group_search_flag = 0;
}

void do_search_group(int dev_type, int dev_num)
{
    if (group_search_thread) rt_thread_delete(group_search_thread);
    group_search_thread = rt_thread_create("search_group", __do_search_group, RT_NULL, 0x300, 20, 20);
    if (group_search_thread) {
        group_search_dev_type = dev_type;
        group_search_dev_num = dev_num;
        rt_thread_startup(group_search_thread);
    }
}

DEF_CGI_HANDLER(searchVarManageExtGroupStart)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    
    if (pCfg) {
        int dev_type = cJSON_GetInt(pCfg, "dt", -1);
        int dev_num = cJSON_GetInt(pCfg, "dtn", -1);
        do_search_group(dev_type, dev_num);
        WEBS_PRINTF("{\"ret\":0}");
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    WEBS_DONE(200);
}

#if 0
DEF_CGI_HANDLER(searchVarManageExtGroupStatus)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    char *szRetJSON = RT_NULL;
    rt_bool_t first = RT_TRUE;
    
    if (pCfg) {
        int dev_type = cJSON_GetInt(pCfg, "dt", -1);
        int dev_num = cJSON_GetInt(pCfg, "dtn", -1);

        first = RT_TRUE;
        WEBS_PRINTF("{\"ret\":0,\"pos\":%d,\"list\":[", group_search_slave);
        
        for (int slave = 1; slave < 247; slave++) {
            if (group_node_list[slave].use && 
                group_node_list[slave].dev_type == dev_type && 
                group_node_list[slave].dev_num == dev_num) {
                cJSON *pItem = cJSON_CreateObject();
                if(pItem) {
                    if (!first) WEBS_PRINTF(",");
                    first = RT_FALSE;
                    cJSON_AddStringToObject(pItem, "sn", group_node_list[slave].sn);
                    cJSON_AddNumberToObject(pItem, "dtp0", group_node_list[slave].dev_type_0);
                    cJSON_AddNumberToObject(pItem, "dtp1", group_node_list[slave].dev_type_1);
                    cJSON_AddNumberToObject(pItem, "sa", slave);
                    szRetJSON = cJSON_PrintUnformatted(pItem);
                    if(szRetJSON) {
                        WEBS_PRINTF(szRetJSON);
                        rt_free(szRetJSON);
                    }
                }
                cJSON_Delete(pItem);
            }
        }
        WEBS_PRINTF("]}");
    } else {
        err = RT_ERROR;
        WEBS_PRINTF("{\"ret\":%d}", err);
    }
    cJSON_Delete(pCfg);
    WEBS_DONE(200);
}
#endif

DEF_CGI_HANDLER(searchVarManageExtGroupStatus)
{
    WEBS_PRINTF("{\"ret\":0,\"pos\":%d}", group_search_slave);
    WEBS_DONE(200);
}

DEF_CGI_HANDLER(searchVarManageExtGroupResult)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    char *szRetJSON = RT_NULL;
    rt_bool_t first = RT_TRUE;
    
    if (pCfg) {
        int dev_type = cJSON_GetInt(pCfg, "dt", -1);
        int dev_num = cJSON_GetInt(pCfg, "dtn", -1);

        first = RT_TRUE;
        WEBS_PRINTF("{\"ret\":0,\"list\":[");
        
        for (int slave = 1; slave < 247; slave++) {
            if (group_node_list[slave].use && 
                group_node_list[slave].dev_type == dev_type && 
                group_node_list[slave].dev_num == dev_num) {
                cJSON *pItem = cJSON_CreateObject();
                if(pItem) {
                    if (!first) WEBS_PRINTF(",");
                    first = RT_FALSE;
                    cJSON_AddStringToObject(pItem, "sn", group_node_list[slave].sn);
                    cJSON_AddNumberToObject(pItem, "dtp0", group_node_list[slave].dev_type_0);
                    cJSON_AddNumberToObject(pItem, "dtp1", group_node_list[slave].dev_type_1);
                    cJSON_AddNumberToObject(pItem, "sa", slave);
                    szRetJSON = cJSON_PrintUnformatted(pItem);
                    if(szRetJSON) {
                        WEBS_PRINTF(szRetJSON);
                        rt_free(szRetJSON);
                    }
                }
                cJSON_Delete(pItem);
            }
        }
        WEBS_PRINTF("]}");
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);
    WEBS_DONE(200);
}

DEF_CGI_HANDLER(setVarManageDoValue)
{
    int rc = 0;
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse( szJSON ) : RT_NULL;
    if( pCfg ) {
        int n = cJSON_GetInt(pCfg, "n", -1);
        int val = cJSON_GetInt(pCfg, "val", -1);
        if (n >= 0 && n < s_xExtDataList.n && val >= 0) {
            var_uint16_t usDevType = 0;
            var_uint8_t  btSlaveAddr = 0;
            var_uint16_t usExtAddr = 0;
            var_uint8_t  btOpCode = 0;
            var_int_t    nregs = 0;
            ExtData_t *data = RT_NULL;
            
            rt_enter_critical();
            {
                data = s_xExtDataList.pList;
                while (data && n--) {
                    data = data->next;
                }
                if (data && data->eAttr == E_VAR_ATTR_DO) {
                    usDevType   = data->xIo.usDevType;
                    btSlaveAddr = data->xIo.param.modbus.btSlaveAddr;
                    usExtAddr   = data->xIo.param.modbus.usExtAddr + data->xIo.param.modbus.usAddrOfs;
                    btOpCode    = data->xIo.param.modbus.btOpCode;
                    nregs       = 1;
                }
            }
            rt_exit_critical();
            
            if (nregs > 0) {
                var_int8_t _p = -1;
                var_uint16_t reg;
                if (PROTO_DEV_IS_RS(usDevType)) {
                    _p = nUartGetInstance(usDevType);
                }
                if(_p >= 0) {
                    if (val == 0) {
                        reg = 0x0000;
                    } else {
                        reg = 0x0100;
                    }
                    switch(btOpCode) {
                    case MODBUS_FC_READ_HOLDING_REGISTERS:
                        rc = modbus_write_registers_with(usDevType, _p, btSlaveAddr, usExtAddr, 1, (const uint16_t *)&reg);
                        if (rc < 0) err = RT_ERROR;
                        break;
                    }
                }
            }
        } else {
            err = RT_ERROR;
        }
    }

    cJSON_Delete(pCfg);
    WEBS_PRINTF( "{\"ret\":%d}", err);
    WEBS_DONE(200);
}

DEF_CGI_HANDLER(getVarManageAIValue)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse( szJSON ) : RT_NULL;
    if( pCfg ) {
        int n = cJSON_GetInt(pCfg, "n", -1);
        if (n >= 0 && n < s_xExtDataList.n) {
            ExtData_t *data = RT_NULL;
            cJSON *pItem = NULL;
            
            rt_enter_critical();
            {
                data = s_xExtDataList.pList;
                while (data && n--) {
                    data = data->next;
                }
                if (data && data->eAttr == E_VAR_ATTR_AI) {
                    pItem = cJSON_CreateObject();
                    if (pItem) {
                        cJSON_AddNumberToObject(pItem, "ret", RT_EOK);
                        cJSON_AddNumberToObject(pItem, "meas", data->xAttach.ai.measure);
                        cJSON_AddNumberToObject(pItem, "eng", data->xAttach.ai.eng_unit);
                    }
                }
            }
            rt_exit_critical();
            
            if (pItem) {
                char *szRetJSON = cJSON_PrintUnformatted( pItem );
                if (szRetJSON) {
                    WEBS_PRINTF(szRetJSON);
                    rt_free(szRetJSON);
                }
                cJSON_Delete(pItem);
            }
        } else {
            err = RT_ERROR;
            WEBS_PRINTF( "{\"ret\":%d}", err);
        }
    }

    cJSON_Delete(pCfg);
    WEBS_DONE(200);
}

const char *varmanage_endian_to_string(eVarType type, int n)
{
    switch (type) {
    case E_VAR_INT32:
    case E_VAR_UINT32:
    case E_VAR_FLOAT:
        if (n == 0) return "AB CD";
        else if (n == 1) return "CD AB";
        else if (n == 2) return "BA DC";
        else if (n == 3) return "DC BA";
        else return NULL;
    case E_VAR_DOUBLE:
        if (n == 0) return "AB CD EF GH";
        else if (n == 1) return "GH EF CD AB";
        else if (n == 2) return "BA DC FE HG";
        else if (n == 3) return "HG FE DC BA";
        else return NULL;
    default: return NULL;
    }
    return NULL;
}

int varmanage_endian_from_string(eVarType type, const char *endian)
{
    switch (type) {
    case E_VAR_INT32:
    case E_VAR_UINT32:
    case E_VAR_FLOAT:
        if (strcasecmp(endian, "AB CD") == 0) return 0;
        else if (strcasecmp(endian, "CD AB") == 0) return 1;
        else if (strcasecmp(endian, "BA DC") == 0) return 2;
        else if (strcasecmp(endian, "DC BA") == 0) return 3;
        else return -1;
    case E_VAR_DOUBLE:
        if (strcasecmp(endian, "AB CD EF GH") == 0) return 0;
        else if (strcasecmp(endian, "GH EF CD AB") == 0) return 1;
        else if (strcasecmp(endian, "BA DC FE HG") == 0) return 2;
        else if (strcasecmp(endian, "HG FE DC BA") == 0) return 3;
        else return -1;
    default: return -1;
    }
    return -1;
}

const char *varmanage_type_to_string(eVarType type)
{
    switch (type) {
    case E_VAR_BIT: return "BIT";
    case E_VAR_INT8: return "INT8";
    case E_VAR_UINT8: return "UINT8";
    case E_VAR_INT16: return "INT16";
    case E_VAR_UINT16: return "UINT16";
    case E_VAR_INT32: return "INT32";
    case E_VAR_UINT32: return "UINT32";
    case E_VAR_FLOAT: return "FLOAT";
    case E_VAR_DOUBLE: return "DOUBLE";
    default: return NULL;
    }
    return NULL;
}

eVarType varmanage_type_from_string(const char *type)
{
    if (strcasecmp(type, "BIT") == 0) return E_VAR_BIT;
    else if (strcasecmp(type, "INT8") == 0) return E_VAR_INT8;
    else if (strcasecmp(type, "UINT8") == 0) return E_VAR_UINT8;
    else if (strcasecmp(type, "INT16") == 0) return E_VAR_INT16;
    else if (strcasecmp(type, "UINT16") == 0) return E_VAR_UINT16;
    else if (strcasecmp(type, "INT32") == 0) return E_VAR_INT32;
    else if (strcasecmp(type, "UINT32") == 0) return E_VAR_UINT32;
    else if (strcasecmp(type, "FLOAT") == 0) return E_VAR_FLOAT;
    else if (strcasecmp(type, "DOUBLE") == 0) return E_VAR_DOUBLE;
    else return E_VAR_ERROR;
}

// op -> 3:READ_HOLDING_REGISTERS  4:READ_INPUT_REGISTERS
// in_type, out_type -> BIT, INT8, UIN8, INT16, UINT16, INT32, UINT32, FLOAT, DOUBLE  【out_type must > in_type】
// in_endian, out_endian -> AB CD, CD AB, BA DC, DC BA, AB CD EF GH, GH EF CD AB, BA DC FE HG, HG FE DC BA

//{ "name":"机柜A_AI_1", "alias":"","op":3, "in_type":"FLOAT", "in_endian":"AB CD", "out_type":"FLOAT", "out_endian":"AB CD", "reg_addr":10, "exp":"#*10"},

int __varmanage_add_dev_with_json(cJSON *obj, int dev_type, int dev_num, int proto)
{
    const char *group = cJSON_GetString(obj, "group", VAR_NULL);
    int slave = cJSON_GetInt(obj, "slave", -1);
    cJSON *regs = cJSON_GetObjectItem(obj, "regs");
    int regs_sz = 0;
    if ((group == VAR_NULL) || (strlen(group) <= 0) || (strlen(group) > (sizeof(VarGroup_t) - 1))) return WEBNET_ERR_PARAM;
    if (__var_has_group(group)) return WEBNET_ERR_GROUP_EXIST;
    if (slave < 1 || slave > 247) return WEBNET_ERR_PARAM;
    if (regs == VAR_NULL || regs->type != cJSON_Array || (regs_sz = cJSON_GetArraySize(regs)) <= 0) return WEBNET_ERR_PARAM;

    // check name
    for (int n = 0; n < regs_sz; n++) {
        cJSON *reg = cJSON_GetArrayItem(regs, n);
        if (reg) {
            const char *name = cJSON_GetString(reg, "name", VAR_NULL);
            if ((name == VAR_NULL) || (strlen(name) <= 0) || (strlen(name) > (sizeof(VarName_t) - 1))) return WEBNET_ERR_PARAM;
            if (__var_has_name(name)) return WEBNET_ERR_NAME_EXIST;
        }
    }

    for (int n = 0; n < regs_sz; n++) {
        cJSON *reg = cJSON_GetArrayItem(regs, n);
        if (reg) {
            const char *name = cJSON_GetString(reg, "name", VAR_NULL);
            const char *alias = cJSON_GetString(reg, "alias", VAR_NULL);
            const char *in_type = cJSON_GetString(reg, "in_type", VAR_NULL);
            const char *in_endian = cJSON_GetString(reg, "in_endian", VAR_NULL);
            const char *out_type = cJSON_GetString(reg, "out_type", VAR_NULL);
            const char *out_endian = cJSON_GetString(reg, "out_endian", VAR_NULL);

            int op = cJSON_GetInt(reg, "op", 3);
            int reg_addr = cJSON_GetInt(reg, "reg_addr", -1);
            const char *exp = cJSON_GetString(reg, "exp", VAR_NULL);
            
            eVarType e_in_type = E_VAR_ERROR;
            eVarType e_out_type = E_VAR_ERROR;
            int e_in_endian = 0;
            int e_out_endian = 0;

            if (__var_has_name(name)) return WEBNET_ERR_NAME_EXIST;
            if (reg_addr < 0 || reg_addr > 0xFFFF) return WEBNET_ERR_PARAM;
            if (op != MODBUS_FC_READ_HOLDING_REGISTERS && op != MODBUS_FC_READ_INPUT_REGISTERS) return WEBNET_ERR_PARAM;
            if ((in_type == VAR_NULL) || (strlen(in_type) <= 0) || ((e_in_type = varmanage_type_from_string(in_type)) >= E_VAR_ERROR)) return WEBNET_ERR_PARAM;
            e_out_type = e_in_type;
            if (out_type && (strlen(out_type) > 0)) {
                e_out_type = varmanage_type_from_string(out_type);
                if (e_out_type >= E_VAR_ERROR) return WEBNET_ERR_PARAM;
            }
            if (in_endian && (strlen(in_endian) > 0)) {
                e_in_endian = varmanage_endian_from_string(e_in_type, in_endian);
                if (e_in_endian < 0) return WEBNET_ERR_PARAM;
            }
            e_out_endian = e_in_endian;
            if (out_endian && (strlen(out_endian) > 0)) {
                e_out_endian = varmanage_endian_from_string(e_out_type, out_endian);
                if (e_out_endian < 0) return WEBNET_ERR_PARAM;
            }

            {
                var_uint16_t addr = __var_get_next_addr();
                ExtData_t *data = VAR_MANAGE_CALLOC(sizeof(ExtData_t), 1);
                if (data) {
                    data->bEnable = VAR_TRUE;
                    strcpy(data->xGroup.szGroup, group);
                    data->eAttr = E_VAR_ATTR_NONE;
                    strcpy(data->xName.szName, name);
                    data->xIo.btInVarType = e_in_type;
                    data->xIo.btInVarSize = g_var_type_sz[e_in_type];
                    data->xIo.btOutVarType = e_out_type;
                    data->xIo.btOutVarSize = g_var_type_sz[e_out_type];
                    data->xIo.usDevType = dev_type;
                    data->xIo.usDevNum = dev_num;
                    data->xIo.usProtoType = proto;
                    data->xIo.usAddr = addr;
                    data->xIo.param.modbus.btOpCode = op;
                    data->xIo.param.modbus.btSlaveAddr = slave;
                    data->xIo.param.modbus.usExtAddr = reg_addr;
                    addr += (g_var_type_sz[e_in_type] + 1) / 2;
                    vVarManage_InsertNode(data);
                    {
                        ExtData_t *datacopy = pVarManage_CopyData(data);
                        if (datacopy) {
                            board_cfg_varext_add(datacopy);
                            vVarManage_FreeData(datacopy);
                        }
                    }
                } else {
                    return WEBNET_ERR_NO_MEM;
                }
            }
        }
    }
    
    __sort_extdata_link();
    __RefreshRegsFlag();
    __creat_sync_slave_link();
    varmanage_update();
    return WEBNET_ERR_OK;
}

var_bool_t varmanage_add_dev_with_json(void *obj, const char *path)
{
    rt_bool_t ret = VAR_FALSE;
    int length = 0;
    int fd = -1;
    cJSON *json = VAR_NULL;
    struct webnet_session *session = (struct webnet_session *)obj;

    if (!session) return RT_FALSE;

    const char *dev_type = CGI_GET_ARG("dt");
    const char *dev_num = CGI_GET_ARG("dtn");
    const char *proto = CGI_GET_ARG("pt");

    if (dev_type && dev_num && proto) {
        ;
    } else {
        WEBS_PRINTF( "{\"ret\":%d}", WEBNET_ERR_PARAM);
        WEBS_DONE(200);
        return RT_FALSE;
    }

    fd = open(path, O_RDONLY, 0666);
    if (fd < 0) {
        WEBS_PRINTF( "{\"ret\":%d}", WEBNET_ERR_PARAM);
        WEBS_DONE(200);
        return RT_FALSE;
    }

    length = lseek(fd, 0L, SEEK_END);
    lseek(fd, 0L, SEEK_SET);

    {
        char *file_buffer = (char *)rt_calloc(1, length + 1);
        
        if (file_buffer) {
            read(fd, file_buffer, length);
            json = cJSON_Parse(file_buffer);
            if (json) {
                int rc = __varmanage_add_dev_with_json(json, atoi(dev_type), atoi(dev_num), atoi(proto));
                WEBS_PRINTF( "{\"ret\":%d}", rc);
                cJSON_Delete(json);
            } else {
                WEBS_PRINTF( "{\"ret\":%d}", WEBNET_ERR_PARAM);
            }
            rt_free(file_buffer);
        } else {
            WEBS_PRINTF( "{\"ret\":%d}", WEBNET_ERR_NO_MEM);
        }
    }
    close(fd);

    WEBS_DONE(200);
    return ret;
}


