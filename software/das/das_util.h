#ifndef __DAS_UTIL_H__
#define __DAS_UTIL_H__

#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <dirent.h>

char *das_strdup(char *str_old, const char *str_new);
char *das_strncpy(char *dest, const char *src, int n);
#define das_strcpy_s(_dest, _src) das_strncpy(_dest, _src, sizeof(_dest))
int das_string_equals(const char *s1, const char *s2, int ignore_case);
int das_string_startwith(const char *s, const char *start, int ignore_case);
char *das_trim_all(char *s);

void das_sys_time_init(void);
uint32_t das_get_time(void);
void das_set_time(uint32_t timestamp, int tz);
struct tm *das_localtime(const time_t *timep);
struct tm *das_localtime_r(const time_t *timep, struct tm *result);

int das_time_get_day_min(time_t t);
int das_time_get_hour(time_t t);
int das_time_is_sync(void);

uint32_t das_sys_time(void);
uint64_t das_sys_msectime(void) ;

int pthread_init_mutex(pthread_mutex_t *mutex);
pthread_mutex_t *pthread_create_mutex(void);
void pthread_delete_mutex(pthread_mutex_t *mutex);

sem_t *pthread_create_sem(int v);

uint16_t das_crc16(uint8_t *data, int len);
void das_delay(uint32_t usec);

int das_mkdir_p(const char *path, mode_t mode);

void das_swap_uint16_buffer(uint16_t *buffer, int nb);

int das_get_cmd_result(const char *cmd, char *result, int sz);
int das_check_process(const char *process);
void das_kill_process(const char *process);

int das_get_file_length(const char *path);
int das_get_dir_size(const char *dir_path, int *file_num);
char *das_read_text_file(const char *path);
int das_write_text_file(const char *path, const char *buffer, int sz);

void das_ntpd_try_sync(void);

#endif

