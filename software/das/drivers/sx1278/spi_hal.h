#ifndef __SX1278_H__
#define __SX1278_H__

#include "board.h"

void sx_init(void);
//void SXReadReg( uint8_t addr, uint8_t *data );
//void SXWriteReg( uint8_t addr, uint8_t data );
uint8_t SpiInOut( uint8_t outData );
void vSetCs(uint8_t stat);

#define SX_CS_0()     rt_pin_write( BOARD_SX_PCS0, PIN_LOW)
#define SX_CS_1()     rt_pin_write( BOARD_SX_PCS0, PIN_HIGH)

#define SX_DIN_0()     rt_pin_write( BOARD_SX_SIN, PIN_LOW)
#define SX_DIN_1()    rt_pin_write( BOARD_SX_SIN, PIN_HIGH)

#define SX_SCLK_0()    rt_pin_write( BOARD_SX_SCK, PIN_LOW)
#define SX_SCLK_1()     rt_pin_write( BOARD_SX_SCK, PIN_HIGH)


#define SX_DOUT_READ()  rt_pin_read( BOARD_SX_SOUT)

#define SX_DID0_READ()  rt_pin_read( BOARD_SX_DIDO)

#define SX_RESET_0()   rt_pin_write( BOARD_SX_RESET, PIN_LOW)
#define SX_RESET_1()   rt_pin_write( BOARD_SX_RESET, PIN_HIGH)

#define SX_SDN_0()    rt_pin_write( BOARD_SX_SDN, PIN_LOW)
#define SX_SDN_1()    rt_pin_write( BOARD_SX_SDN, PIN_HIGH)

#endif