
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/stat.h>

#include "das_util.h"

#include <board.h>

char *das_strdup(char *str_old, const char *str_new)
{
    if (str_new && str_old != str_new) {
        if (str_old) free(str_old);
        {
            size_t len = strlen(str_new) + 1;
            void *new = malloc(len);
            if (new == NULL) return NULL;
            return (char *)memcpy(new, str_new, len);
        }
    }
    return str_old;
}

char *das_strncpy(char *dest, const char *src, int n)
{
    if (dest && src) {
        if (dest != src)
            strncpy(dest, src, n - 1);
        
        dest[n - 1] = '\0';
    }
    return dest;
}

int das_string_equals(const char *s1, const char *s2, int ignore_case)
{
    if (s1 && s2) {
        return 0 == (ignore_case ? strcasecmp(s1, s2) : strcmp(s1, s2));
    }
    return 0;
}

int das_string_startwith(const char *s, const char *start, int ignore_case)
{
    if (s && start) {
        return 0 == (ignore_case ? strncasecmp(s, start, strlen(start)) : strncmp(s, start, strlen(start)));
    }
    return 0;
}

char *das_trim_all(char *s)
{
    char *start;
    char *end;
    int len = strlen(s);
    
    start = s;
    end = s + len - 1;
    
    while(1) {
        char c = *start;
        if( c != ' ' ) break;
        if( ++start > end ) {
            s[0] = '\0';
            return s;
        }
    }
    while(1) {
        char c = *end;
        if( c != ' ' ) break;
        if( start > --end ) {
            s[0] = '\0';  
            return s;
        }  
    }
    end[1] = '\0';
    return start;
}

int das_get_cmd_result(const char *cmd, char *result, int sz)
{
    char buffer[2048] = {0};
    char *p = result;
    int ofs = 0;
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        return -1;
    }
    while(!feof(pipe) && (sz - ofs > 0)) {
        if(fgets(buffer, sizeof(buffer), pipe)) {
            ofs += snprintf(&p[ofs], sz - ofs, "%s", buffer);
        }
    }
    pclose(pipe);
    return 0;
}

int das_check_process(const char *process)
{
    FILE* fp;
    char buf[256];
    int count = 0;

    sprintf(buf, "ps -ef | grep '%s' | grep -v grep | wc -l", process);
    if((fp = popen(buf, "r")) != NULL) {
        if((fgets(buf, 256, fp))!= NULL) {
            count = atoi(buf);
        }
        pclose(fp);
        return count;
    }
    return 0;
}

void das_kill_process(const char *process)
{
    FILE* fp;
    char buf[256];
    int pid = 0;

    sprintf(buf, "ps -ef | grep '%s' | grep -v grep", process);
    if((fp = popen(buf, "r")) != NULL) {
        if((fgets(buf, 256, fp))!= NULL) {
            pid = atoi(buf);
            sprintf(buf, "kill %u", pid);
            system(buf);
        }
        pclose(fp);
    }
}

uint32_t das_get_time()
{
	return time(0);
}

void das_set_time(uint32_t timestamp, int tz)
{
	struct timeval tv;  
	tv.tv_sec = timestamp;  
    tv.tv_usec = 0;  
	struct timezone timezone = {
		 tz / 60,
		 0
	};
	settimeofday(&tv, &timezone);
}

struct tm *das_localtime(const time_t *timep)
{
    time_t t1 = (timep ? *timep : time(0)) + g_host_cfg.nTimezone;
    return gmtime(&t1);
}

struct tm *das_localtime_r(const time_t *timep, struct tm *result)
{
    time_t t1 = (timep ? *timep : time(0)) + g_host_cfg.nTimezone;
    return gmtime_r(&t1, result);
}

int das_time_get_day_min(time_t t)
{
    struct tm lt;
    das_localtime_r(&t, &lt);
    return lt.tm_hour * 60 + lt.tm_min;
}

int das_time_get_hour(time_t t)
{
    struct tm lt;
    das_localtime_r(&t, &lt);
    return lt.tm_hour;
}

int das_time_is_sync(void)
{
    return (das_get_time() >= 1514736000);
}

static uint32_t s_start_time_sec = 0;

void das_sys_time_init(void)
{
    struct timespec m_time = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &m_time);
	s_start_time_sec = m_time.tv_sec;
}

uint32_t das_sys_time()
{
    struct timespec m_time = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &m_time);
	return m_time.tv_sec - s_start_time_sec;
}

uint64_t das_sys_msectime(void) 
{
    struct timespec m_time = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &m_time);
	return ((m_time.tv_sec - s_start_time_sec) * 1000 + m_time.tv_nsec / 1000000);
}

extern int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);

int pthread_init_mutex(pthread_mutex_t *mutex)
{
    if (mutex) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
        pthread_mutex_init(mutex, &attr);
        pthread_mutexattr_destroy(&attr);
    }
    return 0;
}

sem_t *pthread_create_sem(int v)
{
    sem_t *sem = malloc(sizeof(*sem));
    if (sem) {
         sem_init(sem, 0, v);
    }
    return sem;
}

pthread_mutex_t *pthread_create_mutex(void)
{
    pthread_mutex_t *mutex = malloc(sizeof(*mutex));
    if (mutex) {
         pthread_init_mutex(mutex);
    }
    return mutex;
}

void pthread_delete_mutex(pthread_mutex_t *mutex)
{
    if (mutex) {
        pthread_mutex_destroy(mutex);
        free(mutex);
    }
}

static uint8_t _aucCRCHi[] = {
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40
};

static uint8_t _aucCRCLo[] = {
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7,
    0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E,
    0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9,
    0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC,
    0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32,
    0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D,
    0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 
    0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF,
    0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1,
    0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4,
    0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 
    0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA,
    0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
    0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97,
    0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E,
    0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89,
    0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83,
    0x41, 0x81, 0x80, 0x40
};

