
#include <board.h>
#include "modbus.h"
#include "modbus_helper.h"

#define LORA_AT_USE_GLOBAL_BUFFER       (1)

#define DM_LORA_CTL         _IO('L',100)
#define LORA_AT_PI14        0
#define LORA_WAKEUP_PI15    1
#define LORA_RESET_PB5      2
#define LORA_REVER_PB4      3 //预留

typedef struct {
    int len;
    int pos;
    uint8_t buf[LORA_STD_FA_SIZE];
} lora_buf_queue_t;

static lora_buf_queue_t s_lora_rcv_queue;
static rt_thread_t s_lora_parse_thread = RT_NULL;
static rt_thread_t s_lora_recv_thread = RT_NULL;
static rt_thread_t s_lora_worker_thread = RT_NULL;
static rt_mq_t s_lora_cmd_queue = RT_NULL;
static rt_mq_t s_lora_std_queue = RT_NULL;
static volatile int s_lora_in_at_hold = 0;
static volatile int s_lora_in_at = 0;
static pthread_mutex_t s_lora_mutex;
static serial_t *s_lora_serial = NULL;
static bfifo_t s_lora_fifo = NULL;

char g_lora_ver[64+1];
char g_lora_ak[32+1];

extern rt_bool_t g_lora_init;

static int __lora_is_std_head(uint8_t *data, int len);
static int __lora_is_at_end(uint8_t *data, int len);

static uint16_t __lora_get_check(uint16_t check, uint8_t buffer[], int len);

#define big_little_swap16(A)  ((((uint16_t)(A) & 0xff00) >> 8) | (((uint16_t)(A) & 0x00ff) << 8))
#define big_little_swap32(A)  ((((uint32_t)(A) & 0xff000000) >> 24) | (((uint32_t)(A) & 0x00ff0000) >> 8) | (((uint32_t)(A) & 0x0000ff00) << 8) | (((uint32_t)(A) & 0x000000ff) << 24))

#define lora_htons(_n)      big_little_swap16(_n)
#define lora_ntohs(_n)      big_little_swap16(_n)

#define lora_htonl(_n)      big_little_swap32(_n)
#define lora_ntohl(_n)      big_little_swap32(_n)

#define LORA_CMD_WAIT       (RT_TICK_PER_SECOND)

static int __lora_send_buffer(const void *buffer, int n)
{
    return serial_helper_send(BOARD_LORA_UART, (const void *)buffer, n);
}

#if LORA_AT_USE_GLOBAL_BUFFER
static char _lora_at_buffer[256];
#endif

static int __lora_printf_at(const char *fmt, ...)
{
#if !LORA_AT_USE_GLOBAL_BUFFER
    char _lora_at_buffer[256];
#endif
    int len = 0;
    va_list args;
    va_start(args, fmt);
    rt_memset(_lora_at_buffer, 0, sizeof(_lora_at_buffer));
    len = rt_vsprintf(_lora_at_buffer, fmt, args);
    va_end(args);
    rt_kprintf("lora->at : %s", _lora_at_buffer);
    return serial_helper_send(BOARD_LORA_UART, (const void *)_lora_at_buffer, len);
}

static int __lora_is_at_end(uint8_t *data, int len)
{
    return (data[len - 2] == '\r' && data[len - 1] == '\n');
}

static int __lora_is_std_head(uint8_t *data, int len)
{
    return (len >= sizeof(lora_std_head_t) &&
            data[0] == LORA_STD_PRE0 &&
            data[1] == LORA_STD_PRE1);
}

void lora_hw_reset(void)
{
    s_io_t iodata = {.gpio = LORA_RESET_PB5, .dir = 1, .val = 0};
    das_do_io_ctrl(DM_LORA_CTL, &iodata);
    rt_thread_delay(1);
    iodata.val = 1;
    das_do_io_ctrl(DM_LORA_CTL, &iodata);
    rt_thread_delay(500);
}

void lora_enter_at_mode(void)
{
    rt_mutex_take(&s_lora_mutex, RT_WAITING_FOREVER);
    if (s_lora_in_at_hold == 0) {
        s_io_t iodata = {.gpio = LORA_AT_PI14, .dir = 1, .val = 1};
        das_do_io_ctrl(DM_LORA_CTL, &iodata);
        rt_thread_delay(100);
    }
    s_lora_in_at_hold++;
    s_lora_in_at = 1;
    b_mq_reset(s_lora_cmd_queue);
}

void lora_exit_at_mode(void)
{
    s_lora_in_at_hold--;
    if (0 == s_lora_in_at_hold) {
        s_io_t iodata = {.gpio = LORA_AT_PI14, .dir = 1, .val = 0};
        das_do_io_ctrl(DM_LORA_CTL, &iodata);
        rt_thread_delay(100);
        s_lora_in_at = 0;
    }
    rt_mutex_release(&s_lora_mutex);
}

void lora_enter_data_mode(void)
{
    rt_mutex_take(&s_lora_mutex, RT_WAITING_FOREVER);
}

void lora_exit_data_mode(void)
{
    rt_mutex_release(&s_lora_mutex);
}

int lora_in_at_mode(void)
{
    return s_lora_in_at;
}

static void __lora_parse_thread(void *parameter);
static void __lora_recv_thread(void *parameter);
static void __lora_worker_thread(void *parameter);

static int _lora_in_parse = 0;

