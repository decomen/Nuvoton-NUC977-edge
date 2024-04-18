#ifndef __LORA_H__
#define __LORA_H__

#define LORA_OK         0
#define LORA_FAILED     0xFF

#define LORA_CMD_SEND       0x61
#define LORA_CMD_RECV       0x41
#define LORA_HEAD_LEN       0x08

#pragma pack(1)

typedef struct lora_cmd_head {
    uint8_t     cmd;
    uint32_t    addr;
    uint8_t     op;     // 0
    uint16_t    seq;
} lora_cmd_head_t;

typedef struct lora_cmd_pack {
    lora_cmd_head_t head;
    uint8_t     data[200];
    
    // data_len
    int data_len;
} lora_cmd_pack_t;

#pragma pack()

typedef enum {
    LORA_WORK_END_DEVICE    = 0x00,
    LORA_WORK_CENTRAL       = 0x01,
} LORA_WORK_TYPE_E;

typedef enum {
    LORA_FREQ_MODE_FIXED    = 0x00,
    LORA_FREQ_MODE_JUMP     = 0x01,
} LORA_FREQ_MODE_E;

typedef enum {
    LORA_WORK_MODE_FSK      = 0x00, // not support
    LORA_WORK_MODE_LORA     = 0x01,
} LORA_WORK_MODE_E;

typedef enum {
    LORA_BW_125K    = 0x07,
    LORA_BW_250K    = 0x08,
    LORA_BW_500K    = 0x09,
} LORA_BW_E;

typedef enum {
    LORA_CR_4_5     = 0x01,
    LORA_CR_4_6     = 0x02,
    LORA_CR_4_7     = 0x03,
    LORA_CR_4_8     = 0x04,
} LORA_CR_E;

typedef enum {
    LORA_FS_7       = 0x07,
    LORA_FS_8       = 0x08,
    LORA_FS_9       = 0x09,
    LORA_FS_10      = 0x0A,
    LORA_FS_11      = 0x0B,
    LORA_FS_12      = 0x0C,
} LORA_FS_E;

typedef enum {
    LORA_SIP_NONE   = 0x00,
    LORA_SIP_IP     = 0x01,
    LORA_SIP_SEQ    = 0x10,
    LORA_SIP_IP_SEQ = 0x11,
} LORA_SIP_E;

typedef enum {
    LORA_SERIAL_RATE_1200   = 0x00,
    LORA_SERIAL_RATE_2400,
    LORA_SERIAL_RATE_4800,
    LORA_SERIAL_RATE_9600,
    LORA_SERIAL_RATE_19200,
    LORA_SERIAL_RATE_38400,
    LORA_SERIAL_RATE_57600,
    LORA_SERIAL_RATE_115200,
    LORA_SERIAL_RATE_230400,
    LORA_SERIAL_RATE_380400
} LORA_SERIAL_RATE_E;

typedef enum {
    LORA_SERIAL_PAR_NONE   = 0x00,
    LORA_SERIAL_PAR_EVEN,
    LORA_SERIAL_PAR_ODD,
} LORA_SERIAL_PAR_E;

typedef enum {
    LORA_DATA_TYPE_SIMPLER   = 0x00,
    LORA_DATA_TYPE_TLV,
    LORA_DATA_TYPE_FRAME,
    LORA_DATA_TYPE_JSON,
} LORA_DATA_TYPE_E;

typedef enum {
    LORA_DEV_PTP = 0x0001,
    LORA_DEV_AD = 0x0002
} LORA_DEV_E;

typedef enum {
    LORA_ERR_OK = 0x00,
    LORA_ERR_ADDRESS_FAUSE,
    LORA_ERR_LENGTH_FAUSE,
    LORA_ERR_CHECK_FAUSE,
    LORA_ERR_WRITE_FAUSE,
    LORA_ERR_OTHER_FAUSE,
    LORA_ERR_TIMEOUT = 0xFE,
    LORA_ERR_OTHER = 0xFF,
} LORA_ERR_E;

