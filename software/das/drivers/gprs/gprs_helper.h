
#ifndef __GPRS_HELPER_H__
#define __GPRS_HELPER_H__

typedef enum {
    GPRS_CODE_OK,                   // 0:       OK
    GPRS_CODE_ERROR,                // 1:       ERROR
    GPRS_CODE_MAX,
} eGPRS_Code;

typedef enum {
    GPRS_EVT_COPS,                  // +COPS: 
    GPRS_EVT_CSQ,                   // +CSQ: 
    GPRS_EVT_CREG,                  // +CREG: 
    GPRS_EVT_MUX,                   // +QIMUX: 
    GPRS_EVT_NWTIME,                // ^NWTIME:
    GPRS_EVT_SEDN_ACK,              // +QISACK: <sent>,      +QISACK: <sent>, <acked>, <nAcked>    
    GPRS_EVT_IP,
    
    GPRS_EVT_MAX,
} eGPRS_Evt;

typedef struct {
    rt_ubase_t uId;         //ID
    rt_ubase_t uState;      //state
    rt_ubase_t uSrvNums;    //      
    char szIP[16];          //IP
} GPRS_NState_t;

typedef struct {
    rt_ubase_t numericn;    //运营商(数字)
    char alphan[32];        //运营商(字串)
} GPRS_COPN_t;

typedef struct {
    int sent;    //已经发送的数据
    int acked;        //远程已经接受到的数据
    int nAcked; //已经发送未收到远程确认的数据
} GPRS_SEND_ACK_t;


typedef struct {
    rt_ubase_t numericn;    //运营商(数字)
    char alphan[32];        //运营商(字串)
    char salphan[16];       //运营商(短)
} GPRS_COPS_Node_t;

typedef struct {
    rt_base_t nCount;
    GPRS_COPS_Node_t xCOPS[5];  //最多5个已注册运营商
} GPRS_COPS_t;

typedef struct {
    rt_base_t MCC;
    rt_base_t MNC;
    rt_base_t LAC;
    rt_base_t cell;
    rt_base_t BSIC;
    rt_base_t chann;
    rt_base_t RxLev;
    rt_base_t RxLevFull;
    rt_base_t RxLevSub;
    rt_base_t RxQual;
    rt_base_t RxQualFull;
    rt_base_t RxQualSub;
    rt_base_t Timeslot;
} GPRS_SMOND_SCI_t;

typedef struct {
    rt_base_t MCC;
    rt_base_t MNC;
    rt_base_t LAC;
    rt_base_t cell;
    rt_base_t BSIC;
    rt_base_t chann;
    rt_base_t RxLev;
} GPRS_SMOND_NCI_t;

typedef struct {
    GPRS_SMOND_SCI_t xSCI;
    GPRS_SMOND_NCI_t xNCI[6];
    rt_base_t TA;
    rt_base_t RSSI;
    rt_base_t BER;
} GPRS_SMOND_t;

typedef union {
    GPRS_SMOND_t xSMOND;
    rt_base_t nVals[58];
} UGPRS_SMOND_t;

extern GPRS_NState_t g_xGPRS_NState;
extern GPRS_COPN_t g_xGPRS_COPN;
extern GPRS_COPS_t g_xGPRS_COPS;
extern UGPRS_SMOND_t g_xGPRS_SMOND;

extern rt_base_t g_nGPRS_CSQ;
extern rt_base_t g_nGPRS_CREG;
extern rt_base_t g_nGPRS_MUX;

rt_err_t gprs_do_init(void);
rt_err_t gprs_do_reinit(int shutdown_if_init_error);
void vGPRS_HWRest( void );
void vGPRS_PowerDown( void );
void vGPRS_PowerUp( void );
void vGPRS_TermDown( void );
void vGPRS_TermUp( void );

rt_bool_t bGPRS_GetNetTime(rt_bool_t bWait);
rt_bool_t bGPRS_GetCOPN( rt_bool_t bWait );
rt_bool_t bGPRS_GetCOPS( rt_bool_t bWait );
rt_bool_t bGPRS_GetSMOND( rt_bool_t bWait );
rt_bool_t bGPRS_GetCSQ( rt_bool_t bWait );
rt_bool_t bGPRS_GetCREG( rt_bool_t bWait );
rt_bool_t bGPRS_SetCSCA( const char *szMsgNo );
rt_bool_t bGPRS_NetInit( const char *szAPN, const char *szAPNNo, const char *szUser, const char *szPsk );
rt_bool_t bGPRS_TcpOt(void );

rt_bool_t bGPRS_ATTest(void);

#endif

