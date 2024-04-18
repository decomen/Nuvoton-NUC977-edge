
#include <board.h>
#include "modbus.h"
#include "modbus_helper.h"

#define DM_ZIGBEE_CTL       _IO('Z',100)
#define ZIGBEE_RESET        0
#define ZIGBEE_DEF          1

#define ZIGBEE_CMD_BUFFER_SIZE (ZGB_STD_FA_SIZE)

typedef struct {
    int len;
    int pos;
    rt_uint8_t buf[ZGB_STD_FA_SIZE];
} zgb_buf_queue_t;

static zgb_buf_queue_t s_zgb_rcv_queue;
static rt_thread_t s_zgb_parse_thread = RT_NULL;
static rt_thread_t s_zgb_worker_thread = RT_NULL;
static rt_mq_t s_zgb_cmd_queue = RT_NULL;
static rt_mq_t s_zgb_buf_queue = RT_NULL;
static volatile int s_zgb_incmd_hold = 0;
static volatile rt_bool_t s_zgb_incmd = RT_FALSE;
static pthread_mutex_t s_zgb_mutex;
static int s_zgb_timeout = 1000;
static serial_t *s_zgb_serial = NULL;
static bfifo_t s_zgb_fifo = NULL;

static rt_bool_t __is_std_head(rt_uint8_t *data, int len);
static rt_bool_t __is_tempcmd_head(rt_uint8_t *data, int len);
static rt_bool_t __is_cmd_head(rt_uint8_t *data, int len);

#define zigbee_htons(_n)    ((((_n)&0xff)<<8)|(((_n)&0xff00)>>8))
#define zigbee_ntohs(_n)    zigbee_htons(_n)

#define ZIGBEE_CMD_WAIT     ( RT_TICK_PER_SECOND )

#define ZIGBEE_BUFFER_CHECK_SUM(_buffer,_n) do { \
        rt_uint8_t i, sum = 0; \
        for (i = 0; i < (_n) - 1; i++) { \
            sum += _buffer[i]; \
        } \
            _buffer[(_n) - 1] = sum; \
    } while (0)

#define ZIGBEE_SEND_BUFFER(_buffer,_n) do { \
        serial_helper_send(BOARD_ZGB_UART, (const void *)_buffer, _n); \
    } while (0)

#define sZigbeeEnterConfigMode() \
do { \
    rt_mutex_take( &s_zgb_mutex, RT_WAITING_FOREVER ); \
    s_zgb_incmd_hold++;     \
    s_zgb_incmd = RT_TRUE; \
    b_mq_reset(s_zgb_cmd_queue); \
} while (0)

#define sZigbeeExitConfigMode() \
do { \
    s_zgb_incmd_hold--; \
    if( 0 == s_zgb_incmd_hold ) { s_zgb_incmd = RT_FALSE; } \
    rt_mutex_release( &s_zgb_mutex ); \
} while (0)

rt_bool_t bZigbeeInCmd(void)
{
    return s_zgb_incmd;
}

void vZigbeeHWReset(void)
{
    s_io_t iodata = {.gpio = ZIGBEE_RESET, .dir = 1, .val = 0};
    das_do_io_ctrl(DM_ZIGBEE_CTL, &iodata);
    rt_thread_delay(20);
    iodata.val = 1;
    das_do_io_ctrl(DM_ZIGBEE_CTL, &iodata);
}

static void _zgbparse_thread(void *parameter);
static void _zgbworker_thread(void *parameter);

static rt_bool_t _in_parse = RT_FALSE;

static void __zgb_recv_buffer(int port, void *buffer, int size)
{
    _in_parse = RT_TRUE;
    s_zgb_rcv_queue.pos = 0;
    memcpy(s_zgb_rcv_queue.buf, buffer, size);
    s_zgb_rcv_queue.len = size;

    if (bZigbeeInCmd() &&
        (__is_tempcmd_head(s_zgb_rcv_queue.buf, s_zgb_rcv_queue.len) ||
         __is_cmd_head(s_zgb_rcv_queue.buf, s_zgb_rcv_queue.len))
       ) {
        //xfer_dst_serial_rcv_clear(BOARD_ZGB_UART);
        rt_mq_send(s_zgb_cmd_queue, &s_zgb_rcv_queue, sizeof(zgb_buf_queue_t));
    } else if (__is_std_head(s_zgb_rcv_queue.buf, s_zgb_rcv_queue.len)) {
        //xfer_dst_serial_rcv_clear(BOARD_ZGB_UART);
        rt_mq_send(s_zgb_buf_queue, &s_zgb_rcv_queue, sizeof(zgb_buf_queue_t));
    } else {
        if (g_xfer_net_dst_uart_occ[BOARD_ZGB_UART]) {
            if(ZGB_TM_GW == g_zigbee_cfg.tmode) {
                rt_thddog_feed("xfer_dst_serial_recv_buffer");
                xfer_dst_serial_recv_buffer(BOARD_ZGB_UART, (void *)s_zgb_rcv_queue.buf, s_zgb_rcv_queue.len);
            }
        } else if(ZGB_TM_TRT == g_zigbee_cfg.tmode) {
            rt_int8_t instance = nUartGetInstance(g_zigbee_cfg.dst_type);
            if(instance >= 0) {
                rt_thddog_feed("xfer_dst_serial_send -> trt");
                xfer_dst_serial_send(instance, (const void *)s_zgb_rcv_queue.buf, s_zgb_rcv_queue.len);
                goto _END;
            }
        } else {
            // RTU 主从校验方法一致
            if (s_zgb_rcv_queue.len >= MB_RTU_PDU_SIZE_MIN &&
                s_zgb_rcv_queue.len < MB_RTU_PDU_SIZE_MAX &&
                (das_crc16((uint8_t *)&s_zgb_rcv_queue.buf[0], s_zgb_rcv_queue.len) == 0)) {
                // 目前只考虑单网关树形网络  2016/10/20

                UCHAR ucAddress = s_zgb_rcv_queue.buf[MB_RTU_PDU_ADDR_OFF];

                if (ZIGBEE_WORK_END_DEVICE == g_zigbee_cfg.xInfo.WorkMode ||
                    ZIGBEE_WORK_ROUTER == g_zigbee_cfg.xInfo.WorkMode) {
                    if(ZGB_TM_DTU == g_zigbee_cfg.tmode) {
                        int instance = xfer_get_uart_with_slave_addr(ucAddress);
                        if(instance >= 0) {
                            rt_thddog_feed("xfer_dst_serial_send -> dtu");
                            xfer_dst_serial_send(instance, (const void *)s_zgb_rcv_queue.buf, s_zgb_rcv_queue.len);
                            goto _END;
                        }
                    }
                    //vMBSetAddress(BOARD_ZGB_UART, ucAddress);
                    //xMBRTUPostReceived(BOARD_ZGB_UART, (UCHAR *)s_zgb_rcv_queue.buf, (USHORT)s_zgb_rcv_queue.len);
                    //modbus_set_slave(g_mb_ctx_uart[port], ucAddress);
                    bfifo_push(s_zgb_fifo, (const unsigned char *)s_zgb_rcv_queue.buf, s_zgb_rcv_queue.len, 0);
                } else if (ZIGBEE_WORK_COORDINATOR == g_zigbee_cfg.xInfo.WorkMode) {
                    //if (bMBMasterWaitRsp(BOARD_ZGB_UART) && (ucAddress == ucMBMasterGetDestAddress(BOARD_ZGB_UART))) {
                        // RTU主机: 收到回复
                    //    xMBMasterRTUPostReceived(BOARD_ZGB_UART, (UCHAR *)s_zgb_rcv_queue.buf, (USHORT)s_zgb_rcv_queue.len);
                    //}
                    bfifo_push(s_zgb_fifo, (const unsigned char *)s_zgb_rcv_queue.buf, s_zgb_rcv_queue.len, 0);
                }
            } else {
                rt_kprintf("_zgb_serial_timeout_handle : other data!\n");
            }
        }
    }

_END:

    s_zgb_rcv_queue.pos = 0;
    s_zgb_rcv_queue.len = 0;
    _in_parse = RT_FALSE;
}

