

#include "board.h"
#include "sx1276-Hal.h"
#include "spi_hal.h"
//#include "SX1278.h"



/*
#define BOARD_SX_RESET  RT_PIN(HW_GPIOA, 27)     //复位
#define BOARD_SX_SDN    RT_PIN(HW_GPIOA, 28)    //正常收发为高电平，休眠时为低电平
#define BOARD_SX_DIDO    RT_PIN(HW_GPIOA, 29)

#define BOARD_SX_PCS0   RT_PIN(HW_GPIOD, 7)
#define BOARD_SX_SCK    RT_PIN(HW_GPIOC, 19)
#define BOARD_SX_SOUT   RT_PIN(HW_GPIOC, 17)
#define BOARD_SX_SIN    RT_PIN(HW_GPIOC, 18)

*/



void sx_init(void)
{
	/*
   GPIO_QuickInit(SX_SCK_GPIO, SX_SCK_PIN, kGPIO_Mode_OPP);
   GPIO_QuickInit(SX_PCS0_GPIO, SX_PCS0_PIN, kGPIO_Mode_OPP);
   GPIO_QuickInit(SX_SIN_GPIO, SX_SIN_PIN, kGPIO_Mode_OPP);  //输出
   GPIO_QuickInit(SX_SOUT_GPIO, SX_SOUT_PIN, kGPIO_Mode_IFT);
   //GPIO_QuickInit(SX_SOUT_GPIO, SX_SOUT_PIN, kGPIO_Mode_OPP);

	 GPIO_QuickInit(SX_RESET_GPIO, SX_RESET_PIN, kGPIO_Mode_OPP); 
	 GPIO_QuickInit(SX_SDN_GPIO, SX_SDN_PIN, kGPIO_Mode_OPP); 
*/


    rt_pin_mode( BOARD_SX_RESET, PIN_MODE_OUTPUT );
    rt_pin_mode( BOARD_SX_SDN, PIN_MODE_OUTPUT );
    rt_pin_mode( BOARD_SX_DIDO, PIN_MODE_INPUT );
	
    rt_pin_mode( BOARD_SX_PCS0, PIN_MODE_OUTPUT );
    rt_pin_mode( BOARD_SX_SCK, PIN_MODE_OUTPUT );
    rt_pin_mode( BOARD_SX_SOUT, PIN_MODE_INPUT );
    rt_pin_mode( BOARD_SX_SIN, PIN_MODE_OUTPUT );

}



#define delay_SX_us()        {int i = 6; while(i--);}


void sx_reset( tRadioResetState state)
{
	if(state == RADIO_RESET_ON){
		SX_RESET_0();
	}else{
		SX_RESET_1();
	}


	SX_SDN_1();
}

static unsigned char spi_gpio_rw(unsigned char data)
{
	unsigned char i;  
	
	for(i=0; i<8; i++)          // 循环8次  
	{  
		//SET_GPIO_SPI_SCK();
		SX_SCLK_0();
		
		if(data & 0x80){		// byte最高位输出到MOSI  
			//SET_GPIO_SPI_SI();
			SX_DIN_1();
		}
		else{ 
			//RESET_GPIO_SPI_SI();
			SX_DIN_0();
		}
		
		data <<= 1;             // 低一位移位到最高位  
                
                

		//RESET_GPIO_SPI_SCK();	// 拉高SCK，nRF24L01从MOSI读入1位数据，同时从MISO输出1位数据  
		SX_SCLK_1();
                delay_SX_us();

		if(SX_DOUT_READ()){
			data |= 1;           // 读MISO到byte最低位  
		}
		//SET_GPIO_SPI_SCK();               // SCK置低  
		SX_SCLK_0();
                delay_SX_us();
	}  
	
	return(data);               // 返回读出的一字节  
}


uint8_t SpiInOut( uint8_t outData )
{
	return spi_gpio_rw(outData);
}

void vSetCs(uint8_t stat)
{
	if(stat == 0){
		 SX_CS_0() ;
	}else{
		 SX_CS_1() ;
	}
}

/*

static void SXReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;
    //NSS = 0;
    SX_CS_0() ;
    spi_gpio_rw( addr & 0x7F );
    for ( i = 0; i < size; i++ )
    {
        buffer[i] = spi_gpio_rw(0x00);
    }
    SX_CS_1() ;
}

void SXReadReg( uint8_t addr, uint8_t *data )
{
    SXReadBuffer( addr, data, 1 );

}

static void SXWriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;
    SX_CS_0() ;
    spi_gpio_rw(addr|0x80);
    for ( i = 0; i < size; i++ )
    {
        spi_gpio_rw( buffer[i] );
    }
    SX_CS_1() ;
}

void SXWriteReg( uint8_t addr, uint8_t data )
{
    SXWriteBuffer( addr, &data, 1 );
}*/