static void __lora_recv_buffer(int port, void *buffer, int size)
{
    _lora_in_parse = 1;
    s_lora_rcv_queue.pos = 0;
    memcpy(s_lora_rcv_queue.buf, buffer, size);
    s_lora_rcv_queue.len = size;

    if (lora_in_at_mode()) {
        s_lora_rcv_queue.buf[s_lora_rcv_queue.len] = '\0';
        rt_kprintf("%s\n", s_lora_rcv_queue.buf);
        if (__lora_is_at_end(s_lora_rcv_queue.buf, s_lora_rcv_queue.len) && s_lora_rcv_queue.len > 4) {
            //xfer_dst_serial_rcv_clear(BOARD_LORA_UART);
            rt_mq_send(s_lora_cmd_queue, &s_lora_rcv_queue, sizeof(lora_buf_queue_t));
        }
    } else {
        if (g_lora_cfg.work_mode == LORA_WORK_CENTRAL) {
            lora_cmd_head_t cmd_head;
            if (s_lora_rcv_queue.len >= sizeof(lora_cmd_head_t)) {
                memcpy(&cmd_head, &s_lora_rcv_queue.buf[0], sizeof(lora_cmd_head_t));
                if (cmd_head.cmd == LORA_CMD_RECV) {
                    //rt_kprintf("recv packet[ALL], addr = %08X, len = %d\n", cmd_head.addr, s_lora_rcv_queue.len - sizeof(lora_cmd_head_t));
                    if (__lora_is_std_head(&s_lora_rcv_queue.buf[sizeof(lora_cmd_head_t)], s_lora_rcv_queue.len - sizeof(lora_cmd_head_t))) {
                        //xfer_dst_serial_rcv_clear(BOARD_LORA_UART);
                        rt_mq_send(s_lora_std_queue, &s_lora_rcv_queue, sizeof(lora_buf_queue_t));
                    }
                } else {
                    rt_kprintf("-- error packet (unknown cmd)!\n");
                }
            } else {
                rt_kprintf("-- error packet (miss data, datalen : %d)!\n", s_lora_rcv_queue.len);
            }
        }
    }

_END:

    s_lora_rcv_queue.pos = 0;
    s_lora_rcv_queue.len = 0;
    _lora_in_parse = 0;
}

static void __lora_reopen(int port)
{
    serial_helper_cfg(port, &g_uart_cfgs[port].port_cfg);
    serial_helper_open(port);
}

static uint32_t __lora_get_timeout(void)
{
    if (lora_in_at_mode()) {
        return 100 * 1000;
    } else {
        s_lora_serial = g_serials[BOARD_LORA_UART];
        if (s_lora_serial->cfg.baud_rate > 19200) {
            return 35 * 500;
        } else {
            return 500 * (7UL * 220000UL) / (2UL * s_lora_serial->cfg.baud_rate);
        }
    }
}

static void __lora_recv_thread(void *parameter)
{
    while (1) {
        int port = BOARD_LORA_UART;
        uint8_t buffer[4096] = {0};
        while (serial_helper_is_open(port)) {
            int pos = 0;
            int s_rc = serial_helper_select(port, -1);
            int n = serial_helper_recv(port, buffer, sizeof(buffer));
            if (n > 0) {
                pos = n;
                while (1) {
                    s_rc = serial_helper_select(port, __lora_get_timeout());
                    if (s_rc == -1) {
                        usleep(100 * 1000);
                        break;
                    } else if (s_rc == 0) {
                        __lora_recv_buffer(port, buffer, pos);
                        break;
                    } else {
                        n = serial_helper_recv(port, &buffer[pos], sizeof(buffer) - pos);
                        pos += n;
                        if (pos >= sizeof(buffer)) {
                            __lora_recv_buffer(port, buffer, pos);
                            break;
                        }
                    }
                }
            } else if (n <= 0) {
                if (n < 0 && errno == EBADF) __lora_reopen(port);
                rt_thread_delay(500);
            }
        }
        rt_thread_delay(1000);
    }
}


int __lora_reinit(int uart_default)
{
    int bStatus = FALSE;
    s_io_t iodata = {.gpio = LORA_AT_PI14, .dir = 1, .val = 0};
    das_do_io_ctrl(DM_LORA_CTL, &iodata);
    iodata.gpio = LORA_WAKEUP_PI15;
    das_do_io_ctrl(DM_LORA_CTL, &iodata);

    s_lora_fifo = bfifo_create(4096);
    
    lora_hw_reset();

    lora_std_init();

    if (rt_mutex_init(&s_lora_mutex, "loramtx", RT_IPC_FLAG_PRIO) != RT_EOK) {
        rt_kprintf("init loramtx failed\n");
        return 0;
    }

    s_lora_serial = g_serials[BOARD_LORA_UART];

    if (uart_default) {
        s_lora_serial->cfg.baud_rate = 9600;
        s_lora_serial->cfg.data_bits = 8;
        s_lora_serial->cfg.stop_bits = 1;
        s_lora_serial->cfg.parity = 0;
    }
    serial_helper_open(BOARD_LORA_UART);

    if ((s_lora_cmd_queue = rt_mq_create("loracmdq", sizeof(lora_buf_queue_t), 1, RT_IPC_FLAG_PRIO)) != RT_NULL) {
        ;
    } else {
        rt_kprintf("lora rt_mq_create loracmdq falied..\n");
        return 0;
    }

    if ((s_lora_std_queue = rt_mq_create("lorabufq", sizeof(lora_buf_queue_t), 5, RT_IPC_FLAG_PRIO)) != RT_NULL) {
        ;
    } else {
        rt_kprintf("lora rt_mq_create lorabufq falied..\n");
        return 0;
    }

    s_lora_parse_thread = rt_thread_create("loraparse", __lora_parse_thread, RT_NULL, 0x400, 10, 10);
    if (s_lora_parse_thread != RT_NULL) {
        rt_thddog_register(s_lora_parse_thread, 30);
        rt_thread_startup(s_lora_parse_thread);
    }

    s_lora_recv_thread = rt_thread_create("lorarecv", __lora_recv_thread, RT_NULL, 0x400, 10, 10);
    if (s_lora_recv_thread != RT_NULL) {
        rt_thddog_register(s_lora_recv_thread, 30);
        rt_thread_startup(s_lora_recv_thread);
    }

}

int lora_init(int uart_default)
{
    rt_thread_delay(500);
    lora_enter_at_mode();

    do {
        if (lora_at_test() || lora_at_test()) {
            g_uart_cfgs[BOARD_LORA_UART].port_cfg = s_lora_serial->cfg;
            break;
        }
        {
            uart_port_cfg_t cfg = s_lora_serial->cfg;
            cfg.baud_rate = 115200;
            s_lora_serial->cfg.baud_rate = 115200;
            g_uart_cfgs[BOARD_LORA_UART].port_cfg = s_lora_serial->cfg;
            serial_helper_cfg(BOARD_LORA_UART, &cfg);
        }
    } while (0);

    for (int n = 0; n < 10; n++) {
        if (lora_at_test()) break;
    }
    //lora_at_req_all_cfg();
    //lora_at_def();
    if (lora_at_test()) {
        lora_at_req_all_cfg();
        lora_at_req_all();
        lora_at_set_all();
        lora_exit_at_mode();
        g_lora_init = 1;
        lora_learn_now();
        
        s_lora_worker_thread = rt_thread_create("loraworker", __lora_worker_thread, RT_NULL, 0x400, 10, 10);
        if (s_lora_worker_thread != RT_NULL) {
            rt_thddog_register(s_lora_worker_thread, 30);
            rt_thread_startup(s_lora_worker_thread);
        }
        return 1;
    } else {
        lora_exit_at_mode();
        return 0;
    }
}