static void __zgb_reopen(int port)
{
    serial_helper_cfg(port, &g_uart_cfgs[port].port_cfg);
    serial_helper_open(port);
}

static void _zgbworker_thread(void *parameter)
{
    while (1) {
        int port = BOARD_ZGB_UART;
        uint8_t buffer[4096] = {0};
        while (serial_helper_is_open(port)) {
            int pos = 0;
            int s_rc = serial_helper_select(port, -1);
            int n = serial_helper_recv(port, buffer, sizeof(buffer));
            if (n > 0) {
                pos = n;
                while (1) {
                    s_rc = serial_helper_select(port, s_zgb_timeout * 1000);
                    if (s_rc == -1) {
                        usleep(100 * 1000);
                        break;
                    } else if (s_rc == 0) {
                        __zgb_recv_buffer(port, buffer, pos);
                        break;
                    } else {
                        n = serial_helper_recv(port, &buffer[pos], sizeof(buffer) - pos);
                        pos += n;
                        if (pos >= sizeof(buffer)) {
                            __zgb_recv_buffer(port, buffer, pos);
                            break;
                        }
                    }
                }
            } else if (n <= 0) {
                if (n < 0 && errno == EBADF) __zgb_reopen(port);
                rt_thread_delay(500);
            }
        }
        rt_thread_delay(1000);
    }
}

BOOL bZigbeeInit(BOOL uart_default)
{
    BOOL bStatus = FALSE;
    s_io_t iodata = {.gpio = ZIGBEE_DEF, .dir = 1, .val = 1};
    das_do_io_ctrl(DM_ZIGBEE_CTL, &iodata);

    s_zgb_fifo = bfifo_create(4096);
    
    vZigbeeHWReset();

    zgb_std_init();

    if (rt_mutex_init(&s_zgb_mutex, "zgbmtx", RT_IPC_FLAG_PRIO) != RT_EOK) {
        rt_kprintf("init zgbmtx failed\n");
        return RT_FALSE;
    }

    s_zgb_serial = g_serials[BOARD_ZGB_UART];

    if (uart_default) {
        s_zgb_serial->cfg.baud_rate = 115200;
        s_zgb_serial->cfg.data_bits = 8;
        s_zgb_serial->cfg.stop_bits = 1;
        s_zgb_serial->cfg.parity = 0;
    }
    serial_helper_open(BOARD_ZGB_UART);

    {
        rt_tick_t timer_tick = 1000;
        rt_uint16_t usTimerT35_50us = 0;

        if (s_zgb_serial->cfg.baud_rate > 19200) {
            usTimerT35_50us = 35;
        } else {
            usTimerT35_50us = (7UL * 220000UL) / (2UL * s_zgb_serial->cfg.baud_rate);
        }
        s_zgb_timeout = ((50 * usTimerT35_50us) / (1000.0 * 1000 / RT_TICK_PER_SECOND) + 0.5);
    }

    if ((s_zgb_cmd_queue = rt_mq_create("zgbcmdq", sizeof(zgb_buf_queue_t), 1, RT_IPC_FLAG_PRIO)) != RT_NULL) {
        ;
    } else {
        rt_kprintf("zigbee rt_mq_create zgbcmdq falied..\n");
        return RT_FALSE;
    }

    if ((s_zgb_buf_queue = rt_mq_create("zgbbufq", sizeof(zgb_buf_queue_t), 1, RT_IPC_FLAG_PRIO)) != RT_NULL) {
        ;
    } else {
        rt_kprintf("zigbee rt_mq_create zgbbufq falied..\n");
        return RT_FALSE;
    }

    s_zgb_parse_thread = rt_thread_create("zgbparse", _zgbparse_thread, RT_NULL, 0x400, 10, 10);
    if (s_zgb_parse_thread != RT_NULL) {
        rt_thddog_register(s_zgb_parse_thread, 30);
        rt_thread_startup(s_zgb_parse_thread);
    }

    s_zgb_worker_thread = rt_thread_create("zgbworker", _zgbworker_thread, RT_NULL, 0x400, 10, 10);
    if (s_zgb_worker_thread != RT_NULL) {
        rt_thddog_register(s_zgb_worker_thread, 30);
        rt_thread_startup(s_zgb_worker_thread);
    }
    return RT_TRUE;
}