typedef struct {
    uint8_t id[8];
    int snr;
    int rssi;
    //char ak[32];
    
    uint32_t addr;
    uint32_t maddr;
    uint32_t pow;
    uint32_t bw;        //<07>:125K  <08>:250K  <09>:500K
    uint32_t cr;        //<01>:4/5 <02>:4/6 <03>:4/7 <04>:4/8
    uint32_t crc;
    
    uint32_t tfreq;
    uint32_t rfreq;
    
    uint32_t tsf;       // 6 -> 12
    uint32_t rsf;       // 6 -> 12

    uint32_t net_type;
    uint32_t mode;
    
    uint32_t sync;      // 0x12
    uint32_t tprem;     // 0008
    uint32_t rprem;     // 0008
    uint32_t ldr;       // <00>:AUTO方式 BW=125K时SF11、SF12开启BW=250K时SF12开启 
                        // <01>:SF7~SF12全部开启 
                        // <02>:SF7~SF12全部关闭
    uint32_t tiq;       // 00 01
    uint32_t riq;       // 00 01
    uint32_t sip;       // <00>:默认 
                        // <01>:打开协议功能
                        // <10>:打开包序号功能
                        // <11>:打开节点包序号及协议
    uint32_t ack;       // 0
    
    uint32_t brate;     //
    uint32_t par;       // 
    uint32_t data_type; // 0
    uint32_t lcp;       // 0
    uint32_t lft;       // 0
    uint32_t lat;       // 0
    uint32_t lgt;       // 0
    uint32_t el;        // 0x00 -> 0xA8C0
} lora_dev_info;

int lora_init(int uart_default);
int __lora_reinit(int uart_default);

int lora_at_req_all(void);
int lora_at_set_all(void);

int lora_at_req_all_cfg(void);

void lora_hw_reset(void);
void lora_enter_at_mode(void);
void lora_exit_at_mode(void);
int lora_in_at_mode(void);
int lora_at_test(void);
int lora_at_def(void);
int lora_at_sw_reset(void);
int lora_at_req_ver(char *ver);
int lora_at_req_id(uint8_t id[8]);
int lora_at_req_csq(int *snr, int *rssi);
int lora_at_req_ak(char *ak);
int lora_at_req_addr(uint32_t *addr);
int lora_at_req_maddr(uint32_t *maddr);
int lora_at_req_sync(uint32_t *sync);
int lora_at_req_pow(uint32_t *pow);
int lora_at_req_bw(uint32_t *bw);
int lora_at_req_cr(uint32_t *cr);
int lora_at_req_crc(uint32_t *crc);
int lora_at_req_tfreq(uint32_t *tfreq);
int lora_at_req_rfreq(uint32_t *rfreq);
int lora_at_req_freqa(uint32_t *freqa);
int lora_at_req_freqb(uint32_t *freqb);
int lora_at_req_tsf(uint32_t *tsf);
int lora_at_req_rsf(uint32_t *rsf);
int lora_at_set_net(LORA_FREQ_MODE_E net);
int lora_at_set_ak(const char *ak);
int lora_at_set_addr(uint32_t addr);
int lora_at_set_maddr(uint32_t maddr);
int lora_at_set_mode(LORA_WORK_MODE_E mode);
int lora_at_set_tprem(uint32_t tprem);
int lora_at_set_rprem(uint32_t rprem);
int lora_at_set_ldr(uint32_t ldr);
int lora_at_set_sync(uint32_t sync);
int lora_at_set_pow(uint32_t pow);
int lora_at_set_bw(LORA_BW_E bw);
int lora_at_set_cr(LORA_CR_E cr);
int lora_at_set_crc(uint32_t crc);
int lora_at_set_tfreq(uint32_t tfreq);
int lora_at_set_rfreq(uint32_t rfreq);
int lora_at_set_tsf(LORA_FS_E tsf);
int lora_at_set_rsf(LORA_FS_E rsf);
int lora_at_set_tiq(uint32_t tiq);
int lora_at_set_riq(uint32_t riq);
int lora_at_set_sip(LORA_SIP_E sip);
int lora_at_set_ack(uint32_t ack);
int lora_at_set_brate(LORA_SERIAL_RATE_E brate);
int lora_at_set_par(LORA_SERIAL_PAR_E par);
int lora_at_set_type(LORA_DATA_TYPE_E type);
int lora_at_set_lcp(uint32_t lcp);
int lora_at_set_lft(uint32_t lft);
int lora_at_set_lat(uint32_t lat);
int lora_at_set_lgt(uint32_t lgt);
int lora_at_set_el(uint32_t el);

void lora_learn_now(void);

int lora_set_dst_addr(uint32_t addr);
void lora_set_broad_mode(void);
void lora_set_single_mode(void);
int lora_send_buffer(uint8_t *buffer, int n, int type);

extern char g_lora_ver[64+1];
extern char g_lora_ak[32+1];

#endif