static int __lora_match_std_head(lora_buf_queue_t *buf)
{
    if (__lora_is_std_head(&buf->buf[buf->pos], buf->len - buf->pos)) {
        buf->pos += sizeof(lora_std_head_t);
        return 1;
    }
    return 0;
}

static lora_buf_queue_t _cmdqueue;
static char *__lora_wait_at_result(int timeout)
{
    if (RT_EOK == rt_mq_recv(s_lora_cmd_queue, &_cmdqueue, sizeof(lora_buf_queue_t), timeout)) {
        _cmdqueue.buf[_cmdqueue.len] = '\0';
        return _cmdqueue.buf;
    }
    return NULL;
}

static int __lora_wait_at_rsp_ok(int timeout)
{
    char *rsp = __lora_wait_at_result(timeout);
    if (rsp) {
        return (strstr(rsp, "OK") != NULL);
    }
    return 0;
}

static int __lora_wait_at_rsp_str(int timeout, const char *prefix, char *str)
{
    char *rsp = __lora_wait_at_result(timeout);
    if (rsp) {
        char *p = strstr(rsp, prefix);
        if (p) {
            str[0] = '\0';
            return (1 == sscanf(p, "%*[^:]:%s\r\n", str));
        }
    }
    return 0;
}

static int __lora_wait_at_rsp_one_int(int timeout, const char *prefix, uint32_t *num)
{
    char *rsp = __lora_wait_at_result(timeout);
    if (rsp) {
        char *p = strstr(rsp, prefix);
        if (p) {
            *num = 0;
            return (1 == sscanf(p, "%*[^:]:%x\r\n", num));
        }
    }
    return 0;
}

static int __lora_wait_at_rsp_two_int(int timeout, const char *prefix, uint32_t *num0, uint32_t *num1)
{
    char *rsp = __lora_wait_at_result(timeout);
    if (rsp) {
        char *p = strstr(rsp, prefix);
        if (p) {
            *num0 = 0; *num1 = 0;
            return (2 == sscanf(p, "%*[^:]:%d,%d\r\n", num0, num1));
        }
    }
    return 0;
}

int lora_at_test(void)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT\r\n");
    result = __lora_wait_at_rsp_ok(200);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_def(void)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("ATZ\r\n");
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_sw_reset(void)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("ATR\r\n");
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

static void __lora_printf_info(lora_cfg_t *cfg)
{
    rt_kprintf("ver:%s \n", g_lora_ver);
    rt_kprintf("id:%02X%02X%02X%02X%02X%02X%02X%02X\n", 
                cfg->info.id[0], cfg->info.id[1], cfg->info.id[2], cfg->info.id[3], 
                cfg->info.id[4], cfg->info.id[5], cfg->info.id[6], cfg->info.id[7]);
    rt_kprintf("net_type:%02X, mode:%02X \n", cfg->info.net_type, cfg->info.mode);
    rt_kprintf("snr:%d, rssi:%d \n", cfg->info.snr, cfg->info.rssi);
    rt_kprintf("ak:%s\n", g_lora_ak);
    rt_kprintf("addr:%08X, maddr:%08X \n", cfg->info.addr, cfg->info.maddr);
    rt_kprintf("sync:%02X, pow:%02X, bw:%02X, cr:%02X, crc:%02X \n", cfg->info.sync, cfg->info.pow, cfg->info.bw, cfg->info.cr, cfg->info.crc);
    rt_kprintf("tfreq:%d.%dM, rfreq:%d.%dM\n", cfg->info.tfreq / 1000000, (cfg->info.tfreq % 1000000) / 100000, cfg->info.rfreq / 1000000, (cfg->info.rfreq % 1000000) / 100000);
    rt_kprintf("tsf:%02X, rsf:%02X \n", cfg->info.tsf, cfg->info.rsf);
    rt_kprintf("sync:%02X, tprem:%04X, rprem:%04X, ldr:%04X \n", cfg->info.sync, cfg->info.tprem, cfg->info.rprem, cfg->info.ldr);
    rt_kprintf("tiq:%02X, riq:%02X, sip:%02X \n", cfg->info.tiq, cfg->info.riq, cfg->info.sip);
    rt_kprintf("data_type:%02X \n", cfg->info.data_type);
    rt_kprintf("lcp:%04X, lft:%02X, lat:%08X, lgt:%08X, el:%04X \n", cfg->info.lcp, cfg->info.lft, cfg->info.lat, cfg->info.lgt, cfg->info.el);
}

lora_cfg_t s_lora_cfg_old;
int lora_at_req_all(void)
{
    lora_enter_at_mode();
    
    s_lora_cfg_old = c_lora_default_cfg;
    
    lora_at_req_ver(g_lora_ver);
    lora_at_req_id(s_lora_cfg_old.info.id);
    lora_at_req_csq(&s_lora_cfg_old.info.snr, &s_lora_cfg_old.info.rssi);
    lora_at_req_ak(g_lora_ak);
    lora_at_req_addr(&s_lora_cfg_old.info.addr);
    lora_at_req_maddr(&s_lora_cfg_old.info.maddr);
    lora_at_req_sync(&s_lora_cfg_old.info.sync);
    lora_at_req_pow(&s_lora_cfg_old.info.pow);
    lora_at_req_bw(&s_lora_cfg_old.info.bw);
    lora_at_req_cr(&s_lora_cfg_old.info.cr);
    lora_at_req_crc(&s_lora_cfg_old.info.crc);
    lora_at_req_tfreq(&s_lora_cfg_old.info.tfreq);
    lora_at_req_rfreq(&s_lora_cfg_old.info.rfreq);
    lora_at_req_tsf(&s_lora_cfg_old.info.tsf);
    lora_at_req_rsf(&s_lora_cfg_old.info.rsf);
    lora_exit_at_mode();

    __lora_printf_info(&s_lora_cfg_old);

    if (g_lora_cfg.info.addr == 0) {
        g_lora_cfg.info.addr = s_lora_cfg_old.info.addr;
    }
    
    if (g_lora_cfg.info.maddr == 0) {
        g_lora_cfg.info.maddr = s_lora_cfg_old.info.maddr;
    }
    
    // read only
    memcpy(g_lora_cfg.info.id, s_lora_cfg_old.info.id, 8);
    g_lora_cfg.info.snr = s_lora_cfg_old.info.snr;
    g_lora_cfg.info.rssi = s_lora_cfg_old.info.rssi;
    
    __lora_printf_info(&g_lora_cfg);
}