static rt_bool_t __is_std_head(rt_uint8_t *data, int len)
{
    rt_uint32_t pre = 0; memcpy(&pre, &data[0], 4);
    rt_uint32_t npre = 0; memcpy(&npre, &data[4], 4);
    return (
            len >= sizeof(zgb_std_head_t) &&
            pre == ZGB_STD_PRE &&
            npre == ZGB_STD_NPRE
           );
}

static rt_bool_t zigbeeMatchStdHead(zgb_buf_queue_t *buf)
{
    if (__is_std_head(&buf->buf[buf->pos], buf->len - buf->pos)) {
        buf->pos += sizeof(zgb_std_head_t);
        return RT_TRUE;
    }
    return RT_FALSE;
}

// 尝试匹配协议头
static zgb_buf_queue_t _cmdqueue;

static rt_bool_t __is_tempcmd_head(rt_uint8_t *data, int len)
{
    return (
            len >= 4 &&
            data[0] == ZIGBEE_TEMP_PRECODE_0 &&
            data[1] == ZIGBEE_TEMP_PRECODE_1 &&
            data[2] == ZIGBEE_TEMP_PRECODE_2
           );
}

static BOOL prvRecvTempHeadWithTime(UCHAR cmdcode, int timeout)
{
    rt_tick_t ulStartTick = rt_tick_get();

    while (1) {
        int diff = timeout - (int)(rt_tick_get() - ulStartTick);
        if (diff > 0 && RT_EOK == rt_mq_recv(s_zgb_cmd_queue, &_cmdqueue, sizeof(zgb_buf_queue_t), diff)) {
            if (__is_tempcmd_head(&_cmdqueue.buf[_cmdqueue.pos], _cmdqueue.len - _cmdqueue.pos) &&
                _cmdqueue.buf[_cmdqueue.pos + 3] == cmdcode
               ) {
                _cmdqueue.pos += 4;
                return RT_TRUE;
            }
        } else {
            return RT_FALSE;
        }
        if (diff <= 0) return RT_FALSE;
    }
    return RT_FALSE;
}

static BOOL prvRecvTempHead(UCHAR cmdcode)
{
    return prvRecvTempHeadWithTime(cmdcode, RT_TICK_PER_SECOND);
}

static rt_bool_t __is_cmd_head(rt_uint8_t *data, int len)
{
    return (
            len >= 4 &&
            data[0] == ZIGBEE_PRECODE_0 &&
            data[1] == ZIGBEE_PRECODE_1 &&
            data[2] == ZIGBEE_PRECODE_2
           );
}

// 尝试匹配协议头
static BOOL zigbeeRecvHead(UCHAR cmdcode)
{
    rt_tick_t ulStartTick = rt_tick_get();
    while (1) {
        int diff = RT_TICK_PER_SECOND - (int)(rt_tick_get() - ulStartTick);
        if (diff > 0 && RT_EOK == rt_mq_recv(s_zgb_cmd_queue, &_cmdqueue, sizeof(zgb_buf_queue_t), diff)) {
            if (__is_cmd_head(&_cmdqueue.buf[_cmdqueue.pos], _cmdqueue.len - _cmdqueue.pos) &&
                _cmdqueue.buf[_cmdqueue.pos + 3] == cmdcode
               ) {
                _cmdqueue.pos += 4;
                return RT_TRUE;
            }
        } else {
            return RT_FALSE;
        }
        if (diff <= 0) return RT_FALSE;
    }
    return RT_FALSE;
}

static BOOL zigbeeRecvData(void *data, UCHAR len)
{
    if (_cmdqueue.len - _cmdqueue.pos >= len) {
        memcpy(data, &_cmdqueue.buf[_cmdqueue.pos], len);
        _cmdqueue.pos += len;
        return RT_TRUE;
    }
    return RT_FALSE;
}

// 修改通道号
UCHAR ucZigbeeSetChan(UCHAR chan)
{
    UCHAR ret = ZIGBEE_FAILED;
    UCHAR buffer[ZIGBEE_HEAD_LEN + 1] =
    {
        ZIGBEE_TEMP_PRECODE,
        ZIGBEE_TEMP_CMD_SET_CHAN,
        chan
    };
    sZigbeeEnterConfigMode();

    ZIGBEE_SEND_BUFFER(buffer, sizeof(buffer));
    if (prvRecvTempHead(ZIGBEE_TEMP_CMD_SET_CHAN)) {
        UCHAR data;
        if (zigbeeRecvData(&data, 1)) {
            ret = data;
        }
    }

    sZigbeeExitConfigMode();

    return ret;
}

// 修改目的网络
UCHAR ucZigbeeSetDstAddr(USHORT Addr)
{
    UCHAR ret = ZIGBEE_FAILED;
    static USHORT s_usAddr = 0xFFFF;
    if (s_usAddr != Addr) {
        UCHAR buffer[ZIGBEE_HEAD_LEN + 2] =
        {
            ZIGBEE_TEMP_PRECODE,
            ZIGBEE_TEMP_CMD_SET_DST_ADDR,
            (Addr >> 8) & 0xFF,
            Addr & 0xFF
        };
        sZigbeeEnterConfigMode();

        ZIGBEE_SEND_BUFFER(buffer, sizeof(buffer));
        if (prvRecvTempHead(ZIGBEE_TEMP_CMD_SET_DST_ADDR)) {
            UCHAR data;
            if (zigbeeRecvData(&data, 1)) {
                ret = data;
            }
        }

        sZigbeeExitConfigMode();
        if (ZIGBEE_OK == ret) {
            s_usAddr = Addr;
        }
    } else {
        s_usAddr = Addr;
        ret = ZIGBEE_OK;
    }
    return ret;
}

// 包头显示源地址
UCHAR ucZigbeeShowSrcAddr(BOOL show)
{
    UCHAR ret = ZIGBEE_FAILED;
    UCHAR buffer[ZIGBEE_HEAD_LEN + 1] =
    {
        ZIGBEE_TEMP_PRECODE,
        ZIGBEE_TEMP_CMD_SHOW_SRC_ADDR,
        (show ? 0x01 : 0x00)
    };
    sZigbeeEnterConfigMode();

    ZIGBEE_SEND_BUFFER(buffer, sizeof(buffer));
    if (prvRecvTempHead(ZIGBEE_TEMP_CMD_SHOW_SRC_ADDR)) {
        UCHAR data;
        if (zigbeeRecvData(&data, 1)) {
            ret = data;
        }
    }

    sZigbeeExitConfigMode();
    return ret;
}

