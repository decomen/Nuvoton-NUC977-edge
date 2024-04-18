#include "board.h"
#include "das_monitor.h"

static const monitor_cfg_t c_monitor_default_cfg = { 0 };

monitor_cfg_t g_monitor_cfg = { 0 };

static bfifo_t s_monitor_uart_fifo_in[BOARD_UART_MAX];
static bfifo_t s_monitor_uart_fifo_out[BOARD_UART_MAX];
static FILE *s_monitor_uart_fp[BOARD_UART_MAX];
static rt_thread_t s_monitor_uart_thread_in[BOARD_UART_MAX];
static rt_thread_t s_monitor_uart_thread_out[BOARD_UART_MAX];
static rt_mutex_t s_monitor_uart_mutex[BOARD_UART_MAX];
static uint32_t s_monitor_last_flush_time[BOARD_UART_MAX];
static rt_mutex_t s_monitor_mutex;

typedef struct monitor_file {
    char name[24];
    uint32_t date;
    uint32_t index;
    uint32_t size;
    uint32_t mtime;
} monitor_file_t;

void monitor_cfg_set_default(void)
{
    g_monitor_cfg = c_monitor_default_cfg;
}

void monitor_cfg_read_from_fs(void)
{
    if (!board_cfg_read(MONITOR_CFG_NAME, &g_monitor_cfg, sizeof(g_monitor_cfg))) {
        monitor_cfg_set_default();
    }
}

void monitor_cfg_save_to_fs(void)
{
    if (!board_cfg_write(MONITOR_CFG_NAME, &g_monitor_cfg, sizeof(g_monitor_cfg))) {
        monitor_cfg_set_default();
    }
}

/*
static int __monitor_file_cmp(const void * a, const void * b)
{
    monitor_file_t *_a = (monitor_file_t *)a;
    monitor_file_t *_b = (monitor_file_t *)b;
    if (_a->date == _b->date)
        return _a->index - _b->index;
    else 
        return _a->date - _b->date;
}
*/

static int __monitor_file_cmp(const void * a, const void * b)
{
    monitor_file_t *_a = (monitor_file_t *)a;
    monitor_file_t *_b = (monitor_file_t *)b;
    return _a->mtime - _b->mtime;
}

static void __monitor_file_cleanup()
{
    int file_num = 0;
    if (das_get_dir_size(BOARD_MONITOR_PATH, &file_num) >= MONITOR_FILE_TOTAL_SIZE) {
        DIR *d = NULL;
        struct dirent *de;
        struct stat buf;
        int ofs = 0;
        char tmp[256 + 32];
        monitor_file_t *list = malloc(sizeof(monitor_file_t) * file_num);
        
        d = opendir(BOARD_MONITOR_PATH);
        if (list && d) {
            memset(list, 0, sizeof(monitor_file_t) * file_num);
            for (de = readdir(d); de != NULL && ofs < file_num; de = readdir(d)) {
                sprintf(tmp, BOARD_MONITOR_PATH "/%s", de->d_name);
                if (stat(tmp, &buf) >= 0 && !S_ISDIR(buf.st_mode)) {
                    snprintf(list[ofs].name, 23, "%s", de->d_name);
                    {
                        char *p = list[ofs].name;
                        while (*p != '_' && *p != '\0') p++;
                        if (*p == '_') {
                            p++;
                            list[ofs].date = atol(p);
                            while (*p != '_' && *p != '\0') p++;
                            if (*p == '_') {
                                p++;
                                list[ofs].index = atol(p);
                            }
                        }
                    }
                    list[ofs].size = (uint32_t)buf.st_size;
                    list[ofs].mtime = (uint32_t)buf.st_mtime;
                    ofs++;
                }
            }
            file_num = ofs;
            if (ofs > 0) {
                qsort(list, ofs, sizeof(monitor_file_t), __monitor_file_cmp);
                for (int n = 0; n < ofs; n++) {
                    if (das_get_dir_size(BOARD_MONITOR_PATH, &file_num) >= MONITOR_FILE_TOTAL_SIZE - MONITOR_UART_FILE_SIZE) {
                        sprintf(tmp, "rm -f "BOARD_MONITOR_PATH "/%s", list[n].name);
                        rt_kprintf("%s\n", tmp);
                        my_system(tmp);
                    } else {
                        break;
                    }
                }
            }
        }
        if (d) closedir(d);
        if (list) free(list);
    }
}

static FILE *__monitor_uart_open_file(int com)
{
    char path[256] = {0};
    time_t now = time(0);
    struct tm lt;
    int idx = 0;
    das_localtime_r(&now, &lt);

    for (idx = 0; idx < 512; idx++) {
        struct stat buf;
        sprintf(path, BOARD_MONITOR_PATH "COM%d_%04d%02d%02d_%d.log", com + 1, lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, idx);
	    if (stat(path, &buf) >= 0) {
            break;
        }
    }
    if (idx >= 512) idx = 0;
    
    for ( ; idx < 512; idx++) {
        sprintf(path, BOARD_MONITOR_PATH "COM%d_%04d%02d%02d_%d.log", com + 1, lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, idx);
        {
            int f_sz = das_get_file_length(path);
            if (f_sz < MONITOR_UART_FILE_SIZE) {
                return fopen(path, "a+");
            }
        }
    }
    return NULL;
}