int lora_at_set_all(void)
{
    lora_enter_at_mode();
    
    lora_at_set_net(g_lora_cfg.info.net_type);
    lora_at_set_type(g_lora_cfg.info.data_type);
    
    //if (strcmp(g_lora_ak, "00000000000000000000000000000000")) {
    lora_at_set_ak("00000000000000000000000000000000");
    //}
    if (s_lora_cfg_old.info.addr != g_lora_cfg.info.addr) {
        lora_at_set_addr(g_lora_cfg.info.addr);
    }
    if (s_lora_cfg_old.info.maddr != g_lora_cfg.info.maddr) {
        lora_at_set_maddr(g_lora_cfg.info.maddr);
    }
    if (s_lora_cfg_old.info.sync != g_lora_cfg.info.sync) {
        lora_at_set_sync(g_lora_cfg.info.sync);
    }
    if (s_lora_cfg_old.info.pow != g_lora_cfg.info.pow) {
        lora_at_set_pow(g_lora_cfg.info.pow);
    }
    if (s_lora_cfg_old.info.bw != g_lora_cfg.info.bw) {
        lora_at_set_bw(g_lora_cfg.info.bw);
    }
    if (s_lora_cfg_old.info.cr != g_lora_cfg.info.cr) {
        lora_at_set_cr(g_lora_cfg.info.cr);
    }
    if (s_lora_cfg_old.info.crc != g_lora_cfg.info.crc) {
        lora_at_set_crc(g_lora_cfg.info.crc);
    }
    if (s_lora_cfg_old.info.tfreq != g_lora_cfg.info.tfreq) {
        lora_at_set_tfreq(g_lora_cfg.info.tfreq);
    }
    if (s_lora_cfg_old.info.rfreq != g_lora_cfg.info.rfreq) {
        lora_at_set_rfreq(g_lora_cfg.info.rfreq);
    }
    if (s_lora_cfg_old.info.tsf != g_lora_cfg.info.tsf) {
        lora_at_set_tsf(g_lora_cfg.info.tsf);
    }
    if (s_lora_cfg_old.info.rsf != g_lora_cfg.info.rsf) {
        lora_at_set_rsf(g_lora_cfg.info.rsf);
    }

    lora_at_set_mode(g_lora_cfg.info.mode);
    lora_at_set_tprem(g_lora_cfg.info.tprem);
    lora_at_set_rprem(g_lora_cfg.info.rprem);
    lora_at_set_ldr(g_lora_cfg.info.ldr);
    lora_at_set_tiq(g_lora_cfg.info.tiq);
    lora_at_set_riq(g_lora_cfg.info.riq);
    lora_at_set_sip(g_lora_cfg.info.sip);
    lora_at_set_ack(g_lora_cfg.info.ack);
    lora_at_set_lcp(g_lora_cfg.info.lcp);
    lora_at_set_lft(g_lora_cfg.info.lft);
    /*lora_at_set_lat(g_lora_cfg.info.lat);
    lora_at_set_lgt(g_lora_cfg.info.lgt);
    lora_at_set_el(g_lora_cfg.info.el);*/
    
    lora_exit_at_mode();
}

int lora_at_req_all_cfg(void)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+CFG?\r\n");
    while (__lora_wait_at_result(500));
    lora_exit_at_mode();
    
    return result;
}

int lora_at_req_ver(char *ver)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("ATI\r\n");
    {
        char *rsp = __lora_wait_at_result(1000);
        if (rsp) {
            char *p = strstr(rsp, "+ATI");
            ver[0] = '\0';
            if (p) {
                result = (1 == sscanf(p, "%*[^:]:%[^\r\n]", ver));
            } else {
                result = (1 == sscanf(rsp, "%[^\r\n]", ver));
            }
        }
    }
    lora_exit_at_mode();
    
    return result;
}

int lora_at_req_id(uint8_t id[8])
{
    int result = 0;
    memset(id, 0, 8);
    lora_enter_at_mode();
    __lora_printf_at("AT+ID?\r\n");
    {
        char *rsp = __lora_wait_at_result(1000);
        if (rsp) {
            char *p = strstr(rsp, "+ID");
            if (p) {
                char tmp[64] = {0};
                if (1 == sscanf(p, "%*[^:]:%s\r\n", tmp)) {
                    if (strlen(tmp) >= 16) {
                        int n;
                        for (n = 0; n < 8; n++) {
                            char num[3] = {tmp[2 * n + 0], tmp[2 * n + 1], '\0'};
                            id[n] = (uint8_t)strtol(num, NULL, 16);
                        }
                        result = 1;
                    }
                }
            }
        }
    }
    lora_exit_at_mode();
    
    return result;
}

