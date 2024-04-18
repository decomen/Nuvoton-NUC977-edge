#ifndef __USER_MB_APP_H__
#define __USER_MB_APP_H__
/* ----------------------- Modbus includes ----------------------------------*/

#pragma pack(1)

#define MB_IN_USER_REG(_reg,_ad,_n) ((_ad)>=USER_REG_##_reg##_START && (_ad)<=USER_REG_##_reg##_END && ((_ad)+(_n))<=USER_REG_##_reg##_START+USER_REG_##_reg##_SIZE )

#define USER_REG_AI_START           50
#define USER_REG_AI_END             65
#define USER_REG_AI_SIZE            (USER_REG_AI_END-USER_REG_AI_START+1)

typedef struct {
    var_float_t fAI_xx[8];
} AIResult_t;

typedef union {
    AIResult_t xAIResult;
    var_uint16_t regs[USER_REG_AI_SIZE];
} AIResultReg_t;

#define USER_REG_DI_START           70
#define USER_REG_DI_END             77
#define USER_REG_DI_SIZE            (USER_REG_DI_END-USER_REG_DI_START+1)

typedef struct {
    var_uint16_t usDI_xx[8];
} DIResult_t;

typedef union {
    DIResult_t xDIResult;
    rt_uint16_t regs[USER_REG_DI_SIZE];
} DIResultReg_t;

#define USER_REG_DO_START           80
#define USER_REG_DO_END             87
#define USER_REG_DO_SIZE            (USER_REG_DO_END-USER_REG_DO_START+1)

typedef struct {
    var_uint16_t usDO_xx[8];
} DOResult_t;

typedef union {
    DOResult_t xDOResult;
    rt_uint16_t regs[USER_REG_DO_SIZE];
} DOResultReg_t;

#define USER_REG_NET_CFG_START      100
#define USER_REG_NET_CFG_END        133
#define USER_REG_NET_CFG_SIZE       (USER_REG_NET_CFG_END-USER_REG_NET_CFG_START+1)

typedef struct {
    var_uint16_t dhcp;  // 0 关, 1 开
    char ipaddr[16];
    char netmask[16];
    char gw[16];
    char dns[16];
    var_uint16_t status;
} NetCfg_t;

typedef union {
    NetCfg_t xNetCfg;
    rt_uint16_t regs[USER_REG_NET_CFG_SIZE];
} NetCfgReg_t;

#define USER_REG_EXT_DATA_START     1024
#define USER_REG_EXT_DATA_END       10240
#define EXT_DATA_REG_CHECK(_addr)   ((_addr)>=USER_REG_EXT_DATA_START && (_addr)<=USER_REG_EXT_DATA_END)
#define USER_REG_EXT_DATA_SIZE      (USER_REG_EXT_DATA_END-USER_REG_EXT_DATA_START+1)

typedef union {
    AIResultReg_t xAIResultReg;
    DIResultReg_t xDIResultReg;
    DOResultReg_t xDOResultReg;
    NetCfgReg_t xNetCfgReg;
} UserRegData_t;

#pragma pack()

typedef struct {
    rt_uint8_t  btFunctionCode;
    rt_uint16_t usAddOfs;
    rt_uint16_t usNRegs;
    ExtData_t   xData;
} ExtDataOp_t;

extern AIResultReg_t g_xAIResultReg;
extern DIResultReg_t g_xDIResultReg;
extern DOResultReg_t g_xDOResultReg;

void vMBRTUSlavePollTask( void* parameter );
void vMBASCIISlavePollTask( void* parameter );
rt_err_t xMBRTU_ASCIISlavePollReStart( rt_uint8_t ucPort, eMBMode eMode );
void vMBRTU_ASCIISlavePollStop( rt_uint8_t ucPort );

void vMBTCPSlavePollTask( void* parameter );
rt_err_t xMBTCPSlavePollReStart( rt_uint8_t ucPort );
void vMBTCPSlavePollStop( rt_uint8_t ucPort );

void vMBRTU_OverTCPSlavePollTask(void *parameter);
rt_err_t xMBRTU_OverTCPSlavePollReStart(rt_uint8_t ucPort);


rt_err_t xMBRTU_ASCIIMasterPollReStart( rt_uint8_t ucPort, eMBMode eMode );
void vMBRTU_ASCIIMasterPollStop( rt_uint8_t ucPort );

rt_err_t xMBTCPMasterPollReStart( rt_uint8_t ucPort );
void vMBTCPMasterPollStop( rt_uint8_t ucPort );

rt_err_t xMBRTU_OverTCPMasterPollReStart(rt_uint8_t ucPort);


rt_err_t modbus_write_registers_with_uart(int port, int slave,  int addr, int nb, const uint16_t * data);
rt_err_t modbus_write_registers_with_tcp(int port, int slave,  int addr, int nb, const uint16_t * data);
rt_err_t modbus_write_registers_with(int dev_type, int port, int slave, int addr, int nb, const uint16_t * data);

rt_err_t modbus_read_registers_with_uart(int port, int slave, int addr, int nb, uint16_t regs[128]);
rt_err_t modbus_read_registers_with_tcp(int port, int slave, int addr, int nb, uint16_t regs[128]);
rt_err_t modbus_read_registers_with(int dev_type, int dev_num, int port, int slave, int addr, int nb);

rt_err_t modbus_read_input_registers_with_uart(int port, int slave,          int addr, int nb, uint16_t regs[128]);
rt_err_t modbus_read_input_registers_with_tcp(int port, int slave,          int addr, int nb, uint16_t regs[128]);
rt_err_t modbus_read_input_registers_with(int dev_type, int dev_num, int port, int slave, int addr, int nb);

rt_err_t modbus_read_registers_quick_with_uart(int port, int slave,          int addr, int nb, uint16_t regs[128], int usec);

#endif
