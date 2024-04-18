
#ifndef __CC_BJDC_H__
#define __CC_BJDC_H__

#define CC_BJDC_INI_CFG_PATH_PREFIX         BOARD_CFG_PATH"rtu_cc_bjdc_"

#define CC_BJDC_BUF_SIZE        (2048)
#define CC_BJDC_INBUF_SIZE      (512)
#define CC_BJDC_PARSE_STACK     (2048)      //解析任务内存占用
//#define CC_BJDC_BIGEDIAN       1
#define CC_BJDC_LITTLEEDIAN     1

#define CC_BJDC_PRE             0x55AA55AA
#define CC_BJDC_EOM             0x16166868

#if defined(CC_BJDC_LITTLEEDIAN)
#define cc_bjdc_htonl(x)        (x)
#define cc_bjdc_htons(x)        (x)
#elif defined(CC_BJDC_BIGEDIAN)
#define cc_bjdc_htonl(x)        lwip_htonl(x)
#define cc_bjdc_htons(x)        lwip_htons(x)
#else
#error must define EDIAN!
#endif

#define CC_BJDC_MD5_KEY         "IJKLMNOPQRSTUVWX"

typedef struct {
    rt_uint32_t         period;
} CC_BJDC_Cfg_t;

#pragma pack(1)


typedef enum {
    CC_BJDC_PT_VERIFY  = 0,
    CC_BJDC_PT_DEVINFO,
    CC_BJDC_PT_HEARTBEAT,
    CC_BJDC_PT_PERIOD_CFG,
    CC_BJDC_PT_RESTART,
    CC_BJDC_PT_DATA_QUERY,
    CC_BJDC_PT_DATA_CONTINUE,
    CC_BJDC_PT_UNKNOWN,
} eCC_BJDC_PackType_t;

typedef enum {
    CC_BJDC_R_S_HEAD     = 0,
    CC_BJDC_R_S_EOM,
} eCC_BJDC_RcvState_t;

typedef struct {
    rt_uint32_t         ulType;     //指令序号
    rt_uint8_t          *pData;     //指令内容
} CC_BJDC_Msg_t;

typedef struct {
    rt_uint32_t         ulPre;      //前导
    rt_uint32_t         ulLen;      //有效数据长度
    CC_BJDC_Msg_t       xMsg;       //有效数据
    rt_uint16_t         usCheck;    //校验
    rt_uint32_t         ulEom;      //分隔符
} CC_BJDC_Package_t;

typedef enum {
    CC_BJDC_VERIFY_SEQ  = 0,
    CC_BJDC_VERIFY_RESULT,
    CC_BJDC_VERIFY_PASS,
} eCC_BJDC_VerifyState_t;

typedef union {
    char                szSequence[9];  //序号
    rt_bool_t           bPass;          //结果
} CC_BJDC_VerifyParam_t;

typedef struct {
    rt_uint8_t              btState;
    CC_BJDC_VerifyParam_t   xParam;
} CC_BJDC_Verify_t;

typedef struct {
    rt_uint32_t             ulPeriod;
} CC_BJDC_Period_t;

typedef struct {
    char                    szDate[16];
} CC_BJDC_Heartbeat_t;

typedef struct {
    eCC_BJDC_PackType_t     eType;
    union {
        CC_BJDC_Verify_t        xVerify;
        CC_BJDC_Heartbeat_t     xHeartbeat;
        CC_BJDC_Period_t        xPeriod;
    } xData;
} CC_BJDC_Data_t;

#pragma pack()


rt_bool_t cc_bjdc_open(rt_uint8_t index);
void cc_bjdc_close(rt_uint8_t index);

rt_err_t CC_BJDC_PutBytes(rt_uint8_t index, rt_uint8_t *pBytes, rt_uint16_t usBytesLen);
void cc_bjdc_startwork(rt_uint8_t index);
void cc_bjdc_exitwork(rt_uint8_t index);

rt_uint16_t CC_GetCrc16(rt_uint8_t *pData, rt_uint32_t ulLen);
rt_bool_t cc_bjdc_verify_req(rt_uint8_t index, const char *buiding_id, const char *gateway_id);
rt_bool_t cc_bjdc_checkmd5_req(rt_uint8_t index, const char *buiding_id, const char *gateway_id, const char *md5);
rt_bool_t cc_bjdc_deviceinfo_req
(
    rt_uint8_t      index,
    const char      *building_id,
    const char      *gateway_id,
    const char      *build_name,
    const char      *build_no,
    const char      *dev_no,
    const char      *factory,
    const char      *hardware,
    const char      *software,
    const char      *server,
    const int        port,
    const char      *host,
    const int        com,
    const int        dev_num,
    const int        period,
    const char      *address
    );
rt_bool_t cc_bjdc_heartbeat_req(rt_uint8_t index, const char *building_id, const char *gateway_id);
rt_bool_t cc_bjdc_period_rsp(rt_uint8_t index, const char *building_id, const char *gateway_id, rt_bool_t bPass);

#endif