int lora_at_req_csq(int *snr, int *rssi)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+CSQ?\r\n");
    result = __lora_wait_at_rsp_two_int(1000, "+CSQ", snr, rssi);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_req_ak(char *ak)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+AK?\r\n");
    result = __lora_wait_at_rsp_str(1000, "+AK", ak);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_req_addr(uint32_t *addr)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+ADDR?\r\n");
    result = __lora_wait_at_rsp_one_int(1000, "+ADDR", addr);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_req_maddr(uint32_t *maddr)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+MADDR?\r\n");
    result = __lora_wait_at_rsp_one_int(1000, "+MADDR", maddr);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_req_sync(uint32_t *sync)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+SYNC?\r\n");
    result = __lora_wait_at_rsp_one_int(1000, "+SYNC", sync);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_req_pow(uint32_t *pow)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+POW?\r\n");
    result = __lora_wait_at_rsp_one_int(1000, "+POW", pow);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_req_bw(uint32_t *bw)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+BW?\r\n");
    result = __lora_wait_at_rsp_one_int(1000, "+BW", bw);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_req_cr(uint32_t *cr)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+CR?\r\n");
    result = __lora_wait_at_rsp_one_int(1000, "+CR", cr);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_req_crc(uint32_t *crc)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+CRC?\r\n");
    result = __lora_wait_at_rsp_one_int(1000, "+CRC", crc);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_req_tfreq(uint32_t *tfreq)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+TFREQ?\r\n");
    result = __lora_wait_at_rsp_one_int(1000, "+TFREQ", tfreq);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_req_rfreq(uint32_t *rfreq)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+RFREQ?\r\n");
    result = __lora_wait_at_rsp_one_int(1000, "+RFREQ", rfreq);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_req_tsf(uint32_t *tsf)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+TSF?\r\n");
    result = __lora_wait_at_rsp_one_int(1000, "+TSF", tsf);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_req_rsf(uint32_t *rsf)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+RSF?\r\n");
    result = __lora_wait_at_rsp_one_int(1000, "+RSF", rsf);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_net(LORA_FREQ_MODE_E net)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+NET=%02X\r\n", net);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_ak(const char *ak)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+AK=%s\r\n", ak);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_addr(uint32_t addr)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+ADDR=%08X\r\n", addr);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_maddr(uint32_t maddr)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+MADDR=%08X\r\n", maddr);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_mode(LORA_WORK_MODE_E mode)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+MODE=%02X\r\n", mode);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_tprem(uint32_t tprem)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+TPREM=%04X\r\n", tprem);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_rprem(uint32_t rprem)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+RPREM=%04X\r\n", rprem);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_ldr(uint32_t ldr)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+LDR=%02X\r\n", ldr);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_sync(uint32_t sync)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+SYNC=%02X\r\n", sync);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_pow(uint32_t pow)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+POW=%02X\r\n", pow);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_bw(LORA_BW_E bw)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+BW=%02X\r\n", bw);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_cr(LORA_CR_E cr)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+CR=%02X\r\n", cr);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_crc(uint32_t crc)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+CRC=%02X\r\n", crc);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_tfreq(uint32_t tfreq)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+TFREQ=%08X\r\n", tfreq);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_rfreq(uint32_t rfreq)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+RFREQ=%08X\r\n", rfreq);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_tsf(LORA_FS_E tsf)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+TSF=%02X\r\n", tsf);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_rsf(LORA_FS_E rsf)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+RSF=%02X\r\n", rsf);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_tiq(uint32_t tiq)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+TIQ=%02X\r\n", tiq);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_riq(uint32_t riq)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+RIQ=%02X\r\n", riq);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_sip(LORA_SIP_E sip)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+SIP=%02X\r\n", sip);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_ack(uint32_t ack)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+ACK=%02X\r\n", ack);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_brate(LORA_SERIAL_RATE_E brate)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+BRATE=%02X\r\n", brate);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_par(LORA_SERIAL_PAR_E par)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+PAR=%02X\r\n", par);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_type(LORA_DATA_TYPE_E type)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+TYPE=%02X\r\n", type);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_lcp(uint32_t lcp)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+LCP=%04X\r\n", lcp);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_lft(uint32_t lft)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+LFT=%04X\r\n", lft);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_lat(uint32_t lat)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+LAT=%08X\r\n", lat);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_lgt(uint32_t lgt)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+LGT=%08X\r\n", lgt);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

int lora_at_set_el(uint32_t el)
{
    int result = 0;
    
    lora_enter_at_mode();
    __lora_printf_at("AT+EL=%04X\r\n", el);
    result = __lora_wait_at_rsp_ok(1000);
    lora_exit_at_mode();
    
    return result;
}

const lora_std_head_t c_lora_hrt_head = {
    .pre        = { LORA_STD_PRE0, LORA_STD_PRE1 }, 
    .packtype   = LORA_STD_POST, 
    .p2p        = 1, 
    .msgtype    = LORA_STD_MSG_PROT_HRT, 
    .rssi       = 0, 
    .msglen     = sizeof(lora_std_head_t) + 2, 
};

const lora_std_head_t c_lora_info_req_head = {
    .pre        = { LORA_STD_PRE0, LORA_STD_PRE1 }, 
    .packtype   = LORA_STD_REQ, 
    .p2p        = 0, 
    .msgtype    = LORA_STD_MSG_PROT_INFO, 
    .rssi       = 0, 
    .msglen     = sizeof(lora_std_head_t) + sizeof(lora_std_info_req_t) + 2, 
};

const lora_std_head_t c_lora_sync_head = {
    .pre        = { LORA_STD_PRE0, LORA_STD_PRE1 }, 
    .packtype   = LORA_STD_POST, 
    .p2p        = 0, 
    .msgtype    = LORA_STD_MSG_PROT_SYNC, 
    .rssi       = 0, 
    .msglen     = sizeof(lora_std_head_t) + sizeof(lora_std_sync_t) + 2, 
};

const lora_std_head_t c_lora_join_head = {
    .pre        = { LORA_STD_PRE0, LORA_STD_PRE1 }, 
    .packtype   = LORA_STD_POST, 
    .p2p        = 1, 
    .msgtype    = LORA_STD_MSG_PROT_JOIN, 
    .rssi       = 0, 
    .msglen     = sizeof(lora_std_head_t) + 2, 
};

const lora_std_head_t c_lora_rssi_req_head = {
    .pre        = { LORA_STD_PRE0, LORA_STD_PRE1 }, 
    .packtype   = LORA_STD_REQ, 
    .p2p        = 0, 
    .msgtype    = LORA_STD_MSG_RSSI, 
    .rssi       = 0, 
    .msglen     = sizeof(lora_std_head_t) + sizeof(lora_std_rssi_req_t) + 2, 
};

static uint16_t __lora_get_check(uint16_t check, uint8_t buffer[], int len)
{
    for (int n = 0; n < len; n++) {
        check += buffer[n];
    }
    return check;
}

