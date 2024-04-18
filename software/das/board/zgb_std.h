
#ifndef _ZIGBEE_STD_H__
#define _ZIGBEE_STD_H__

#define ZGB_OFS(_t, _m)  (unsigned long)(&(((_t *)0)->_m))

typedef struct {
    rt_uint8_t  mac[8];
} zgb_mac_t;

typedef enum {
    ZGB_SN_T_MODBUS_RTU,
    ZGB_SN_T_MODBUS_ASCII,

} ezgb_sn_type_t;

typedef union {
    rt_uint8_t addr_8;      // 8位地址
} zgb_sn_val_t;

typedef struct _zgb_snode {
    rt_uint8_t          type;           // 节点类型
    rt_uint8_t          cnt;            // 数目
    zgb_sn_val_t        *lst;           // 队列
    struct _zgb_snode   *next, *prev;   // 管理
} zgb_snode_t;

typedef struct _zgb_mnode {
    rt_uint8_t          workmode;       // 工作模式
    zgb_mac_t           mac;            // MAC地址 (fastzigbee 无用)
    rt_uint16_t         netid;          // 网络地址(fastzigbee 专用)
    rt_uint8_t          online;         // 是否在线
    rt_uint8_t          rssi;           // 信号强度
    rt_uint32_t         uptime;         // 上线时间
    rt_uint32_t         lasttime;       // 最终时间
    rt_uint32_t         offtime;        // 下线时间
    zgb_snode_t         *snode;         // 子节点(链表)
    struct _zgb_mnode   *next, *prev;   // 管理
} zgb_mnode_t;

#pragma pack(1)

#define ZGB_STD_FA_SIZE     (2048)

#define ZGB_STD_PRE         (0x9AABDEEF)
#define ZGB_STD_PREn(n)     ((ZGB_STD_PRE>>(n<<3))&0xFF)
#define ZGB_STD_NPRE        (~ZGB_STD_PRE)
#define ZGB_STD_NPREn(n)    ((ZGB_STD_NPRE>>(n<<3))&0xFF)

typedef enum {
    ZGB_STD_FA_ACK          = 0x00,
    ZGB_STD_FA_POST         = 0x01,
    ZGB_STD_FA_REQ          = 0x02,
    ZGB_STD_FA_RSP          = 0x03,
} ezgb_std_pack_t;

typedef enum {
    //ZGB_STD_MSG_SCAN        = 0x00,  // 扫描
    ZGB_STD_MSG_SCAN        = 0x01,  // 扫描(2016/10/28采用新命令字,防止与旧版本冲突)
} ezgb_std_msg_t;

typedef struct {
    rt_uint32_t pre;        // 前导(尽量长,避免冲突)
    rt_uint32_t npre;       // 反前导(按位取反)
    rt_uint8_t  ver;        // 版本
    rt_uint16_t msglen;     // 消息长度
    rt_uint8_t  seq;        // 序号
    rt_uint8_t  packtype;   // 帧类型
    rt_uint8_t  msgtype;    // 消息类型 (目前为0)
} zgb_std_head_t;

typedef struct {
    zgb_std_head_t  head;
    rt_uint8_t      *data;
    rt_uint32_t     crc;
} zgb_std_pack_t;

typedef struct {
    rt_uint16_t netid;
    zgb_mac_t   mac;
    rt_uint8_t  type;
    rt_uint16_t learnstep;
} zgb_std_scan_req_t;

typedef struct {
    rt_uint8_t  workmode;
    rt_uint16_t netid;
    zgb_mac_t   mac;
    rt_uint8_t  rssi;
    rt_uint8_t  type;
    rt_uint8_t  cnt;
    rt_uint8_t  data[1];
} zgb_std_scan_t;

#pragma pack()

// 初始化
void zgb_std_init( void );
// 查找节点
zgb_mnode_t *zgb_mnode_find_with_netid( rt_uint16_t netid );
// 查找节点
zgb_mnode_t *zgb_mnode_find_with_addr( ezgb_sn_type_t type, int addr );
// 根据网络地址移除
void zgb_mnode_rm_with_id( rt_uint16_t netid );
// 移除所有节点
void zgb_mnode_rm_all( void );
// 插入数据
zgb_mnode_t *zgb_mnode_insert( zgb_std_scan_t *info );
// 数据解析
zgb_mnode_t *zgb_std_parse_buffer( zgb_std_head_t *head, rt_uint8_t buffer[] );

rt_bool_t zgb_set_dst_node( ezgb_sn_type_t type, int addr );

void zgb_check_online( void );

#endif

