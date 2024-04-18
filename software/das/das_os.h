#ifndef __DAS_OS_H__
#define __DAS_OS_H__

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#include "block_msg_queue.h"

void das_os_init(void);
void das_os_lock(void);
void das_os_unlock(void);

struct rt_thread {
    char        name[RT_NAME_MAX];
    pthread_t   tid;
    sem_t       start_sem;
    sem_t       run_sem;
    void (*entry)(void *parameter);
    void *parameter;
    void *user_data;
};
typedef struct rt_thread *rt_thread_t;

struct rt_timer {
    char            name[RT_NAME_MAX];
    int             flag;
    pthread_t       tid;
    sem_t           run_sem;
    pthread_mutex_t mutex;
    pthread_cond_t  c_ctrl;
    void (*timeout_func)(void *parameter);
    void        *parameter;
    rt_time_t   timeout;
};
typedef struct rt_timer *rt_timer_t;

#define RT_WAITING_FOREVER              -1
#define RT_WAITING_NO                   0

#define RT_IPC_FLAG_FIFO                0x00
#define RT_IPC_FLAG_PRIO                0x01

#define RT_TIMER_FLAG_DEACTIVATED       0x0             /**< timer is deactive */
#define RT_TIMER_FLAG_ACTIVATED         0x1             /**< timer is active */
#define RT_TIMER_FLAG_ONE_SHOT          0x0             /**< one shot timer */
#define RT_TIMER_FLAG_PERIODIC          0x2             /**< periodic timer */

#define RT_TIMER_CTRL_SET_TIME          0x0             /**< set timer control command */
#define RT_TIMER_CTRL_GET_TIME          0x1             /**< get timer control command */
#define RT_TIMER_CTRL_SET_ONESHOT       0x2             /**< change timer to one shot */
#define RT_TIMER_CTRL_SET_PERIODIC      0x3             /**< change timer to periodic */

//#define NVIC_SystemReset()              do { bStorageDoClose(); board_cfg_uinit(); storage_log_uinit(); webnet_exit();   exit(0); } while (0)
#define NVIC_SystemReset()              do { bStorageDoClose(); board_cfg_uinit(); storage_log_uinit(); /*webnet_exit();*/ my_system("reboot"); } while (0)
#define NVIC_SystemReboot()             NVIC_SystemReset()
#define rt_tick_get()                   das_sys_msectime()
#define rt_tick_from_millisecond(n)     (n)
#define rt_millisecond_from_tick(n)     (n)

#define rt_thread_delay(n)              das_delay(1000*(n))

#define RT_ASSERT(_c)                   if (!(_c)) printf("(%s) has assert failed at %s:%d.\n", #_c, __FUNCTION__, __LINE__)
#define LWIP_ASSERT(_s,_c)              if (!(_c)) printf("(%s) has assert failed at %s:%d.\n", #_c, __FUNCTION__, __LINE__)
#define lwip_socket                     socket
#define lwip_read                       read
#define lwip_write                      write
#define lwip_send                       send
#define lwip_sendto                     sendto
#define lwip_recvfrom                   recvfrom
#define lwip_accept                     accept
#define lwip_connect                    connect
#define lwip_shutdown                   shutdown
#define lwip_close                      close
#define lwip_select                     select
#define lwip_listen                     listen
#define lwip_bind                       bind
#define lwip_setsockopt                 setsockopt
#define lwip_fcntl                      fcntl
#define lwip_getsockname                getsockname
#define lwip_getpeername                getpeername
#define lwip_gethostbyname              gethostbyname
#define lwip_get_error(...)             errno

#define rt_kprintf                      printf
#define rt_sprintf                      sprintf
#define rt_snprintf                     snprintf
#define rt_vsnprintf                    vsnprintf
#define rt_malloc                       malloc
#define rt_realloc                      realloc
#define rt_calloc                       calloc
#define rt_free(p)                      if (p)  { free((void *)(p)); (p) = NULL;}

#define rt_strcasecmp                   strcasecmp
#define rt_trim                         das_trim_all

#define rt_memset                       memset
#define rt_vsprintf                     vsprintf
#define rt_memcpy                       memcpy

#define rt_strlen(s)                    (s ? strlen(s) : -1)
#define rt_strdup(s)                    (s ? strdup(s) : NULL)
#define rt_strncpy                      strncpy
#define rt_strcpy                       strcpy

typedef pthread_mutex_t *               rt_mutex_t;
#define rt_mutex_create(...)            pthread_create_mutex()
#define rt_mutex_init(m,...)            pthread_init_mutex(m)
#define rt_mutex_take(m,...)            pthread_mutex_lock(m)
#define rt_mutex_try_take(m,...)        pthread_mutex_trylock(m)
#define rt_mutex_release(m)             pthread_mutex_unlock(m)
#define rt_mutex_delete(m)              pthread_delete_mutex(m)

typedef sem_t *                         rt_sem_t;
#define rt_sem_create(name,v,flag)      pthread_create_sem(v)
#define rt_sem_init(sem,name,v,flag)    sem_init(sem, 0, v)
#define rt_sem_take(sem,...)            sem_wait(sem)
#define rt_sem_release(sem)             sem_post(sem)

typedef b_mq_t                          rt_mq_t;
#define rt_mq_create(name,sz,msgs,f)    b_mq_create(sz, msgs)
#define rt_mq_delete(mq)                b_mq_delete(mq)
#define rt_mq_send(mq, data, sz)        b_mq_send(mq, (void *)data, sz)
#define rt_mq_recv(mq, data, sz, t)     b_mq_recv(mq, (void *)data, sz, t)
#define rt_mq_reset(mq, ...)            b_mq_reset(mq)

#define rt_enter_critical()             das_os_lock()
#define rt_exit_critical()              das_os_unlock()

rt_err_t rt_thread_init(struct rt_thread *thread,
                        const char       *name,
                        void (*entry)(void *parameter),
                        void             *parameter,
                        void             *stack_start,
                        rt_uint32_t       stack_size,
                        rt_uint8_t        priority,
                        rt_uint32_t       tick);
rt_thread_t rt_thread_create(const char *name,
                             void (*entry)(void *parameter),
                             void       *parameter,
                             rt_uint32_t stack_size,
                             rt_uint8_t  priority,
                             rt_uint32_t tick);
rt_thread_t rt_thread_self(void);
rt_err_t rt_thread_startup(rt_thread_t thread);
rt_err_t rt_thread_delete(rt_thread_t thread);
rt_bool_t rt_sd_in(void);

void rt_timer_init(rt_timer_t  timer,
                   const char *name,
                   void (*timeout)(void *parameter),
                   void       *parameter,
                   rt_time_t   time,
                   rt_uint8_t  flag);
rt_timer_t rt_timer_create(const char *name,
                           void (*timeout)(void *parameter),
                           void       *parameter,
                           rt_time_t   time,
                           rt_uint8_t  flag);
rt_err_t rt_timer_delete(rt_timer_t timer);
rt_err_t rt_timer_start(rt_timer_t timer);
rt_err_t rt_timer_stop(rt_timer_t timer);
rt_err_t rt_timer_control(rt_timer_t timer, rt_uint8_t cmd, void *arg);
rt_timer_t rt_timer_self(void);
int das_serial_select(int fd, int usec);
void das_printf_buffer(const uint8_t *buffer, int nb);

#endif

