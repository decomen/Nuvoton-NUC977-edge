

#ifndef _TEST_H
#define _TEST_H

#define TEST_UART_NAME   "/dev/ttyS7"

/*

#define TEST_ATE         _IO('A',103)
#define TEST_LED         _IO('A',104)
#define TEST_MODE        _IO('A',105)   //判断是否进入测试模式
#define UPDATE_LED       _IO('A',106)   //升级指示灯

*/


#define FACTORY_KEY         _IO('A',103)     //按键，按键10s恢复出厂设置
#define TEST_MODE           _IO('A',105)     //判断是否进入测试模式
#define UPDATE_LED          _IO('A',106)     //升级指示灯


#define TEST_QUEUE_SIZE (1* 1024 + 1)

typedef struct {
    mdINT nFront; 	/* 头指针 */
    mdINT nRear;  	/* 尾指针，若队列不空，指向队列尾元素的下一个位置 */
    mdBYTE pData[TEST_QUEUE_SIZE];	/* 可以是其他基本类型 */
} queue_test_t;

// 存放MYSIZEOF(S_PACKAGE_HEAD)个元素需要MYSIZEOF(S_PACKAGE_HEAD) + 1 的空间
#define QUEUE_HEADE_SIZE (sizeof(S_PACKAGE_HEAD) + 1)

typedef struct {
    mdINT nFront;       /* 头指针 */
    mdINT  nRear;        /* 尾指针，若队列不空，指向队列尾元素的下一个位置 */
    mdBYTE pData[QUEUE_HEADE_SIZE];
} queue_head_t;

extern queue_test_t *p_com_queue; 
extern queue_head_t g_queue_com_head ;
void USART3_IRQHandler(void) ;  

void set_product_info(const S_MSG *msg, byte_buffer_t *bb);

void vTestLedToggle();
void vTestRtc(const S_MSG *msg, byte_buffer_t *bb);
void vTestSpiFlash(const S_MSG *msg, byte_buffer_t *bb);
void vTestSdCard(const S_MSG *msg, byte_buffer_t *bb);
void vTestVs1003(const S_MSG *msg, byte_buffer_t *bb);
void vTestGprs(const S_MSG *msg, byte_buffer_t *bb);
void vTestZigbee(const S_MSG *msg, byte_buffer_t *bb);
void vTestNet(const S_MSG *msg, byte_buffer_t *bb);
void vTestTTLInput(const S_MSG *msg, byte_buffer_t *bb);

void vTestGetAdc(const S_MSG *msg, byte_buffer_t *bb);
void vTestGetTestAdcValue(const S_MSG *msg, byte_buffer_t *bb);
void vTestUart(const S_MSG *msg, byte_buffer_t *bb);
void vTestSetCheckDefault(const S_MSG *msg, byte_buffer_t *bb);
void vTestSetAdc(const S_MSG *msg, byte_buffer_t *bb);

void vTestGetCalValue(const S_MSG *msg, byte_buffer_t *bb);
void vTestRelays(const S_MSG *msg, byte_buffer_t *bb);
void vTestLora(const S_MSG *msg, byte_buffer_t *bb);

int KeyIoStatus(void);


rt_err_t xTestTaskStart( void );
extern mdBOOL g_bIsTestMode;

typedef enum
{
    HW_UART0 = 0x00,
    HW_UART1 ,
    HW_COM ,
    HW_MAX ,
}e_UART_T;


typedef struct 
{
    int fd;
    queue_test_t queue;
}s_TestUartInfo;


int vIsTestModeIo(void);

void vUartSend(uint32_t instance, const char *buf,int len);


#endif 