// 设置GPIO输入输出方向
UCHAR ucZigbeeGPIOConfig(USHORT Addr, UCHAR cfg)
{
    UCHAR ret = ZIGBEE_FAILED;
    UCHAR buffer[ZIGBEE_HEAD_LEN + 2 + 1] =
    {
        ZIGBEE_TEMP_PRECODE,
        ZIGBEE_TEMP_CMD_GPIO_CONFIG,
        (Addr >> 8) & 0xFF,
        Addr & 0xFF,
        cfg
    };
    sZigbeeEnterConfigMode();

    ZIGBEE_SEND_BUFFER(buffer, sizeof(buffer));
    if (prvRecvTempHead(ZIGBEE_TEMP_CMD_GPIO_CONFIG)) {
        UCHAR data;
        if (zigbeeRecvData(buffer + ZIGBEE_HEAD_LEN, 2)
            && zigbeeRecvData(&data, 1)) {
            ret = data;
        }
    }

    sZigbeeExitConfigMode();
    return ret;
}

// 读取GPIO
UCHAR ucZigbeeGPIORead(USHORT Addr, UCHAR *pGpio)
{
    UCHAR ret = ZIGBEE_FAILED;
    UCHAR buffer[ZIGBEE_HEAD_LEN + 2] =
    {
        ZIGBEE_TEMP_PRECODE,
        ZIGBEE_TEMP_CMD_GPIO_READ,
        (Addr >> 8) & 0xFF,
        Addr & 0xFF,
    };
    sZigbeeEnterConfigMode();

    ZIGBEE_SEND_BUFFER(buffer, sizeof(buffer));
    if (prvRecvTempHead(ZIGBEE_TEMP_CMD_GPIO_READ)) {
        if (zigbeeRecvData(buffer + ZIGBEE_HEAD_LEN, 2)
            && zigbeeRecvData(pGpio, 1)) {
            ret = ZIGBEE_OK;
        }
    }

    sZigbeeExitConfigMode();
    return ret;
}

// 设置GPIO电平
UCHAR ucZigbeeGPIOWrite(USHORT Addr, UCHAR gpio)
{
    UCHAR ret = ZIGBEE_FAILED;
    UCHAR buffer[ZIGBEE_HEAD_LEN + 2 + 1] =
    {
        ZIGBEE_TEMP_PRECODE,
        ZIGBEE_TEMP_CMD_GPIO_WRITE,
        (Addr >> 8) & 0xFF,
        Addr & 0xFF,
        gpio
    };
    sZigbeeEnterConfigMode();

    ZIGBEE_SEND_BUFFER(buffer, sizeof(buffer));
    if (prvRecvTempHead(ZIGBEE_TEMP_CMD_GPIO_WRITE)) {
        UCHAR data;
        if (zigbeeRecvData(buffer + ZIGBEE_HEAD_LEN, 2)
            && zigbeeRecvData(&data, 1)) {
            ret = data;
        }
    }

    sZigbeeExitConfigMode();
    return ret;
}

// 读取AD
UCHAR ucZigbeeAD(USHORT Addr, UCHAR channel, USHORT *pADValue)
{
    UCHAR ret = ZIGBEE_FAILED;
    UCHAR buffer[ZIGBEE_HEAD_LEN + 2 + 1] =
    {
        ZIGBEE_TEMP_PRECODE,
        ZIGBEE_TEMP_CMD_AD,
        (Addr >> 8) & 0xFF,
        Addr & 0xFF,
        channel
    };
    sZigbeeEnterConfigMode();

    ZIGBEE_SEND_BUFFER(buffer, sizeof(buffer));
    if (prvRecvTempHead(ZIGBEE_TEMP_CMD_AD)) {
        if (zigbeeRecvData(buffer + ZIGBEE_HEAD_LEN, 2)
            && zigbeeRecvData(pADValue, 2)) {
            ret = ZIGBEE_OK;
        }
    }

    sZigbeeExitConfigMode();
    return ret;
}

// 休眠
UCHAR ucZigbeeSleep()
{
    UCHAR buffer[ZIGBEE_HEAD_LEN + 1] =
    {
        ZIGBEE_TEMP_PRECODE,
        ZIGBEE_TEMP_CMD_SLEEP,
        0x01
    };
    sZigbeeEnterConfigMode();

    ZIGBEE_SEND_BUFFER(buffer, sizeof(buffer));

    sZigbeeExitConfigMode();

    return ZIGBEE_OK;
}

// 设置通讯模式
UCHAR ucZigbeeMode(ZIGBEE_MSG_MODE_E mode)
{
    UCHAR ret = ZIGBEE_FAILED;
    UCHAR buffer[ZIGBEE_HEAD_LEN + 1] =
    {
        ZIGBEE_TEMP_PRECODE,
        ZIGBEE_TEMP_CMD_MODE,
        mode
    };
    sZigbeeEnterConfigMode();

    ZIGBEE_SEND_BUFFER(buffer, sizeof(buffer));
    if (prvRecvTempHead(ZIGBEE_TEMP_CMD_MODE)) {
        UCHAR data;
        if (zigbeeRecvData(&data, 1)) {
            ret = data;
        }
    }

    sZigbeeExitConfigMode();
    return ret;
}

// 获取节点信号强度
UCHAR ucZigbeeRSSI(USHORT Addr, UCHAR *pRssi)
{
    UCHAR RetAddr[2] = { 0 };
    UCHAR ret = ZIGBEE_FAILED;
    UCHAR buffer[ZIGBEE_HEAD_LEN + 2] =
    {
        ZIGBEE_TEMP_PRECODE,
        ZIGBEE_TEMP_CMD_RSSI,
        (Addr >> 8) & 0xFF,
        Addr & 0xFF
    };
    sZigbeeEnterConfigMode();

    ZIGBEE_SEND_BUFFER(buffer, sizeof(buffer));
    if (prvRecvTempHeadWithTime(ZIGBEE_TEMP_CMD_RSSI, 3000)) {
        if (zigbeeRecvData(RetAddr, 2) &&
            (RetAddr[0] == ((Addr >> 8) & 0xFF) && RetAddr[1] == (Addr & 0xFF))) {
            if (zigbeeRecvData(pRssi, 1)) {
                ret = ZIGBEE_OK;
            }
        }
    }

    sZigbeeExitConfigMode();
    return ret;
}