static int __check_buffer(lora_std_head_t *head, uint8_t buffer[])
{
    if (head->pre[0] == LORA_STD_PRE0 && 
        head->pre[1] == LORA_STD_PRE1 && 
        head->packtype < LORA_STD_MAX && 
        head->msgtype < LORA_STD_MSG_MAX && 
        head->msglen >= (sizeof(lora_std_head_t) + 2)) {
        uint16_t check = 0;
        memcpy(&check, &buffer[head->msglen - 2], 2);
        return (check == __lora_get_check(0, buffer, head->msglen - 2));
    } else {
        return 0;
    }
}


static uint64_t __lora_last_send_time = 0;
static int __lora_hold_time(void)
{
    switch (g_lora_cfg.info.tsf) {
    case LORA_FS_7: return 100;
    case LORA_FS_8: return 100;
    case LORA_FS_9: return 200;
    case LORA_FS_10: return 200;
    case LORA_FS_11: return 300;
    case LORA_FS_12: return 300;
    default: return 100;
    }
}

static void __lora_hold_do()
{
    int hole_time = __lora_hold_time();
    int diff = (int)(das_sys_msectime() - __lora_last_send_time);
    if (hole_time > diff) {
        //rt_kprintf("__lora_hold_do delay diff : %d,%d,%d ms\n", hole_time, diff, (hole_time - diff));
        das_delay((hole_time - diff) * 1000);
    }
}

static void __lora_hold_refresh()
{
    __lora_last_send_time = das_sys_msectime();
}

// 该任务只负责解析

static void __lora_send_sync_post(void)
{
    uint32_t cnt = lora_mnode_num();
    lora_enter_data_mode();
    {
        uint16_t check = 0;
        lora_std_sync_t sync = {
            .netid      = g_lora_cfg.info.addr,
            .type       = LORA_SN_T_MODBUS_RTU, 
            .timestamp  = time(0), 
            .cnt        = ((cnt > 255) ? 255 : cnt)
        };
        lora_cmd_head_t _cmd_head = {
            .cmd    = LORA_CMD_SEND, 
            .addr   = g_lora_cfg.info.maddr, 
            .op     = 0, 
            .seq    = 0, 
        };
        __lora_hold_do();
        __lora_send_buffer(&_cmd_head, sizeof(lora_cmd_head_t));
        __lora_send_buffer(&c_lora_sync_head, sizeof(lora_std_head_t));
        check = __lora_get_check(check, (uint8_t *)&c_lora_sync_head, sizeof(lora_std_head_t));
        __lora_send_buffer(&sync, sizeof(lora_std_sync_t));
        check = __lora_get_check(check, (uint8_t *)&sync, sizeof(lora_std_sync_t));
        __lora_send_buffer(&check, sizeof(check));
        __lora_hold_refresh();
    }
    lora_exit_data_mode();
}

static void __lora_send_hrt_post(uint32_t addr)
{
    lora_enter_data_mode();
    {
        uint16_t check = 0;
        lora_cmd_head_t _cmd_head = {
            .cmd    = LORA_CMD_SEND, 
            .addr   = addr, 
            .op     = 0, 
            .seq    = 0, 
        };
        __lora_hold_do();
        __lora_send_buffer(&_cmd_head, sizeof(lora_cmd_head_t));
        __lora_send_buffer(&c_lora_hrt_head, sizeof(lora_std_head_t));
        check = __lora_get_check(check, (uint8_t *)&c_lora_hrt_head, sizeof(lora_std_head_t));
        __lora_send_buffer(&check, sizeof(check));
        __lora_hold_refresh();
    }
    lora_exit_data_mode();
}

static void __lora_send_info_req(uint32_t addr)
{
    lora_enter_data_mode();
    {
        uint16_t check = 0;
        lora_std_info_req_t req = {
            .netid      = addr
        };
        lora_cmd_head_t _cmd_head = {
            .cmd    = LORA_CMD_SEND, 
            .addr   = g_lora_cfg.info.maddr, 
            .op     = 0, 
            .seq    = 0, 
        };
        __lora_hold_do();
        __lora_send_buffer(&_cmd_head, sizeof(lora_cmd_head_t));
        __lora_send_buffer(&c_lora_info_req_head, sizeof(lora_std_head_t));
        check = __lora_get_check(check, (uint8_t *)&c_lora_info_req_head, sizeof(lora_std_head_t));
        __lora_send_buffer(&req, sizeof(lora_std_info_req_t));
        check = __lora_get_check(check, (uint8_t *)&req, sizeof(lora_std_info_req_t));
        __lora_send_buffer(&check, sizeof(check));
        __lora_hold_refresh();
    }
    lora_exit_data_mode();
}

static void __lora_send_rssi_req(uint32_t addr)
{
    lora_enter_data_mode();
    {
        uint16_t check = 0;
        lora_std_rssi_req_t req = {
            .netid      = addr
        };
        lora_cmd_head_t _cmd_head = {
            .cmd    = LORA_CMD_SEND, 
            .addr   = g_lora_cfg.info.maddr, 
            .op     = 0, 
            .seq    = 0, 
        };
        __lora_hold_do();
        __lora_send_buffer(&_cmd_head, sizeof(lora_cmd_head_t));
        __lora_send_buffer(&c_lora_rssi_req_head, sizeof(lora_std_head_t));
        check = __lora_get_check(check, (uint8_t *)&c_lora_rssi_req_head, sizeof(lora_std_head_t));
        __lora_send_buffer(&req, sizeof(lora_std_rssi_req_t));
        check = __lora_get_check(check, (uint8_t *)&req, sizeof(lora_std_rssi_req_t));
        __lora_send_buffer(&check, sizeof(check));
        __lora_hold_refresh();
    }
    lora_exit_data_mode();
}

static void __lora_send_join_post(uint32_t addr)
{
    lora_enter_data_mode();
    {
        uint16_t check = 0;
        lora_cmd_head_t _cmd_head = {
            .cmd    = LORA_CMD_SEND, 
            .addr   = addr, 
            .op     = 0, 
            .seq    = 0, 
        };
        __lora_hold_do();
        __lora_send_buffer(&_cmd_head, sizeof(lora_cmd_head_t));
        __lora_send_buffer(&c_lora_join_head, sizeof(lora_std_head_t));
        check = __lora_get_check(check, (uint8_t *)&c_lora_join_head, sizeof(lora_std_head_t));
        __lora_send_buffer(&check, sizeof(check));
        __lora_hold_refresh();
    }
    lora_exit_data_mode();
}

