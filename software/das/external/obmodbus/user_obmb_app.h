#ifndef __USER_OB_MB_APP_H__
#define __USER_OB_MB_APP_H__

rt_err_t xOBMBRTU_ASCIIMasterPollReStart(rt_uint8_t ucPort, eMBMode eMode);
void vOBMBRTU_ASCIIMasterPollStop(rt_uint8_t ucPort);
rt_err_t obmodbus_read_registers_with_uart(int port, int slave, int function, int addr, int nb, uint8_t regs[256]);
rt_err_t obmodbus_read_registers_quick_with_uart(int port, int slave, int function, int addr, int nb, uint8_t regs[256], int usec);
rt_err_t obmodbus_read_registers_with(int dev_type, int port, int dev_num, int slave, int function, int addr, int nb);

#endif