// 读取本地配置命令
ZIGBEE_ERR_E eZigbeeGetDevInfo(ZIGBEE_DEV_INFO_T *pInfo, UCHAR *pState, USHORT *pType, USHORT *pVer)
{
    ZIGBEE_ERR_E ret = ZIGBEE_ERR_OTHER;

    UCHAR buffer[ZIGBEE_HEAD_LEN + 1] =
    {
        ZIGBEE_PRECODE,
        ZIGBEE_CMD_GET_DEV_INFO
    };
    sZigbeeEnterConfigMode();

    ZIGBEE_BUFFER_CHECK_SUM(buffer, sizeof(buffer));
    ZIGBEE_SEND_BUFFER(buffer, sizeof(buffer));
    if (zigbeeRecvHead(ZIGBEE_CMD_GET_DEV_INFO)) {
        if (zigbeeRecvData(pInfo, sizeof(ZIGBEE_DEV_INFO_T))
            && zigbeeRecvData(pState, 1)
            && zigbeeRecvData(pType, 2)
            && zigbeeRecvData(pVer, 2)) {
            ret = ZIGBEE_ERR_OK;
        }
    }

    sZigbeeExitConfigMode();

    if (ZIGBEE_ERR_OK == ret) {
        pInfo->PanID = zigbee_htons(pInfo->PanID);
        pInfo->Addr = zigbee_htons(pInfo->Addr);
        pInfo->DstAddr = zigbee_htons(pInfo->DstAddr);
        *pType = zigbee_htons(*pType);
        *pVer = zigbee_htons(*pVer);
    }

    return ret;
}

// 修改通道号
ZIGBEE_ERR_E eZigbeeSetChan(UCHAR chan)
{
    ZIGBEE_ERR_E ret = ZIGBEE_ERR_OTHER;

    UCHAR buffer[ZIGBEE_HEAD_LEN + 1 + 1] =
    {
        ZIGBEE_PRECODE,
        ZIGBEE_CMD_SET_CHAN,
        chan
    };
    sZigbeeEnterConfigMode();

    ZIGBEE_BUFFER_CHECK_SUM(buffer, sizeof(buffer));
    ZIGBEE_SEND_BUFFER(buffer, sizeof(buffer));
    if (zigbeeRecvHead(ZIGBEE_CMD_SET_CHAN)) {
        UCHAR data;
        if (zigbeeRecvData(&data, 1)) {
            ret = (ZIGBEE_ERR_E)data;
        }
    }

    sZigbeeExitConfigMode();
    return ret;
}

// 搜索
ZIGBEE_ERR_E eZigbeeSearch
(
    USHORT *pType, UCHAR *pChan, UCHAR *pSpeed,
    USHORT *pPanId, USHORT *pAddr, UCHAR *pState
    )
{
    ZIGBEE_ERR_E ret = ZIGBEE_ERR_OTHER;

    UCHAR buffer[ZIGBEE_HEAD_LEN + 1] =
    {
        ZIGBEE_PRECODE,
        ZIGBEE_CMD_SEARCH
    };
    sZigbeeEnterConfigMode();

    ZIGBEE_BUFFER_CHECK_SUM(buffer, sizeof(buffer));
    ZIGBEE_SEND_BUFFER(buffer, sizeof(buffer));
    if (zigbeeRecvHead(ZIGBEE_CMD_SEARCH)) {
        //UCHAR data;
        if (zigbeeRecvData(pType, 2)
            && zigbeeRecvData(pChan, 1)
            && zigbeeRecvData(pSpeed, 1)
            && zigbeeRecvData(pPanId, 2)
            && zigbeeRecvData(pAddr, 2)
            && zigbeeRecvData(pState, 1)) {
            ret = ZIGBEE_ERR_OK;
        }
    }

    sZigbeeExitConfigMode();
    return ret;
}

// 获取远程配置信息
ZIGBEE_ERR_E eZigbeeGetOtherDevInfo(USHORT Addr, ZIGBEE_DEV_INFO_T *pInfo, UCHAR *pState, USHORT *pType, USHORT *pVer)
{
    ZIGBEE_ERR_E ret = ZIGBEE_ERR_OTHER;

    UCHAR buffer[ZIGBEE_HEAD_LEN + 2 + 1] =
    {
        ZIGBEE_PRECODE,
        ZIGBEE_CMD_GET_OTHER_DEV_INFO,
        (Addr >> 8) & 0xFF,
        Addr & 0xFF
    };
    sZigbeeEnterConfigMode();

    ZIGBEE_BUFFER_CHECK_SUM(buffer, sizeof(buffer));
    ZIGBEE_SEND_BUFFER(buffer, sizeof(buffer));
    if (zigbeeRecvHead(ZIGBEE_CMD_GET_OTHER_DEV_INFO)) {
        if (zigbeeRecvData(pInfo, sizeof(ZIGBEE_DEV_INFO_T))
            && zigbeeRecvData(pState, 1)
            && zigbeeRecvData(pType, 2)
            && zigbeeRecvData(pVer, 2)) {
            ret = ZIGBEE_ERR_OK;
        }
    }

    sZigbeeExitConfigMode();
    return ret;
}

