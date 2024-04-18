#ifndef __DM101__H
#define __DM101__H
#include <stdint.h>

//#define DEBUG_CJY_OAT 1

//debug
#define DM101_DEBUG
#ifdef DM101_DEBUG
	#define dm101_debug rt_kprintf
        #define DM101_DEBUG_OAT	0
#else
	#define dm101_debug(...)
        #define DM101_DEBUG_OAT	0
#endif


#define USE_CRC		1
#define USE_CRYPT	1

#define CJY_TCP_PORT 4321
#define CJY_UDP_PORT 4322

#define MAGIC "SIOT"
#define MAGIC_0 'S'
#define MAGIC_1 'I'
#define MAGIC_2 'O'
#define MAGIC_3 'T'
#define MSG_VERSION  1

#define ZERO_END 1

#define F_MSG_NOACK 0
#define F_MSG_NEED_ACK 1
#define F_MSG_ACK 2

#define F_CRYPT	0x1

#define FORMAT_NONE 0
#define FORMAT_JSON 1
#define FORMAT_OCT  2
#define FORMAT_TEXT 3

//code define
#define CODE_SERVER_FIND 	0x0001
#define CODE_REPORT_INFO 	0x0002
#define CODE_SET_INFO		0x0003
#define CODE_HAERT			0x0004
#define CODE_OFFLINE		0x0005
#define CODE_UPDATE			0x0006 //服务端主动下发
#define CODE_REMOTE_REBOOT	0x0007
#define CODE_REQ_TIME		0x0008
#define CODE_SYS_TIME		0x0009

#define CODE_CHECK_VER       0x0010 // 设备端查询最新版本的固件信息。


#define CODE_SET_CONFIG		0x0101
#define CODE_GET_CONFIG		0x0102
#define CODE_SET_GROUP_DATA	0x0103
#define CODE_GET_GROUP_DATA	0x0104

#define CODE_SET_REPORT_RULE	0x0201
#define CODE_REPORT_DATA	0x0202
#define CODE_COMM_CTRL		0x0301


#define CODE_REQUEST_REC_FILE		0x0105  //请求资源文件下载地址

#define CODE_DOWNLOAD_REC_FILE		0x0106  //服务端下发资源文件信息



#define CTRL_CODE_STOP				0x0
#define CTRL_CODE_OPEN_CLOSE		0x101
#define CTRL_CODE_OPEN_CLOSE_INT	0x102
#define CTRL_CODE_OPEN_CLOSE_HOLD	0x103
#define CTRL_CODE_AI				0x201
#define CTRL_CODE_PI				0x202
#define CTRL_CODE_PERCENT			0x203

#define CTRL_OPEN		0x1			
#define CTRL_CLOSE		0x0	


#define CODE_STATUS_OK				0x1000
#define CODE_STATUS_NOT_ALLOW		0x1001
#define CODE_STATUS_FORMAT_ERR		0x1002
#define CODE_STATUS_UNSPORT_MSG		0x1003
#define CODE_STATUS_INTERNAL_ERR	0x1004

#define CODE_STATUS_PASER_ERR		0x2000
#define CODE_STATUS_DATA_INCON		0x2001
#define CODE_STATUS_DATA_MISS		0x2002
#define CODE_STATUS_BUSY			0x3000

//package
#define MAGIC_LEN			4
#define VAR_LEN				1
#define TYPE_LEN			1
#define SN_LEN				2
#define MSGCODE_LEN			2
#define FORMAT_LEN			1
#define MFLAG_LEN			1
#define PAYLOAD_LEN_LEN			4
#define TIME_STAMP_LEN			8
#define SRC_ID_LEN			8
#define DEST_ID_LEN			8
#define TOKEN_LEN			8
#define CODE_LEN			2
#define RESERVE_LEN			1
#define CRC8_LEN			1
#define CRC32_LEN			4
#define UTC_LEN				8
#define AUTH_LEN			8


#define OFFSIZE_MAGIC		0
#define OFFSIZE_VAR			(OFFSIZE_MAGIC + MAGIC_LEN)
#define OFFSIZE_TYPE		(OFFSIZE_VAR + VAR_LEN)
#define OFFSIZE_SN			(OFFSIZE_TYPE + TYPE_LEN)
#define OFFSIZE_MSGCODE		(OFFSIZE_SN + SN_LEN)
#define OFFSIZE_FORMAT		(OFFSIZE_MSGCODE + MSGCODE_LEN)
#define OFFSIZE_MFLAG		(OFFSIZE_FORMAT + FORMAT_LEN)
#define OFFSIZE_PAYLOAD_LEN	(OFFSIZE_MFLAG + MFLAG_LEN)
#define OFFSIZE_TIME_STAMP	(OFFSIZE_PAYLOAD_LEN + PAYLOAD_LEN_LEN)
#define OFFSIZE_SRC_ID			(OFFSIZE_TIME_STAMP + TIME_STAMP_LEN)
#define OFFSIZE_DEST_ID			(OFFSIZE_SRC_ID + SRC_ID_LEN)
#define OFFSIZE_TOKEN		(OFFSIZE_DEST_ID + TOKEN_LEN)
#define OFFSIZE_CODE		(OFFSIZE_TOKEN + TOKEN_LEN)
#define OFFSIZE_RESERVE		(OFFSIZE_CODE + CODE_LEN)
#define OFFSIZE_CRC8		(OFFSIZE_RESERVE + RESERVE_LEN)
#define HEAD_LEN			(OFFSIZE_CRC8 + CRC8_LEN)