static void __monitor_uart_add_line_in(int com, int idx, const uint8_t *line, int len)
{
    if (len > 0) {
        time_t now = time(0);
        struct tm lt;
        das_localtime_r(&now, &lt);
        
        rt_mutex_take(s_monitor_uart_mutex[com]);
        {
            fprintf(s_monitor_uart_fp[com], "%02d:%02d:%02d[R%d]:", lt.tm_hour, lt.tm_min, lt.tm_sec, idx);
            for (int pos = 0; pos < len; pos++) {
                fprintf(s_monitor_uart_fp[com], "%02X", line[pos]);
            }
            fputs("\r\n", s_monitor_uart_fp[com]);
            if (das_sys_time() - s_monitor_last_flush_time[com] > 10) {
                fflush(s_monitor_uart_fp[com]);
                s_monitor_last_flush_time[com] = das_sys_time();
            }
        }
        rt_mutex_release(s_monitor_uart_mutex[com]);
    } else {
        if (das_sys_time() - s_monitor_last_flush_time[com] > 10) {
            rt_mutex_take(s_monitor_uart_mutex[com]);
            fflush(s_monitor_uart_fp[com]);
            rt_mutex_release(s_monitor_uart_mutex[com]);
            s_monitor_last_flush_time[com] = das_sys_time();
        }
    }
}

static void __monitor_uart_add_line_out(int com, int idx, const uint8_t *line, int len)
{
    if (len > 0) {
        time_t now = time(0);
        struct tm lt;
        das_localtime_r(&now, &lt);
        
        rt_mutex_take(s_monitor_uart_mutex[com]);
        {
            fprintf(s_monitor_uart_fp[com], "%02d:%02d:%02d[S%d]:", lt.tm_hour, lt.tm_min, lt.tm_sec, idx);
            for (int pos = 0; pos < len; pos++) {
                fprintf(s_monitor_uart_fp[com], "%02X", line[pos]);
            }
            fputs("\r\n", s_monitor_uart_fp[com]);
            if (das_sys_time() - s_monitor_last_flush_time[com] > 10) {
                fflush(s_monitor_uart_fp[com]);
                s_monitor_last_flush_time[com] = das_sys_time();
            }
        }
        rt_mutex_release(s_monitor_uart_mutex[com]);
    } else {
        if (das_sys_time() - s_monitor_last_flush_time[com] > 10) {
            rt_mutex_take(s_monitor_uart_mutex[com]);
            fflush(s_monitor_uart_fp[com]);
            rt_mutex_release(s_monitor_uart_mutex[com]);
            s_monitor_last_flush_time[com] = das_sys_time();
        }
    }
}

static void __monitor_uart_worker_in(void *parameter)
{
    int com = (int)(long)parameter;
    uint8_t tmp[256];
    uint8_t line[MONITOR_UART_LINE_LEN];
    int idx = 0;
    while (1) {
        int pos = 0, remain = MONITOR_UART_LINE_LEN;
        while (remain > 0) {
            int len = bfifo_pull(s_monitor_uart_fifo_in[com], &line[pos], remain, 50000);
            if (len > 0) {
                pos += len;
                remain -= len;
            } else {
                break;
            }
        }
        __monitor_uart_add_line_in(com, idx, line, pos);
        if (remain <= 0) {
            idx++;
        } else {
            idx = 0;
        }
    }
}

static void __monitor_uart_worker_out(void *parameter)
{
    int com = (int)(long)parameter;
    uint8_t tmp[256];
    uint8_t line[MONITOR_UART_LINE_LEN];
    int idx = 0;
    while (1) {
        int pos = 0, remain = MONITOR_UART_LINE_LEN;
        while (remain > 0) {
            int len = bfifo_pull(s_monitor_uart_fifo_out[com], &line[pos], remain, 50000);
            if (len > 0) {
                pos += len;
                remain -= len;
            } else {
                break;
            }
        }
        __monitor_uart_add_line_out(com, idx, line, pos);
        if (remain <= 0) {
            idx++;
        } else {
            idx = 0;
        }
    }
}

