#ifndef __ZIGBEE_H__
#define __ZIGBEE_H__

#define ZIGBEE_TEMP_PRECODE_0   0xDE
#define ZIGBEE_TEMP_PRECODE_1   0xDF
#define ZIGBEE_TEMP_PRECODE_2   0xEF

#define ZIGBEE_TEMP_PRECODE ZIGBEE_TEMP_PRECODE_0, ZIGBEE_TEMP_PRECODE_1, ZIGBEE_TEMP_PRECODE_2

#define ZIGBEE_HEAD_LEN     0x04

#define ZIGBEE_TEMP_CMD_SET_CHAN        0xD1
#define ZIGBEE_TEMP_CMD_SET_DST_ADDR    0xD2
#define ZIGBEE_TEMP_CMD_SHOW_SRC_ADDR   0xD3
#define ZIGBEE_TEMP_CMD_GPIO_CONFIG     0xD4
#define ZIGBEE_TEMP_CMD_GPIO_READ       0xD5
#define ZIGBEE_TEMP_CMD_GPIO_WRITE      0xD6
#define ZIGBEE_TEMP_CMD_AD              0xD7
#define ZIGBEE_TEMP_CMD_SLEEP           0xD8
#define ZIGBEE_TEMP_CMD_MODE            0xD9
#define ZIGBEE_TEMP_CMD_RSSI            0xDA

#define ZIGBEE_OK   0
#define ZIGBEE_FAILED   0xFF

typedef enum {
    ZIGBEE_IO_IN = 0x00,
    ZIGBEE_IO_OUT = 0x01
} ZIGBEE_IO_CFG_E;

typedef enum {
    ZIGBEE_MSG_MODE_SINGLE = 0x00,
    ZIGBEE_MSG_MODE_BROAD = 0x01
} ZIGBEE_MSG_MODE_E;


#define ZIGBEE_PRECODE_0    0xAB
#define ZIGBEE_PRECODE_1    0xBC
#define ZIGBEE_PRECODE_2    0xCD

#define ZIGBEE_PRECODE  ZIGBEE_PRECODE_0, ZIGBEE_PRECODE_1, ZIGBEE_PRECODE_2

#define ZIGBEE_CMD_GET_DEV_INFO         0xD1
#define ZIGBEE_CMD_SET_CHAN             0xD2
#define ZIGBEE_CMD_SEARCH           0xD4
#define ZIGBEE_CMD_GET_OTHER_DEV_INFO   0xD5
#define ZIGBEE_CMD_SET_DEV_INFO         0xD6
#define ZIGBEE_CMD_RST                  0xD9
#define ZIGBEE_CMD_DEFAULT              0xDA
#define ZIGBEE_CMD_PWD_EN               0xDE
#define ZIGBEE_CMD_LOGIN                0xDF

typedef enum {
    ZIGBEE_WORK_END_DEVICE = 0x00,
    ZIGBEE_WORK_ROUTER,
    ZIGBEE_WORK_COORDINATOR
} ZIGBEE_WORK_TYPE_E;

typedef enum {
    ZIGBEE_SERIAL_RATE_2400 = 0x01,
    ZIGBEE_SERIAL_RATE_4800,
    ZIGBEE_SERIAL_RATE_9600,
    ZIGBEE_SERIAL_RATE_19200,
    ZIGBEE_SERIAL_RATE_38400,
    ZIGBEE_SERIAL_RATE_57600,
    ZIGBEE_SERIAL_RATE_115200
} ZIGBEE_SERIAL_RATE_E;

typedef enum {
    ZIGBEE_DEV_PTP = 0x0001,
    ZIGBEE_DEV_AD = 0x0002
} ZIGBEE_DEV_E;

typedef enum {
    ZIGBEE_ERR_OK = 0x00,
    ZIGBEE_ERR_ADDRESS_FAUSE,
    ZIGBEE_ERR_LENGTH_FAUSE,
    ZIGBEE_ERR_CHECK_FAUSE,
    ZIGBEE_ERR_WRITE_FAUSE,
    ZIGBEE_ERR_OTHER_FAUSE,
    ZIGBEE_ERR_TIMEOUT = 0xFE,
    ZIGBEE_ERR_OTHER = 0xFF,
} ZIGBEE_ERR_E;

