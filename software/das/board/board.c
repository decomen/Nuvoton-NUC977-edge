/*
 * File      : board.c
 */

#include "board.h"

s_Rs232_Rs485_Stat gUartType;

void rt_hw_board_init(void) {
    threaddog_init();
    vTestRs232Rs485(&gUartType);
    if (gUartType.eUart0Type == UART_TYPE_232) {
        rt_kprintf(" uart 0 232\r\n"); 
    } else {
        rt_kprintf(" uart 0 485\r\n");
    }

    if (gUartType.eUart1Type == UART_TYPE_232) {
        rt_kprintf(" uart 1 232\r\n");
    } else {
        rt_kprintf(" uart 1 485\r\n");
    }
    //vAd7689Init();
    //vInOutInit();
}

#define DEV_NAME		"/dev/dm_io"
#define TTY_UART_CTL    _IO('T',100)
#define TTYS1_TEST  0
#define TTYS2_TEST  1

typedef struct {
	unsigned char gpio;
	unsigned char dir; //0 input 1 output
	unsigned char val;
}dm_io_t;

void vTestRs232Rs485(s_Rs232_Rs485_Stat *pState) 
{
	int fd = -1;	
	fd = open(DEV_NAME, O_RDWR);
	if (fd == -1) {		
		printf("Cannot open %s!\n",DEV_NAME);		
		return;	
	}
    
	dm_io_t iodata;
    
	iodata.gpio = TTYS1_TEST;
	iodata.dir = 0;
	iodata.val = 1;
    if (ioctl(fd, TTY_UART_CTL, &iodata) < 0) {	
        printf("TTY_UART_CTL set fail\n");
    }
    if (iodata.val == 1) {
		pState->eUart1Type = UART_TYPE_232;
    } else {
		pState->eUart1Type = UART_TYPE_485;
    }

    
    iodata.gpio = TTYS2_TEST;
	iodata.dir = 0;
	iodata.val = 1;
    if (ioctl(fd, TTY_UART_CTL,&iodata) < 0) {   
		printf("TTY_UART_CTL set fail\n");
    }
    if (iodata.val == 1) {
		pState->eUart0Type = UART_TYPE_232;
    } else {
		pState->eUart0Type = UART_TYPE_485;
    }
	close(fd);
}



//模块检测定义
/*

MODULE_1  MODULE_2   
  1         0        EC20   (4G)
  0         1        NB-IOT 
  1         1        合宙4G模块AIR720 
  0         0        目前是空载     或(2G)

*/
#define MODULE_SELECT_CTL    _IO('M',100)
#define MODULE_1  0
#define MODULE_2  1

eCELLNetTYPE_t g_xCellNetType = E_GPRS_M26;

eCELLNetTYPE_t vCheckCellNetType(void)
{

    s_io_t iodata, iodata1;
    iodata.gpio = MODULE_1;
    iodata.dir = 0;
    iodata.val = -1;
    das_do_io_ctrl(MODULE_SELECT_CTL, &iodata);

    iodata1.gpio = MODULE_2;
    iodata1.dir = 0;
    iodata1.val = -1;
    das_do_io_ctrl(MODULE_SELECT_CTL, &iodata1);

  //  printf("iodata.val: %d, iodata1.val: %d\r\n",iodata.val,iodata1.val);

     g_xCellNetType = E_4G_EC200S;
     return E_4G_EC200S;

  //  return E_4G_EC20;
    
    if( (iodata.val==1) && (iodata1.val==0) ){
        g_xCellNetType = E_4G_EC20;
        return E_4G_EC20;
    }else if( (iodata.val==0) && (iodata1.val==1) ){
        g_xCellNetType = E_NBIOT_BC26;
        return E_NBIOT_BC26;
    }else if( (iodata.val==1) && (iodata1.val==1) ){
        g_xCellNetType = E_AIR720;
        return E_AIR720;
    }else if( (iodata.val==0) && (iodata1.val==0) ){
        g_xCellNetType = E_GPRS_M26;
        return E_GPRS_M26;
    }
    
    /*if(access("/dev/ttyUSB0", R_OK) == 0) {
       // rt_kprintf("\n============= 4G MODULE EC20 IS CHECK!\n");
        g_xCellNetType = E_4G_EC20;
        return E_4G_EC20;
    } else if(access("/dev/ttyUSB2", R_OK) == 0) {
        //rt_kprintf("\n============= 2G MODULE M26 IS CHECK!\n");
        g_xCellNetType = E_GPRS_M26;
        return E_GPRS_M26;
    } else {
        //rt_kprintf("\n============= NB MODULE BC95 IS CHECK!\n");
        g_xCellNetType = E_NBIOT_BC26;
        return E_NBIOT_BC26;
    }*/
    
}

