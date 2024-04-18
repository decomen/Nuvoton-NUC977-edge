
#ifndef _LORA_STD_H__
#define _LORA_STD_H__

#define LORA_OFS(_t, _m)    (unsigned long)(&(((_t *)0)->_m))

#define INVALID_RSSI        1000
#define INVALID_SNR         1000

typedef struct {
    uint16_t type0;
    uint16_t type1;
    uint8_t  sn[16];
} lora_id_t;

typedef enum {
    LORA_SN_T_MODBUS_RTU,
    LORA_SN_T_MODBUS_ASCII,
} elora_sn_type_t;

typedef union {
    uint8_t addr_8;      // 8位地址
} lora_sn_val_t;

typedef struct _lora_snode {
    uint8_t             type;           // 节点类型
    uint8_t             cnt;            // 数目
    lora_sn_val_t       *lst;           // 队列
    struct _lora_snode  *next, *prev;   // 管理
} lora_snode_t;

typedef struct _lora_mnode {
    uint8_t         workmode;       // 工作模式
    lora_id_t       id;             // 地址
    uint32_t        netid;          // 网络地址
    uint8_t         online;         // 是否在线
    uint8_t         rssi;           // 信号强度
    
    int16_t         snr;            // 信噪比(dB)
    int16_t         rssi_dB;        // 信号强度(dB)
    
    uint32_t        uptime;         // 上线时间
    uint32_t        lasttime;       // 最终时间
    uint32_t        last_hrt_time;  // 最终心跳时间
    uint32_t        last_rssi_time; // 最终更新rssi时间
    uint32_t        offtime;        // 下线时间
    lora_snode_t         *snode;         // 子节点(链表)
    struct _lora_mnode   *next, *prev;   // 管理
} lora_mnode_t;

#pragma pack(1)

#define LORA_STD_FA_SIZE     (1024)

#define LORA_STD_PRE0       (0xFE)
#define LORA_STD_PRE1       (0xFD)

typedef enum {
    LORA_STD_ACK            = 0x00,
    LORA_STD_POST           = 0x01,
    LORA_STD_REQ            = 0x02,
    LORA_STD_RSP            = 0x03,
    LORA_STD_MAX            = 0x04,
} elora_std_pack_t;

typedef enum {
    LORA_STD_MSG_PROT_SYNC      = 0x00,
    LORA_STD_MSG_PROT_HRT       = 0x01,
    LORA_STD_MSG_PROT_ONLINE    = 0x02,
    LORA_STD_MSG_PROT_JOIN      = 0x03,
    LORA_STD_MSG_PROT_INFO      = 0x04,
    LORA_STD_MSG_DATA_MODBUS_RTU    = 0x05,
    LORA_STD_MSG_DATA_TRT           = 0x06,
    LORA_STD_MSG_RSSI               = 0x07,
    LORA_STD_MSG_MAX                = 0x08,
} elora_std_msg_t;

typedef enum {
    LORA_STD_STATE_WAIT_SYNC            = 0x00,
    LORA_STD_STATE_WAIT_GET_INFO        = 0x01,
    LORA_STD_STATE_JOIN                 = 0x02,
} elora_std_state_t;

typedef struct {
    uint8_t     pre[2];         // 前导
    uint8_t     packtype:2;     // 帧类型
    uint8_t     p2p:1;          // 是否点对点
    uint8_t     msgtype:5;      // 消息类型
    uint8_t     rssi;           // 信号强度
    uint8_t     msglen;         // 消息长度
} lora_std_head_t;

typedef struct {
    lora_std_head_t head;
    uint8_t         *data;
    uint16_t        check;
} lora_std_pack_t;

typedef struct {
    uint32_t netid;
    uint8_t  type;
    uint32_t timestamp;
    uint8_t  cnt;
} lora_std_sync_t;

typedef struct {
    uint32_t netid;
} lora_std_info_req_t;

typedef struct {
    uint8_t  workmode;
    uint32_t netid;
    lora_id_t id;
    uint8_t  type;
    uint8_t  cnt;
    uint8_t  data[1];
} lora_std_info_t;

typedef struct {
    uint32_t netid;
} lora_std_rssi_req_t;

typedef struct {
    int16_t snr;
    int16_t rssi;
} lora_std_rssi_t;

#pragma pack()

// 初始化
void lora_std_init( void );

int lora_std_lock(void);
int lora_std_trylock(void);
void lora_std_unlock(void);
// 查找节点
lora_mnode_t *lora_mnode_find_with_netid( uint32_t netid );
// 查找节点
lora_mnode_t *lora_mnode_find_with_addr( elora_sn_type_t type, int addr );
// 根据网络地址移除
void lora_mnode_rm_with_id( uint32_t netid );
// 移除所有节点
void lora_mnode_rm_all( void );

uint32_t lora_mnode_num(void);
// 插入数据
lora_mnode_t *lora_mnode_insert( lora_std_info_t *info, uint8_t rssi);

lora_mnode_t* lora_mnode_online(uint32_t netid, uint8_t rssi);

rt_bool_t lora_set_dst_node( elora_sn_type_t type, int addr );

void lora_check_online( void );

lora_mnode_t *lora_mnode_update_rssi(lora_std_rssi_t *rssi, uint32_t netid, uint8_t rssi_p);

lora_mnode_t* lora_mnode_check_hrt(void);
lora_mnode_t* lora_mnode_check_rssi(void);

#endif