// 修改配置命令
ZIGBEE_ERR_E eZigbeeSetDevInfo(USHORT Addr, ZIGBEE_DEV_INFO_T xInfo)
{
    ZIGBEE_ERR_E ret = ZIGBEE_ERR_OTHER;

    UCHAR buffer[ZIGBEE_HEAD_LEN + 2 + sizeof(ZIGBEE_DEV_INFO_T) + 1] =
    {
        ZIGBEE_PRECODE,
        ZIGBEE_CMD_SET_DEV_INFO,
        (Addr >> 8) & 0xFF,
        Addr & 0xFF
    };

    sZigbeeEnterConfigMode();

    xInfo.PanID = zigbee_htons(xInfo.PanID);
    xInfo.Addr = zigbee_htons(xInfo.Addr);
    xInfo.DstAddr = zigbee_htons(xInfo.DstAddr);

    if (xInfo.WorkMode == ZIGBEE_WORK_COORDINATOR) xInfo.WorkMode = ZIGBEE_WORK_END_DEVICE;
    memcpy(buffer + ZIGBEE_HEAD_LEN + 2, &xInfo, sizeof(ZIGBEE_DEV_INFO_T));
    ZIGBEE_BUFFER_CHECK_SUM(buffer, sizeof(buffer));
    ZIGBEE_SEND_BUFFER(buffer, sizeof(buffer));
    if (zigbeeRecvHead(ZIGBEE_CMD_SET_DEV_INFO)) {
        USHORT Reserve;
        UCHAR data;
        if (zigbeeRecvData(&Reserve, 2)
            && zigbeeRecvData(&data, 1)) {
            ret = (ZIGBEE_ERR_E)data;
        }
    }

    sZigbeeExitConfigMode();
    return ret;
}

// 复位
void vZigbeeReset(USHORT Addr, ZIGBEE_DEV_E devType)
{
    UCHAR buffer[ZIGBEE_HEAD_LEN + 2 + 2 + 1] =
    {
        ZIGBEE_PRECODE,
        ZIGBEE_CMD_RST,
        (Addr >> 8) & 0xFF, Addr & 0xFF,
        (devType >> 8) & 0xFF, devType & 0xFF
    };
    sZigbeeEnterConfigMode();

    ZIGBEE_BUFFER_CHECK_SUM(buffer, sizeof(buffer));
    ZIGBEE_SEND_BUFFER(buffer, sizeof(buffer));

    sZigbeeExitConfigMode();
}

// 恢复出厂设置
ZIGBEE_ERR_E eZigbeeSetDefault(USHORT Addr, ZIGBEE_DEV_E devType)
{
    ZIGBEE_ERR_E ret = ZIGBEE_ERR_OTHER;

    UCHAR buffer[ZIGBEE_HEAD_LEN + 2 + 2 + 1] =
    {
        ZIGBEE_PRECODE,
        ZIGBEE_CMD_DEFAULT,
        (Addr >> 8) & 0xFF, Addr & 0xFF,
        (devType >> 8) & 0xFF, devType & 0xFF
    };
    sZigbeeEnterConfigMode();

    ZIGBEE_BUFFER_CHECK_SUM(buffer, sizeof(buffer));
    ZIGBEE_SEND_BUFFER(buffer, sizeof(buffer));
    if (zigbeeRecvHead(ZIGBEE_CMD_DEFAULT)) {
        USHORT Reserve, type;
        UCHAR data;
        if (zigbeeRecvData(&Reserve, 2)
            && zigbeeRecvData(&type, 2)
            && zigbeeRecvData(&data, 1)) {
            ret = (ZIGBEE_ERR_E)data;
        }
    }

    sZigbeeExitConfigMode();
    return ret;
}

// 获取终端休眠时间
ZIGBEE_ERR_E eZigbeeGetPwdEnable(BOOL *pEn)
{
    ZIGBEE_ERR_E ret = ZIGBEE_ERR_OTHER;

    UCHAR buffer[ZIGBEE_HEAD_LEN + 1 + 1 + 1] =
    {
        ZIGBEE_PRECODE,
        ZIGBEE_CMD_PWD_EN,
        0x00,
        0x01
    };
    sZigbeeEnterConfigMode();

    ZIGBEE_BUFFER_CHECK_SUM(buffer, sizeof(buffer));
    ZIGBEE_SEND_BUFFER(buffer, sizeof(buffer));
    if (zigbeeRecvHead(ZIGBEE_CMD_PWD_EN)) {
        UCHAR data;
        if (zigbeeRecvData(pEn, 1)
            && zigbeeRecvData(&data, 1)) {
            ret = (ZIGBEE_ERR_E)data;
        }
    }

    sZigbeeExitConfigMode();
    return ret;
}

ZIGBEE_ERR_E eZigbeeSetPwdEnable(BOOL en)
{
    ZIGBEE_ERR_E ret = ZIGBEE_ERR_OTHER;

    UCHAR buffer[ZIGBEE_HEAD_LEN + 1 + 1 + 1] =
    {
        ZIGBEE_PRECODE,
        ZIGBEE_CMD_PWD_EN,
        0x01,
        (en ? 0x01 : 0x00)
    };
    sZigbeeEnterConfigMode();

    ZIGBEE_BUFFER_CHECK_SUM(buffer, sizeof(buffer));
    ZIGBEE_SEND_BUFFER(buffer, sizeof(buffer));
    if (zigbeeRecvHead(ZIGBEE_CMD_PWD_EN)) {
        UCHAR data;
        if (zigbeeRecvData(&en, 1)
            && zigbeeRecvData(&data, 1)) {
            ret = (ZIGBEE_ERR_E)data;
        }
    }

    sZigbeeExitConfigMode();
    return ret;
}

ZIGBEE_ERR_E eZigbeeLogin(UCHAR pwd[], UCHAR len)
{
    ZIGBEE_ERR_E ret = ZIGBEE_ERR_OTHER;

    UCHAR buffer[ZIGBEE_HEAD_LEN + 16 + 1] =
    {
        ZIGBEE_PRECODE,
        ZIGBEE_CMD_LOGIN
    };
    sZigbeeEnterConfigMode();

    memcpy(buffer + ZIGBEE_HEAD_LEN, pwd, len);
    ZIGBEE_BUFFER_CHECK_SUM(buffer, sizeof(buffer));
    ZIGBEE_SEND_BUFFER(buffer, sizeof(buffer));
    if (zigbeeRecvHead(ZIGBEE_CMD_LOGIN)) {
        UCHAR data;
        if (zigbeeRecvData(&data, 1)) {
            ret = (ZIGBEE_ERR_E)data;
        }
    }

    sZigbeeExitConfigMode();
    return ret;
}

const zgb_std_head_t scan_req_head = {
    .pre        = ZGB_STD_PRE,
    .npre       = ZGB_STD_NPRE,
    .ver        = 0,
    .msglen     = sizeof(zgb_std_head_t) + sizeof(zgb_std_scan_req_t) + 4,
    .seq        = 0,
    .packtype   = ZGB_STD_FA_REQ,
    .msgtype    = ZGB_STD_MSG_SCAN,
};