#pragma pack(1)
typedef struct {
    UCHAR DevName[16];      //设备名称(字串)
    UCHAR DevPwd[16];       //设备密码(字串)
    UCHAR WorkMode;         //工作类型 0=END_DEVICE，1=ROUTER，2=COORDINATOR
    UCHAR Chan;             //通道号
    USHORT PanID;           //网络ID
    USHORT Addr;            //本机网络地址
    UCHAR Mac[8];           //本机物理地址(MAC, 只读固定不可修改)
    USHORT DstAddr;         //目标网络地址
    UCHAR DstMac[8];        //目标物理地址(MAC)
    UCHAR Reserve1[1];      //保留
    UCHAR PowerLevel;       //发射功率(保留, 只读)
    UCHAR RetryNum;         //发送数据重试次数
    UCHAR TranTimeout;      //发送数据重试时间间隔(单位:100ms)
    UCHAR SerialRate;       //波特率设置 (0=1200, 1=2400, 2=4800, 3=9600, 4=19200, 5=38400, 6=57600, 7=115200, 8=230400, 9=460800)
    UCHAR SerialDataB;      //串口数据位 5~8
    UCHAR SerialStopB;      //串口停止位 1~2
    UCHAR SerialParityB;    //串口校验位 0=无校验 1=奇校验 2=偶校验
    UCHAR MsgMode;          //发送模式 0=单播  1=广播
} ZIGBEE_DEV_INFO_T;
#pragma pack()

// 修改通道号
UCHAR ucZigbeeSetChan(UCHAR chan);
// 修改目的网络
UCHAR ucZigbeeSetDstAddr(USHORT Addr);
// 包头显示源地址
UCHAR ucZigbeeShowSrcAddr(BOOL show);
// 设置GPIO输入输出方向
UCHAR ucZigbeeGPIOConfig(USHORT Addr, UCHAR cfg);
// 读取GPIO
UCHAR ucZigbeeGPIORead(USHORT Addr, UCHAR *pGpio);
// 设置GPIO电平
UCHAR ucZigbeeGPIOWrite(USHORT Addr, UCHAR gpio);
// 读取AD
UCHAR ucZigbeeAD(USHORT Addr, UCHAR channel, USHORT *pADValue);
// 休眠
UCHAR ucZigbeeSleep();
// 设置通讯模式
UCHAR ucZigbeeMode(ZIGBEE_MSG_MODE_E mode);
// 获取节点信号强度
UCHAR ucZigbeeRSSI(USHORT Addr, UCHAR *pRssi);


// 读取本地配置命令
ZIGBEE_ERR_E eZigbeeGetDevInfo(ZIGBEE_DEV_INFO_T *pInfo, UCHAR *pState, USHORT *pType, USHORT *pVer);
// 修改通道号
ZIGBEE_ERR_E eZigbeeSetChan(UCHAR chan);
// 搜索
ZIGBEE_ERR_E eZigbeeSearch(USHORT *pType, UCHAR *pChan, UCHAR *pSpeed, USHORT *pPanId, USHORT *pAddr, UCHAR *pState);
// 获取远程配置信息
ZIGBEE_ERR_E eZigbeeGetOtherDevInfo(USHORT Addr, ZIGBEE_DEV_INFO_T *pInfo, UCHAR *pState, USHORT *pType, USHORT *pVer);
// 修改配置命令
ZIGBEE_ERR_E eZigbeeSetDevInfo(USHORT Addr, ZIGBEE_DEV_INFO_T xInfo);
// 复位
void vZigbeeReset(USHORT Addr, ZIGBEE_DEV_E devType);
// 恢复出厂设置
ZIGBEE_ERR_E eZigbeeSetDefault(USHORT Addr, ZIGBEE_DEV_E devType);
// 获取终端休眠时间
ZIGBEE_ERR_E eZigbeeGetPwdEnable(BOOL *pEn);
// 获取终端休眠时间
ZIGBEE_ERR_E eZigbeeSetPwdEnable(BOOL en);
// 设置终端休眠时间
ZIGBEE_ERR_E eZigbeeLogin(UCHAR pwd[], UCHAR len);

rt_bool_t bZigbeeInCmd(void);
void vZigbeeRecvHandle(rt_uint8_t value);

BOOL bZigbeeInit(BOOL uart_default);
void vZigbeeHWReset(void);

void vZigbeeLearnNow(void);

void vZigbee_SendBuffer(rt_uint8_t *buffer, int n);

#endif