#define INTER_BUF_SIZE	1024
#define DATA_ID_LEN		16
#define DM101_SENDBUF_LEN	8192
#define DM101_RECVBUF_LEN	4096

struct Head {
	uint8_t magic[4];
	uint8_t ver;
	uint8_t type;
	uint16_t sn;
	uint16_t msgcode;
	uint8_t mflag;
	uint8_t format;
	uint32_t payload_length;
	uint64_t timestamp;	
	uint64_t src_id;
	uint64_t dest_id;
	uint64_t token;
	uint16_t code;
	uint8_t reserve[RESERVE_LEN];
	uint8_t dm_crc8;
};

struct Dm101pkg {
	struct Head head;
	char *pload_buf;
	uint32_t dm_crc32;
};

struct Data{
	uint32_t sid[DATA_ID_LEN];
	uint8_t id[DATA_ID_LEN];
	double value;
	uint16_t status;
    uint8_t  pi;    //小数点位数
};
struct Data_pkg{
	uint64_t timestamp;
	uint32_t data_cnt;
	struct Data *data;
};

struct Update_data{
	uint8_t type[32];
	uint8_t ver[32];
	uint8_t url[512];
	uint8_t hash[128];
};

struct Base_info{
	uint8_t type[32];
	uint8_t ver[32];
	uint8_t name[64];
};

struct Server_info{
	uint32_t id;
	char name[64];
	char addr[32];
	uint16_t port;	
};


struct Report_rule{
	uint8_t enabled;
	uint32_t interval;
};

struct dm101_cfg {
    uint8_t magic[MAGIC_LEN];
    uint8_t ver;
    uint8_t crypt;
    uint8_t auth[AUTH_LEN];
    uint64_t srdid;
};

struct Dm101_context {
	uint8_t *recvbuf;
	uint8_t *sendbuf;
	struct dm101_cfg cfg;
    void *resetargs;
	void *closeargs;
	void *postargs;
    void *getargs;

	void *(*post_get_init)(void *args);
	void (*post_get_reset)(void *args);
	void (*post_get_close)(void *args);
	int (*postdata)(void *args, void *buf, uint32_t count,int8_t rw_flag);
	int (*getdata)(void *args,void *buf, uint32_t count,int8_t rw_flag,uint32_t timeout);
};



#define TRACE_CMH_2() \
rt_kprintf("%s(%d)-<%s>: \n", __FILE__, __LINE__, __FUNCTION__)

int32_t dm101_make_pkg(struct Dm101_context *context, struct Dm101pkg *dm101pkg, int32_t code, void *text, uint32_t text_len);
int32_t dm101_get_pkg(struct Dm101_context *context, struct Dm101pkg *dm101pkg);
uint32_t make_ack_pkg(struct Dm101_context *context, struct Dm101pkg *dm101pkg, int32_t ackcode, void *text, uint32_t text_len);

int32_t read_pkg(struct Dm101_context *context, struct Dm101pkg *dm101pkg);
int64_t  send_pkg(struct Dm101_context *context, struct Dm101pkg *dm101pkg);


int32_t showoatbuf(char *tag, char *buf, int32_t len);

struct Dm101_context *dm101_context_init(
    struct dm101_cfg *cfg,
	//void *post_get_init,
	void *(*post_get_init)(void *argv),
	void *post_get_init_args,
	void (*post_get_reset)(void *argv),
	void (*post_get_close)(void *argv),
	int (*postdata)(void *args, void *buf, uint32_t count, int8_t rw_flag),
	int (*getdata)(void *args, void *buf, uint32_t count, int8_t rw_flag,uint32_t timeout));


uint32_t dm101_context_reset(struct Dm101_context *context);
uint32_t dm101_context_close(struct Dm101_context *context);
uint32_t distory_dm101_context(struct Dm101_context *context);

extern uint64_t get_utc_time(void);

extern const struct dm101_cfg c_dm101_default_cfg;

#endif