static rt_uint8_t s_zgb_rssi = 0;

ZIGBEE_ERR_E eZigbeeSendStdLstRsp(zgb_std_scan_req_t *req)
{
    rt_uint16_t addr = req->netid;
    ezgb_sn_type_t type = req->type;
    ZIGBEE_ERR_E ret = ZIGBEE_ERR_OK;
    sZigbeeEnterConfigMode();
    {
        //rt_int8_t rssi = 0;
        if (addr != g_zigbee_cfg.xInfo.DstAddr) {
            ret = ucZigbeeSetDstAddr(addr);
        }
        if (ZIGBEE_ERR_OK == ret) {
            /*ret = */ucZigbeeRSSI(addr, (UCHAR *)&s_zgb_rssi);
        }
        if (ZIGBEE_ERR_OK == ret) {
            // add by jay 2016/11/22 : 匹配终端协议
            if (g_zigbee_cfg.nProtoType == type) {
                rt_uint32_t crc = 0;
                rt_uint8_t cnt = 0;
                rt_uint8_t *addr = RT_NULL;
                if (g_zigbee_cfg.btSlaveAddr > 0) {
                    cnt++;
                }
                for (int i = 0; i < BOARD_UART_MAX; i++) {
                    if(g_xfer_uart_cfgs[i].enable && g_xfer_net_dst_uart_dtu[i] && g_xfer_uart_cfgs[i].count > 0) {
                        for(int j = 0; j < g_xfer_uart_cfgs[i].count; j++) {
                            cnt++;
                        }
                    }
                }
                if(cnt > 0) {
                    addr = rt_calloc(1, cnt);
                    cnt = 0;
                    if(addr) {
                        if (g_zigbee_cfg.btSlaveAddr > 0) {
                            addr[cnt++] = g_zigbee_cfg.btSlaveAddr;
                        }
                        for (int i = 0; i < BOARD_UART_MAX; i++) {
                            if(g_xfer_uart_cfgs[i].enable && g_xfer_net_dst_uart_dtu[i] && g_xfer_uart_cfgs[i].count > 0) {
                                for(int j = 0; j < g_xfer_uart_cfgs[i].count; j++) {
                                    addr[cnt++] = g_xfer_uart_cfgs[i].addrs[j];
                                }
                            }
                        }
                    }
                }

                zgb_std_head_t head = {
                    .pre        = ZGB_STD_PRE,
                    .npre       = ZGB_STD_NPRE,
                    .ver        = 0,
                    .msglen     = sizeof(zgb_std_head_t) + ZGB_OFS(zgb_std_scan_t, data) + cnt + 4,
                    .seq        = 0,
                    .packtype   = ZGB_STD_FA_RSP,
                    .msgtype    = ZGB_STD_MSG_SCAN,
                };
                zgb_std_scan_t info = {
                    .workmode   = g_zigbee_cfg.xInfo.WorkMode,
                    .netid      = g_zigbee_cfg.xInfo.Addr,
                    .rssi       = s_zgb_rssi,
                    .type       = ZGB_SN_T_MODBUS_RTU,
                    .cnt        = cnt
                };

                memcpy(&info.mac, g_zigbee_cfg.xInfo.Mac, 8);
                ZIGBEE_SEND_BUFFER(&head, sizeof(head));
                crc = ulRTCrc32(crc, (void *)&head, sizeof(head));
                ZIGBEE_SEND_BUFFER(&info, ZGB_OFS(zgb_std_scan_t, data));
                crc = ulRTCrc32(crc, &info, ZGB_OFS(zgb_std_scan_t, data));
                if (cnt > 0 && addr) {
                    ZIGBEE_SEND_BUFFER(addr, cnt);
                    crc = ulRTCrc32(crc, addr, cnt);
                }
                if(addr) rt_free(addr);
                ZIGBEE_SEND_BUFFER(&crc, sizeof(crc));

                if(req->learnstep == 0) {
                    g_zigbee_cfg.ulLearnStep = 30;
                } else {
                    g_zigbee_cfg.ulLearnStep = req->learnstep;
                }
                info.workmode = ZIGBEE_WORK_COORDINATOR;
                info.netid = req->netid;
                info.mac = req->mac;
                info.cnt = 0;
                zgb_mnode_insert(&info);
            }
        }
    }
    sZigbeeExitConfigMode();
    return ret;
}

static rt_bool_t __check_buffer(zgb_std_head_t *head, rt_uint8_t buffer[])
{
    rt_uint8_t *data = buffer;

    memcpy(head, data, sizeof(zgb_std_head_t)); data += sizeof(zgb_std_head_t);

    if (ZGB_STD_PRE == head->pre && ZGB_STD_NPRE == head->npre && head->ver == 0 && head->msglen >= 4) {
        rt_uint32_t crc = 0;
        memcpy(&crc, &buffer[head->msglen - 4], 4);
        return (crc == ulRTCrc32(0, buffer, head->msglen - 4));
    }

    return RT_FALSE;
}

// 该任务只负责解析
static rt_uint32_t s_zgb_learn_starttick;

void vZigbeeLearnNow(void)
{
    s_zgb_learn_starttick = rt_tick_get() - rt_tick_from_millisecond(g_zigbee_cfg.ulLearnStep * 1000);
}

