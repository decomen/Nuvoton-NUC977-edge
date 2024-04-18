/* RT-Thread config file */
#ifndef __RTTHREAD_CFG_H__
#define __RTTHREAD_CFG_H__

//#define RT_USING_LUA
//#define LUA_USE_STDIO

//#define my_system(cmd)              do { printf("%s:%d  (%s)\n", __FUNCTION__, __LINE__, cmd); system(cmd); } while(0)

#define TEST_ON_PC                  0

#define my_system(cmd)              system(cmd)

#define BOARD_BASE_PATH             "/media/nand"
#define BOARD_DATA_PATH             BOARD_BASE_PATH"/data/"
#define BOARD_CFG_PATH              BOARD_BASE_PATH"/cfg/"
//#define BOARD_UPDATE_PATH           BOARD_BASE_PATH"/update/"
#define BOARD_LOG_PATH              BOARD_BASE_PATH"/log/"
#define BOARD_MONITOR_PATH          BOARD_BASE_PATH"/monitor/"

/* RT_NAME_MAX*/
#define RT_NAME_MAX                 24

/* RT_ALIGN_SIZE*/
#define RT_ALIGN_SIZE               4

/* Tick per Second */
#define RT_TICK_PER_SECOND          1000

#define MB_ADDRESS_MIN              1
#define MB_ADDRESS_MAX              247

#define NET_HAS_GPRS                (1)

#define ENET_CNT                    1
#define BOARD_UART_MAX                  5
#define BOARD_ENET_TCPIP_NUM            12

#if NET_HAS_GPRS
#define BOARD_GPRS_TCPIP_NUM            8
#else
#define BOARD_GPRS_TCPIP_NUM            0
#endif

#define BOARD_TCPIP_MAX                 (BOARD_ENET_TCPIP_NUM+BOARD_GPRS_TCPIP_NUM)

#define BOARD_COLL_MAX                  (BOARD_UART_MAX + BOARD_TCPIP_MAX)

#endif
