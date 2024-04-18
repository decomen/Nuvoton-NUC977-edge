/*
 * THE FOLLOWING FIRMWARE IS PROVIDED: (1) "AS IS" WITH NO WARRANTY; AND 
 * (2)TO ENABLE ACCESS TO CODING INFORMATION TO GUIDE AND FACILITATE CUSTOMER.
 * CONSEQUENTLY, SEMTECH SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT OR
 * CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT
 * OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION
 * CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 * 
 * Copyright (C) SEMTECH S.A.
 */
/*! 
 * \file       sx1276-Hal.c
 * \brief      SX1276 Hardware Abstraction Layer
 *
 * \version    2.0.B2 
 * \date       Nov 21 2012
 * \author     Miguel Luis
 *
 * Last modified by Miguel Luis on Jun 19 2013
 */
#include <stdint.h>
#include <stdbool.h> 
#include "sx1276-Hal.h"
#include "spi_hal.h"

/*!
 * SX1276 RESET I/O definitions
 */

#if 0

#define RESET_IOPORT                                GPIOA
#define RESET_PIN                                   GPIO_Pin_1

/*!
 * SX1276 SPI NSS I/O definitions
 */
#define NSS_IOPORT                                  GPIOA
#define NSS_PIN                                     GPIO_Pin_4     //ԭGPIO_Pin_15

/*!
 * SX1276 DIO pins  I/O definitions
 */
#define DIO0_IOPORT                                 GPIOA
#define DIO0_PIN                                    GPIO_Pin_0

#define DIO1_IOPORT                                 GPIOB
#define DIO1_PIN                                    GPIO_Pin_1

#define DIO2_IOPORT                                 GPIOB
#define DIO2_PIN                                    GPIO_Pin_2

#define DIO3_IOPORT                                 GPIOA
#define DIO3_PIN                                    GPIO_Pin_8

#define DIO4_IOPORT                                 GPIOA
#define DIO4_PIN                                    GPIO_Pin_11

#define DIO5_IOPORT                                 GPIOA
#define DIO5_PIN                                    GPIO_Pin_12

#define RXTX_IOPORT                                 
#define RXTX_PIN                                    FEM_CTX_PIN


#define RXE_PORT       			GPIOA
#define RXE_PIN  				GPIO_Pin_2
#define RXE_CLOCK  				RCC_APB2Periph_GPIOA
#define RXE_HIGH()         		GPIO_SetBits(RXE_PORT,RXE_PIN)
#define RXE_LOW()          		GPIO_ResetBits(RXE_PORT,RXE_PIN)
#define RXE_STATE()        		GPIO_ReadOutputDataBit(RXE_PORT,RXE_PIN)

#define TXE_PORT       			GPIOA
#define TXE_PIN  				GPIO_Pin_3
#define TXE_CLOCK  				RCC_APB2Periph_GPIOA
#define TXE_HIGH()         		GPIO_SetBits(TXE_PORT,TXE_PIN)
#define TXE_LOW()          		GPIO_ResetBits(TXE_PORT,TXE_PIN)
#define TXE_STATE()        		GPIO_ReadOutputDataBit(TXE_PORT,TXE_PIN)

#endif


void Set_RF_Switch_RX(void)
{
	//RXE_HIGH();
	//TXE_LOW();
}

void Set_RF_Switch_TX(void)
{
	//RXE_LOW();
	//TXE_HIGH();
}


void SX1276InitIo( void )
{
    sx_init();
}

void SX1276SetReset( uint8_t state )
{
      sx_reset(state);
  
}

void SX1276Write( uint8_t addr, uint8_t data )
{
    SX1276WriteBuffer( addr, &data, 1 );
}

void SX1276Read( uint8_t addr, uint8_t *data )
{
    SX1276ReadBuffer( addr, data, 1 );
}

void SX1276WriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    //NSS = 0;
    //GPIO_WriteBit( NSS_IOPORT, NSS_PIN, Bit_RESET );
	vSetCs(0);

    SpiInOut( addr | 0x80 );
    for( i = 0; i < size; i++ )
    {
        SpiInOut( buffer[i] );
    }

    //NSS = 1;
   // GPIO_WriteBit( NSS_IOPORT, NSS_PIN, Bit_SET );
   vSetCs(1);
}

void SX1276ReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    //NSS = 0;
   // GPIO_WriteBit( NSS_IOPORT, NSS_PIN, Bit_RESET );
   vSetCs(0);

    SpiInOut( addr & 0x7F );

    for( i = 0; i < size; i++ )
    {
        buffer[i] = SpiInOut( 0 );
    }

    //NSS = 1;
     vSetCs(1);
    //GPIO_WriteBit( NSS_IOPORT, NSS_PIN, Bit_SET );
}

void SX1276WriteFifo( uint8_t *buffer, uint8_t size )
{
    SX1276WriteBuffer( 0, buffer, size );
}

void SX1276ReadFifo( uint8_t *buffer, uint8_t size )
{
    SX1276ReadBuffer( 0, buffer, size );
}




//射频芯片收发切换
void SX1276WriteRxTx( uint8_t txEnable )
{
    if( txEnable != 0 )
    {
		Set_RF_Switch_TX(); //单片机将射频开关芯片切换成发射状态
//        IoePinOn( FEM_CTX_PIN );
//        IoePinOff( FEM_CPS_PIN );
    }
    else
    {
		Set_RF_Switch_RX();  //单片机将射频开关芯片切换成接收状态
//        IoePinOff( FEM_CTX_PIN );
//        IoePinOn( FEM_CPS_PIN );
    }
}