static void _zgbparse_thread(void *parameter)
{
    zgb_std_head_t head;
    static zgb_buf_queue_t _stdqueue;
    s_zgb_learn_starttick = rt_tick_get() - rt_tick_from_millisecond(g_zigbee_cfg.ulLearnStep * 1000);

    for (;;) {
        rt_uint8_t *buffer = _stdqueue.buf;

        rt_thddog_feed("rt_mq_recv 1 sec");
        if (RT_EOK == rt_mq_recv(s_zgb_buf_queue, &_stdqueue, sizeof(zgb_buf_queue_t), RT_TICK_PER_SECOND)) {
            if (_stdqueue.len >= sizeof(zgb_std_head_t) + 4) {
                memcpy(&head, buffer, sizeof(zgb_std_head_t));
                rt_thddog_feed("__check_buffer");
                if (__check_buffer(&head, buffer)) {
                    memset(&buffer[head.msglen - 4], 0, sizeof(_stdqueue.buf) - (head.msglen - 4));
                    buffer += sizeof(zgb_std_head_t);
                    if (ZGB_STD_FA_POST == head.packtype || ZGB_STD_FA_RSP == head.packtype) {
                        rt_thddog_feed("zgb_std_parse_buffer");
                        zgb_std_parse_buffer(&head, buffer);
                    } else if (ZGB_STD_FA_REQ == head.packtype) {
                        if (ZGB_STD_MSG_SCAN == head.msgtype) {
                            if (ZIGBEE_WORK_END_DEVICE == g_zigbee_cfg.xInfo.WorkMode ||
                                ZIGBEE_WORK_ROUTER == g_zigbee_cfg.xInfo.WorkMode
                               ) {
                                zgb_std_scan_req_t req;
                                
                                memcpy(&req, buffer, sizeof(zgb_std_scan_req_t));

                                // 匹配地址, 则回复设备列表
                                if (g_zigbee_cfg.xInfo.DstAddr == req.netid) {
                                    // 257 素数求余, 延时发送
                                    rt_thread_delay(rt_tick_from_millisecond(req.netid % 257));
                                    rt_thddog_feed("eZigbeeSendStdLstRsp");
                                    eZigbeeSendStdLstRsp(&req);
                                }
                            }
                        }
                    }
                }
            }
        }

        rt_uint32_t nowtick = rt_tick_get();

        if (ZIGBEE_WORK_COORDINATOR == g_zigbee_cfg.xInfo.WorkMode) {
            // 30 秒进行一次学习
            if (nowtick - s_zgb_learn_starttick > rt_tick_from_millisecond(g_zigbee_cfg.ulLearnStep * 1000)) {
                rt_thddog_feed("begin snd learn buffer");
                sZigbeeEnterConfigMode();
                {
                    rt_uint32_t crc = 0;
                    zgb_std_scan_req_t req = {
                        .netid      = g_zigbee_cfg.xInfo.Addr,
                        .type       = ZGB_SN_T_MODBUS_RTU, 
                        .learnstep  = g_zigbee_cfg.ulLearnStep
                    };
                    // 转发时, 扫描指定转发协议终端/路由  add by jay 2016/11/22
                    /*if (g_xfer_net_dst_uart_occ[BOARD_ZGB_UART]) {
                        switch (g_tcpip_cfgs[g_xfer_net_dst_uart_map[BOARD_ZGB_UART]].cfg.xfer->cfg.gw.proto_type) {
                        case XFER_PROTO_MODBUS_RTU:
                            req.type = ZGB_SN_T_MODBUS_RTU;
                            break;
                        case XFER_PROTO_MODBUS_ASCII:
                            req.type = ZGB_SN_T_MODBUS_ASCII;
                            break;
                        }
                    }*/
                    memcpy(&req.mac, g_zigbee_cfg.xInfo.Mac, 8);
                    ucZigbeeMode(ZIGBEE_MSG_MODE_BROAD);
                    ZIGBEE_SEND_BUFFER(&scan_req_head, sizeof(scan_req_head));
                    crc = ulRTCrc32(crc, (void *)&scan_req_head, sizeof(scan_req_head));
                    ZIGBEE_SEND_BUFFER(&req, sizeof(req));
                    crc = ulRTCrc32(crc, &req, sizeof(req));
                    ZIGBEE_SEND_BUFFER(&crc, sizeof(crc));
                    rt_thread_delay(100 * RT_TICK_PER_SECOND / 1000);
                    ucZigbeeMode(ZIGBEE_MSG_MODE_SINGLE);
                }
                sZigbeeExitConfigMode();
                rt_thddog_feed("end snd learn buffer");
                s_zgb_learn_starttick = nowtick;
            }
        }
        rt_thddog_feed("zgb_check_online");
        zgb_check_online();
    }

    rt_thddog_exit();
}

void vZigbee_SendBuffer(rt_uint8_t *buffer, int n)
{
    if ( s_zgb_serial && buffer && n > 0) {
        ZIGBEE_SEND_BUFFER(buffer, n);
    }
}

// for webserver
DEF_CGI_HANDLER(doZigbeeLearnNow)
{
    rt_kprintf("vZigbeeLearnNow\n");

    WEBS_PRINTF("{\"ret\":%d}", RT_EOK);
    WEBS_DONE(200);

    vZigbeeLearnNow();
}

static ssize_t __mb_zgb_send(modbus_t *ctx, const uint8_t *req, int req_length)
{
    modbus_user_data_t *user_data = (modbus_user_data_t *)modbus_get_user_data(ctx);
    if (req && req_length > 0) {
        return serial_helper_send(BOARD_ZGB_UART, (const void *)req, (rt_size_t)req_length);
    }
    return -1;
}

static ssize_t __mb_zgb_recv(modbus_t *ctx, uint8_t *rsp, int rsp_length)
{
    return bfifo_pull(s_zgb_fifo, (unsigned char *)rsp, rsp_length, 0);
}

static int __mb_zgb_connect(modbus_t *ctx)
{
    modbus_user_data_t *user_data = (modbus_user_data_t *)modbus_get_user_data(ctx);
    modbus_set_socket(ctx, 0);
    bfifo_reset(s_zgb_fifo);
    return (s_zgb_fifo ? 0 : -1);
}

static void __mb_zgb_close(modbus_t *ctx)
{
    modbus_set_socket(ctx, -1);
}

static int __mb_zgb_select(modbus_t *ctx, fd_set *rset, struct timeval *tv, int msg_length)
{
    int rc = -1;
    if (s_zgb_fifo) {
        rc = bfifo_pull_wait(s_zgb_fifo, tv ? tv->tv_usec + tv->tv_sec * 1000000 : -1);
        if (rc == 0) {
            errno = ETIMEDOUT;
            rc = -1;
        }
    } else {
        errno = EBADF;
    }
    return rc;
}

modbus_customize_backend_t g_zgb_customize_backend = {
    .send       = __mb_zgb_send, 
    .recv       = __mb_zgb_recv, 
    .connect    = __mb_zgb_connect, 
    .close      = __mb_zgb_close, 
    .select     = __mb_zgb_select, 
};