static void __lora_std_parse_buffer(lora_cmd_head_t *cmd_head, lora_std_head_t *head, uint8_t buffer[])
{
    lora_mnode_t *retnode = RT_NULL;

    if (LORA_STD_ACK == head->packtype) {
        if (LORA_STD_MSG_PROT_HRT == head->msgtype) {
            rt_kprintf("LORA_STD_MSG_PROT_HRT\n");
            if (0 == lora_std_lock()) {
                lora_mnode_online(cmd_head->addr, head->rssi);
                lora_std_unlock();
            }
        }
    } else if (LORA_STD_POST == head->packtype) {
        if (LORA_STD_MSG_PROT_ONLINE == head->msgtype) {
            rt_kprintf("LORA_STD_MSG_PROT_ONLINE\n");
            if (0 == lora_std_lock()) {
                __lora_send_info_req(cmd_head->addr);
                lora_std_unlock();
            }
        }
    } else if (LORA_STD_RSP == head->packtype) {
        if (LORA_STD_MSG_PROT_INFO == head->msgtype) {
            lora_std_info_t *info = (lora_std_info_t *)buffer;
            rt_kprintf("LORA_STD_MSG_PROT_INFO\n");
            if (0 == lora_std_lock()) {
                lora_mnode_insert(info, head->rssi);
                __lora_send_join_post(cmd_head->addr);
                lora_mnode_online(cmd_head->addr, head->rssi);
                lora_std_unlock();
            }
        } else if (LORA_STD_MSG_RSSI == head->msgtype) {
            lora_std_rssi_t *info = (lora_std_rssi_t *)buffer;
            rt_kprintf("LORA_STD_MSG_RSSI\n");
            if (0 == lora_std_lock()) {
                lora_mnode_update_rssi(info, cmd_head->addr, head->rssi);
                lora_std_unlock();
            }
        } else if (LORA_STD_MSG_DATA_MODBUS_RTU == head->msgtype) {
            uint32_t net_id = g_lora_cfg.info.addr;
            if (!head->p2p) {
                memcpy(&net_id, buffer, sizeof(net_id));
                buffer += sizeof(net_id);
            }
            if (net_id == g_lora_cfg.info.addr) {
                uint8_t *data = (uint8_t *)buffer;
                uint8_t data_len = head->msglen - sizeof(lora_std_head_t) - 4 - 2;
                if (head->p2p) data_len += 4;
                if (g_xfer_net_dst_uart_occ[BOARD_LORA_UART]) {
                    if(LORA_TM_GW == g_lora_cfg.tmode) {
                        rt_thddog_feed("xfer_dst_serial_recv_buffer");
                        xfer_dst_serial_recv_buffer(BOARD_LORA_UART, (void *)data, data_len);
                    }
                } else if(LORA_TM_TRT == g_lora_cfg.tmode) {
                    int8_t instance = nUartGetInstance(g_lora_cfg.dst_type);
                    if(instance >= 0) {
                        rt_thddog_feed("xfer_dst_serial_send -> trt");
                        xfer_dst_serial_send(instance, (const void *)data, data_len);
                    }
                } else {
                    // RTU 主从校验方法一致
                    if (data_len >= MB_RTU_PDU_SIZE_MIN &&
                        data_len < MB_RTU_PDU_SIZE_MAX &&
                        (das_crc16((uint8_t *)data, data_len) == 0)) {
                        // 目前只考虑单网关树形网络  2016/10/20

                        UCHAR ucAddress = data[MB_RTU_PDU_ADDR_OFF];

                        if (LORA_WORK_END_DEVICE == g_lora_cfg.work_mode) {
                            if(LORA_TM_DTU == g_lora_cfg.tmode) {
                                int instance = xfer_get_uart_with_slave_addr(ucAddress);
                                if(instance >= 0) {
                                    rt_thddog_feed("xfer_dst_serial_send -> dtu");
                                    xfer_dst_serial_send(instance, (const void *)data, data_len);
                                    goto _END;
                                }
                            }
                            bfifo_push(s_lora_fifo, (const unsigned char *)data, data_len, 0);
                        } else if (LORA_WORK_CENTRAL == g_lora_cfg.work_mode) {
                            bfifo_push(s_lora_fifo, (const unsigned char *)data, data_len, 0);
                        }
                    } else {
                        rt_kprintf("_lora_serial_timeout_handle : other data!\n");
                    }
                }
                lora_mnode_online(cmd_head->addr, head->rssi);
            } else {
                // wait 100 ms , do nothing
                if (!head->p2p) {
                    rt_thread_delay(rt_tick_from_millisecond(100));
                }
            }
        }
    }
_END:
    return ;
}

static void __lora_parse_thread(void *parameter)
{
    static lora_buf_queue_t _stdqueue;

    for (;;) {
        rt_thddog_feed("rt_mq_recv 1 sec");
        if (RT_EOK == rt_mq_recv(s_lora_std_queue, &_stdqueue, sizeof(lora_buf_queue_t), RT_WAITING_FOREVER)) {
            int remain = _stdqueue.len;
            int pos = 0;
            int index = 0;
            lora_cmd_head_t cmd_head;
            lora_std_head_t std_head;
            while (remain > 0) {
                memcpy(&cmd_head, &_stdqueue.buf[pos], sizeof(lora_cmd_head_t));
                pos += sizeof(lora_cmd_head_t);
                remain -= sizeof(lora_cmd_head_t);
                if ((remain >= (sizeof(lora_std_head_t) + 2)) && cmd_head.cmd == LORA_CMD_RECV) {
                    memcpy(&std_head, &_stdqueue.buf[pos], sizeof(lora_std_head_t));
                    rt_thddog_feed("__check_buffer");
                    if (__check_buffer(&std_head, &_stdqueue.buf[pos])) {
                        //rt_kprintf("recv packet[%d], addr = %08X, packtype = %d, msgtype = %d, msglen = %d\n", ++index, cmd_head.addr, std_head.packtype, std_head.msgtype, std_head.msglen);
                        rt_thddog_feed("lora_std_parse_buffer");
                        __lora_std_parse_buffer(&cmd_head, &std_head, &_stdqueue.buf[pos + sizeof(lora_std_head_t)]);
                        pos += std_head.msglen;
                        remain -= std_head.msglen;
                    } else {
                        pos++;
                        remain--;
                    }
                } else {
                    pos++;
                    remain--;
                }
            }
        }
    }

    rt_thddog_exit();
}