uint16_t das_crc16(uint8_t *data, int len)
{
    uint8_t ucCRCHi = 0xFF;
    uint8_t ucCRCLo = 0xFF;
    int index;
    while (len--) {
        index = ucCRCLo ^ *(data++);
        ucCRCLo = (uint8_t)( ucCRCHi ^ _aucCRCHi[index]);
        ucCRCHi = _aucCRCLo[index];
    }
    return (uint16_t)( ucCRCHi << 8 | ucCRCLo );
}

void das_delay(uint32_t usec)
{
    struct timeval delay;
    if (usec == 0) usec = 1000;
    delay.tv_sec = usec / 1000000;
    delay.tv_usec = usec % 1000000;
    select(0, NULL,NULL, NULL, &delay);
}

int das_mkdir_p(const char *path, mode_t mode)
{
    struct stat s;
    int i, len = strlen(path);
    char dir_path[256];
    
    if( dir_path ) {
        dir_path[len] = '\0';
        strncpy(dir_path, path, len);
        for( i=0; i<len; i++ ) {
            if( dir_path[i] == '/' && i > 0 ) {
                dir_path[i]='\0';
                if( stat( dir_path, &s ) < 0 ) {
                    if( mkdir(dir_path, mode) < 0 ) {
                        printf("Err: mkdir_p=%s\n", dir_path);
                        return -1;
                    }
                }
                dir_path[i]='/';
            }
        }
    } else {
        return -1;
    }
    return 0;
}

void das_swap_uint16_buffer(uint16_t *buffer, int nb)
{
    int n;
    for (n = 0; n < nb; n++) {
        buffer[n] = (buffer[n] << 8) | (buffer[n] >> 8);
    }
}

int das_get_file_length(const char *path)
{
    /*FILE *f = fopen(path, "r");
    int sz = 0;
    if (f) {
        fseek (f, 0, SEEK_END);
        sz = ftell(f);
        fclose(f);
    }
    return sz;*/
    
	struct stat buf;
	if (stat(path, &buf) < 0) {
		return 0;
	} else {
		return buf.st_size;
	}
}

int das_get_dir_size(const char *dir_path, int *file_num)
{
    DIR *d;
    struct dirent *de;
    struct stat buf;
    int exists;
    int total_size = 0;
    char full_path[256];
    
    d = opendir(dir_path);
    *file_num = 0;
    
    if (d == NULL) return 0;
    
    for (de = readdir(d); de != NULL; de = readdir(d)) {
        sprintf(full_path, "%s/%s", dir_path, de->d_name);
        exists = stat(full_path, &buf);
        if (exists >= 0) {
            total_size += buf.st_size;
            *file_num = *file_num + 1;
        }
    }
    
    closedir(d);
    return total_size;
}

char *das_read_text_file(const char *path)
{
    char *buffer = NULL;
    FILE *f = fopen(path, "r");
    int sz = 0;
    if (f) {
        fseek (f, 0, SEEK_END);
        sz = ftell(f);
        rewind(f);
        if (sz > 0) {
            buffer = malloc(sz + 1);
            if (buffer) {
                sz = fread(buffer, 1, sz, f);
                buffer[sz] = '\0';
            }
        }
        fclose(f);
    }
    return buffer;
}

int das_write_text_file(const char *path, const char *buffer, int sz)
{
    if (buffer && sz >= 0) {
        FILE *f = fopen(path, "w+");
        if (f) {
            if (sz > 0) {
                sz = fwrite(buffer, 1, sz, f);
            }
            fclose(f);
        }
        return sz;
    }
    return -1;
}


const char *ntpd_servers[] = {
    "cn.ntp.org.cn", "ntp2.aliyun.com", 
    "s1b.time.edu.cn", 
    "0.asia.pool.ntp.org", "0.cn.pool.ntp.org", 
    "1.asia.pool.ntp.org", "1.cn.pool.ntp.org", 
    "time.windows.com", 
};


static int __ntp_server_max = 8;
static int __ntp_server_index = 0;



void das_ntpd_try_sync(void)
{
    static uint32_t last_sync_time = 0;
    if (0 == last_sync_time || das_sys_time() - last_sync_time >= 24 * 60 * 60 || (!das_time_is_sync() && !das_check_process("ntpd"))) {
        system("killall ntpd");
        {
            if( (strlen(g_host_cfg.szNTP[0])<=0) && (strlen(g_host_cfg.szNTP[1])<=0) ){
                printf("#### use inter ntpd set time\n");
                char cmd[128] = {0};
                sprintf(cmd, "ntpd -qNnd -p %s &", ntpd_servers[__ntp_server_index]);
                __ntp_server_index++;
                if (__ntp_server_index >= __ntp_server_max) __ntp_server_index = 0;
                system(cmd);
                if (das_sys_time() - last_sync_time >= 24 * 60 * 60) __ntp_server_index = 0;
            }else {
                char cmd[128] = {0};
                if(strlen(g_host_cfg.szNTP[1]) > 0){
                    sprintf(cmd, "ntpd -qNnd -p %s &", g_host_cfg.szNTP[1]); 
                    printf("#### use %s ntpd set time\n",g_host_cfg.szNTP[1]);
                }
                if(strlen(g_host_cfg.szNTP[0]) > 0){
                    sprintf(cmd, "ntpd -qNnd -p %s &", g_host_cfg.szNTP[0]); 
                    printf("#### use %s ntpd set time\n",g_host_cfg.szNTP[0]);
                }
                system(cmd);   
            }
        }
        last_sync_time = das_sys_time();
    }
}