static bool __monitor_uart_open(int com)
{
    bool result = false;
    
    if (com < BOARD_UART_MAX) {
        rt_mutex_take(s_monitor_mutex);
        if (s_monitor_uart_mutex[com] == NULL) {
            s_monitor_uart_mutex[com] = rt_mutex_create("monitor_uart");
        }
        if (NULL == s_monitor_uart_fifo_in[com]) {
            s_monitor_uart_fifo_in[com] = bfifo_create(MONITOR_UART_FIFO_LEN);
        }
        if (NULL == s_monitor_uart_fifo_out[com]) {
            s_monitor_uart_fifo_out[com] = bfifo_create(MONITOR_UART_FIFO_LEN);
        }
        if (s_monitor_uart_mutex[com]) rt_mutex_take(s_monitor_uart_mutex[com]);
        if (NULL == s_monitor_uart_fp[com]) {
            s_monitor_uart_fp[com] = __monitor_uart_open_file(com);
        }
        if (s_monitor_uart_fifo_in[com] != NULL && s_monitor_uart_fifo_out[com] != NULL && s_monitor_uart_fp[com] != NULL) {
            if (ftell(s_monitor_uart_fp[com]) > MONITOR_UART_FILE_SIZE) {
                fclose(s_monitor_uart_fp[com]);
                __monitor_file_cleanup();
                s_monitor_uart_fp[com] = __monitor_uart_open_file(com);
            }
        }
        if (s_monitor_uart_mutex[com]) rt_mutex_release(s_monitor_uart_mutex[com]);
        if (s_monitor_uart_fifo_in[com] != NULL && s_monitor_uart_fifo_out[com] != NULL && s_monitor_uart_fp[com] != NULL) {
            if (s_monitor_uart_mutex[com] != NULL) {
                if (s_monitor_uart_thread_in[com] == NULL) {
                    s_monitor_uart_thread_in[com] = \
                        rt_thread_create("monitor_uart_in", __monitor_uart_worker_in, (void *)(long)com, 0, 0, 0);
                    if (s_monitor_uart_thread_in[com]) {
                        rt_thread_startup(s_monitor_uart_thread_in[com]);
                    }
                }
                if (s_monitor_uart_thread_out[com] == NULL) {
                    s_monitor_uart_thread_out[com] = \
                        rt_thread_create("monitor_uart_out", __monitor_uart_worker_out, (void *)(long)com, 0, 0, 0);
                    if (s_monitor_uart_thread_out[com]) {
                        rt_thread_startup(s_monitor_uart_thread_out[com]);
                    }
                }
            }
        }
        result = (s_monitor_uart_thread_out[com] != NULL && s_monitor_uart_fp[com] != NULL);
        rt_mutex_release(s_monitor_mutex);
    }
    
    return result;
}

static void __monitor_uart_init(void)
{
    for (int n = 0; n < BOARD_UART_MAX; n++) {
        s_monitor_uart_fifo_in[n] = NULL;
        s_monitor_uart_fifo_out[n] = NULL;
        s_monitor_uart_fp[n] = NULL;
        s_monitor_uart_mutex[n] = NULL;
        s_monitor_uart_thread_in[n] = NULL;
        s_monitor_uart_thread_out[n] = NULL;
        s_monitor_last_flush_time[n] = das_sys_time();
    }
}

int monitor_init(void)
{
    my_system("mkdir -p " BOARD_MONITOR_PATH);
    s_monitor_mutex = rt_mutex_create("monitor");
    monitor_cfg_read_from_fs();
    __monitor_uart_init();
}

bool monitor_uart_recv_data(int com, uint8_t *data, int d_len)
{
    if (g_monitor_cfg.uart_enable[com] && das_time_is_sync() && __monitor_uart_open(com)) {
        bfifo_push(s_monitor_uart_fifo_in[com], (const unsigned char *)data, d_len, 0);
    }
}

bool monitor_uart_send_data(int com, uint8_t *data, int d_len)
{
    if (g_monitor_cfg.uart_enable[com] && das_time_is_sync() && __monitor_uart_open(com)) {
        bfifo_push(s_monitor_uart_fifo_out[com], (const unsigned char *)data, d_len, 0);
    }
}

DEF_CGI_HANDLER(set_monitor_cfg)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        g_monitor_cfg.uart_enable[0] = cJSON_GetInt(pCfg, "com1", 0);
        g_monitor_cfg.uart_enable[1] = cJSON_GetInt(pCfg, "com2", 0);
        //g_monitor_cfg.uart_enable[2] = cJSON_GetInt(pCfg, "com3", 0);
        //g_monitor_cfg.uart_enable[3] = cJSON_GetInt(pCfg, "com4", 0);

        monitor_cfg_save_to_fs();
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}

DEF_CGI_HANDLER(get_monitor_cfg)
{
    char *szRetJSON = RT_NULL;
    cJSON *pItem = cJSON_CreateObject();
    if (pItem) {
        cJSON_AddNumberToObject(pItem, "ret", RT_EOK);
        cJSON_AddNumberToObject(pItem, "com1", g_monitor_cfg.uart_enable[0]);
        cJSON_AddNumberToObject(pItem, "com2", g_monitor_cfg.uart_enable[1]);
        //cJSON_AddNumberToObject(pItem, "com3", g_monitor_cfg.uart_enable[2]);
        //cJSON_AddNumberToObject(pItem, "com4", g_monitor_cfg.uart_enable[3]);
        szRetJSON = cJSON_PrintUnformatted(pItem);
        if(szRetJSON) {
            WEBS_PRINTF( szRetJSON );
            rt_free( szRetJSON );
        }
    }
    cJSON_Delete( pItem );

    WEBS_DONE(200);
}