static uint32_t s_lora_last_sync_sec = 0;

void lora_learn_now(void)
{
    s_lora_last_sync_sec = 0;
}

static void __lora_worker_thread(void *parameter)
{
    for (;;) {
        uint32_t nowsec = das_sys_time();
        if (LORA_WORK_CENTRAL == g_lora_cfg.work_mode) {
            if (s_lora_last_sync_sec == 0 || (nowsec - s_lora_last_sync_sec >= g_lora_cfg.learnstep)) {
                rt_thddog_feed("begin snd sync buffer");
                if (0 == lora_std_lock()) {
                    __lora_send_sync_post();
                    lora_std_unlock();
                }
                rt_thddog_feed("end snd learn buffer");
                s_lora_last_sync_sec = nowsec;
            }
        }
        rt_thddog_feed("lora_check_online");
        lora_check_online();
        
        if (0 == lora_std_lock()) {
            lora_mnode_t *node = lora_mnode_check_hrt();
            if (node) {
                rt_thddog_feed("__lora_send_hrt_post");
                __lora_send_hrt_post(node->netid);
            }
            lora_std_unlock();
        }
        if (0 == lora_std_lock()) {
            lora_mnode_t *node = lora_mnode_check_rssi();
            if (node) {
                rt_thddog_feed("__lora_send_rssi_req");
                __lora_send_rssi_req(node->netid);
            }
            lora_std_unlock();
        }
        rt_thread_delay(RT_TICK_PER_SECOND);
    }

    rt_thddog_exit();
}

static int s_single_mode_flag = 1;

int lora_set_dst_addr(uint32_t addr)
{
    g_lora_cfg.dst_addr = addr;
    return 0;
}

void lora_set_broad_mode(void)
{
    s_single_mode_flag = 0;
}

void lora_set_single_mode(void)
{
    s_single_mode_flag = 1;
}

int lora_send_buffer(uint8_t *buffer, int n, int type)
{
    int result = 0;
    uint16_t check = 0;
    lora_enter_data_mode();
    if (s_lora_serial && buffer && n > 0) {
        lora_cmd_head_t _cmd_head = {
            .cmd    = LORA_CMD_SEND, 
            .addr   = g_lora_cfg.info.maddr, 
            .op     = 0, 
            .seq    = 0, 
        };
        if (!s_single_mode_flag) {
            _cmd_head.addr = g_lora_cfg.info.maddr;
        }
        __lora_hold_do();
        result = __lora_send_buffer(&_cmd_head, sizeof(lora_cmd_head_t));
        if (type == LORA_STD_MSG_DATA_MODBUS_RTU) {
            lora_std_head_t head = {
                .pre        = { LORA_STD_PRE0, LORA_STD_PRE1 }, 
                .packtype   = LORA_STD_REQ, 
                .p2p        = 0, 
                .msgtype    = LORA_STD_MSG_DATA_MODBUS_RTU, 
                .rssi       = 0, 
                .msglen     = sizeof(lora_std_head_t) + n + 4 + 2, 
            };
            __lora_send_buffer(&head, sizeof(lora_std_head_t));
            check = __lora_get_check(check, (uint8_t *)&head, sizeof(lora_std_head_t));
            __lora_send_buffer(&g_lora_cfg.dst_addr, sizeof(g_lora_cfg.dst_addr));
            check = __lora_get_check(check, (uint8_t *)&g_lora_cfg.dst_addr, sizeof(g_lora_cfg.dst_addr));
        } else if (type == LORA_STD_MSG_DATA_TRT) {
            ;
        }
        if (result > 0) {
            result = __lora_send_buffer(buffer, n);
            check = __lora_get_check(check, (uint8_t *)buffer, n);
        }
        __lora_send_buffer(&check, sizeof(check));
        __lora_hold_refresh();
    }
    lora_exit_data_mode();
    return result;
}

// for webserver
DEF_CGI_HANDLER(doLoRaLearnNow)
{
    rt_kprintf("lora_learn_now()\n");

    WEBS_PRINTF("{\"ret\":%d}", RT_EOK);
    WEBS_DONE(200);

    lora_learn_now();
}

static ssize_t __mb_lora_send(modbus_t *ctx, const uint8_t *req, int req_length)
{
    int result = -1;
    modbus_user_data_t *user_data = (modbus_user_data_t *)modbus_get_user_data(ctx);
    if (req && req_length > 0) {
        if (0 == lora_std_lock()) {
            result = lora_send_buffer((uint8_t *)req, req_length, LORA_STD_MSG_DATA_MODBUS_RTU);
            lora_std_unlock();
        }
    }
    return result;
}

static ssize_t __mb_lora_recv(modbus_t *ctx, uint8_t *rsp, int rsp_length)
{
    return bfifo_pull(s_lora_fifo, (unsigned char *)rsp, rsp_length, 0);
}

static int __mb_lora_connect(modbus_t *ctx)
{
    modbus_user_data_t *user_data = (modbus_user_data_t *)modbus_get_user_data(ctx);
    modbus_set_socket(ctx, 0);
    bfifo_reset(s_lora_fifo);
    return (s_lora_fifo ? 0 : -1);
}

static void __mb_lora_close(modbus_t *ctx)
{
    modbus_set_socket(ctx, -1);
}

static int __mb_lora_select(modbus_t *ctx, fd_set *rset, struct timeval *tv, int msg_length)
{
    int rc = -1;
    if (s_lora_fifo) {
        rc = bfifo_pull_wait(s_lora_fifo, tv ? tv->tv_usec + tv->tv_sec * 1000000 : -1);
        if (rc == 0) {
            errno = ETIMEDOUT;
            rc = -1;
        }
    } else {
        errno = EBADF;
    }
    return rc;
}

modbus_customize_backend_t g_lora_customize_backend = {
    .send       = __mb_lora_send, 
    .recv       = __mb_lora_recv, 
    .connect    = __mb_lora_connect, 
    .close      = __mb_lora_close, 
    .select     = __mb_lora_select, 
};


