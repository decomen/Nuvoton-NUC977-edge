
#include "board.h"
#include "das_os.h"
#include <sys/syscall.h>
#include <signal.h>

static pthread_mutex_t g_os_mutex;
static int g_os_init = 0;
static pthread_key_t g_th_key;

extern int pthread_condattr_setclock(pthread_condattr_t *attr,clockid_t clock_id);

void das_os_init(void)
{
    static struct rt_thread os_thread;
    pthread_key_create(&g_th_key, NULL);
    das_strcpy_s(os_thread.name, "os_main");
    pthread_setspecific(g_th_key, &os_thread);
    rt_thddog_register(&os_thread, 60);
    pthread_init_mutex(&g_os_mutex);

    {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = SIG_IGN;
        sigaction(SIGPIPE, &sa, 0);
    }
    
    g_os_init = 1;
}

void das_os_lock(void)
{
    if (g_os_init)
        pthread_mutex_lock(&g_os_mutex);
}

void das_os_unlock(void)
{
    if (g_os_init)
        pthread_mutex_unlock(&g_os_mutex);
}

static void *__rt_thread_worker(void *parameter)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
    pthread_detach(pthread_self());
    {
        rt_thread_t thread = (rt_thread_t)parameter;
        pthread_setspecific(g_th_key, parameter);
        //printf("__rt_thread_worker name : %s, pid : %d\n", thread->name, syscall(SYS_getpid));
        sem_post(&thread->start_sem);
        sem_wait(&thread->run_sem);
        if (thread && thread->entry) thread->entry(thread->parameter);
        //sem_destroy(&thread->run_sem);
        //free(thread);
    }
    pthread_exit(NULL);
    return NULL;
}

rt_err_t rt_thread_init(struct rt_thread *thread,
                        const char       *name,
                        void (*entry)(void *parameter),
                        void             *parameter,
                        void             *stack_start,
                        rt_uint32_t       stack_size,
                        rt_uint8_t        priority,
                        rt_uint32_t       tick)
{
    if (thread) {
        das_strcpy_s(thread->name, name);
        thread->entry = entry;
        thread->parameter = parameter;
        sem_init(&thread->run_sem, 0, 0);
        sem_init(&thread->start_sem, 0, 0);
        if (pthread_create(&thread->tid, NULL, __rt_thread_worker, thread) != -1) {
            sem_wait(&thread->start_sem);
            sem_destroy(&thread->start_sem);
            return -RT_EOK;
        }
    }
    return -RT_ERROR;
}

rt_thread_t rt_thread_create(const char *name,
                             void (*entry)(void *parameter),
                             void       *parameter,
                             rt_uint32_t stack_size,
                             rt_uint8_t  priority,
                             rt_uint32_t tick)
{
    struct rt_thread *thread = malloc(sizeof(*thread));
    if (thread) {
        memset(thread, 0, sizeof(*thread));
        rt_thread_init(thread, name, entry, parameter, NULL, stack_size, priority, tick);
    }
    return thread;
}

rt_thread_t rt_thread_self(void)
{
    return (rt_thread_t)pthread_getspecific(g_th_key);
}

rt_err_t rt_thread_startup(rt_thread_t thread)
{
    if (thread) {
        sem_post(&thread->run_sem);
        return -RT_EOK;
    }
    return -RT_ERROR;
}

rt_err_t rt_thread_cancel(rt_thread_t thread)
{
    if (thread) {
        sem_destroy(&thread->run_sem);
        pthread_cancel(thread->tid);
        pthread_join(thread->tid, NULL);
        return -RT_EOK;
    }
    return -RT_ERROR;
}

rt_err_t rt_thread_delete(rt_thread_t thread)
{
    if (thread) {
        sem_destroy(&thread->run_sem);
        pthread_cancel(thread->tid);
        pthread_join(thread->tid, NULL);
        free(thread);
        return -RT_EOK;
    }
    return -RT_ERROR;
}

rt_bool_t rt_sd_in(void)
{
    return (access("/dev/mmcblk0p1",F_OK) !=-1);
}

static void *__rt_timer_worker(void *parameter)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
    pthread_detach(pthread_self());
    {
        rt_timer_t timer = (rt_timer_t)parameter;
        pthread_setspecific(g_th_key, parameter);
        while (1) {
            sem_wait(&timer->run_sem);
            while (1) {
                pthread_mutex_lock(&timer->mutex);
                {
                    struct timespec _abstime;
                    struct timespec _now;
                    clock_gettime(CLOCK_MONOTONIC, &_now);
                    long nsec = _now.tv_nsec + (timer->timeout % 1000000) * 1000;
                    _abstime.tv_nsec = nsec % 1000000000;
                    _abstime.tv_sec = _now.tv_sec + nsec / 1000000000 + timer->timeout / 1000000;
                    if (0 == pthread_cond_timedwait(&timer->c_ctrl, &timer->mutex, &_abstime)) {
                        if (!(timer->flag & RT_TIMER_FLAG_ACTIVATED)) {
                            pthread_mutex_unlock(&timer->mutex);
                            break;
                        }
                    } else {
                        if (timer && timer->timeout_func) timer->timeout_func(timer->parameter);
                        if (!(timer->flag & RT_TIMER_FLAG_PERIODIC)) {
                            timer->flag &= ~RT_TIMER_FLAG_ACTIVATED;
                            pthread_mutex_unlock(&timer->mutex);
                            break;
                        }
                    }
                    pthread_mutex_unlock(&timer->mutex);
                }
            }
        }
        //free(thread);
    }
    pthread_exit(NULL);
    return NULL;
}

void rt_timer_init(rt_timer_t  timer,
                   const char *name,
                   void (*timeout)(void *parameter),
                   void       *parameter,
                   rt_time_t   time,
                   rt_uint8_t  flag)
{
    if (timer) {
        das_strcpy_s(timer->name, name);
        timer->timeout_func = timeout;
        timer->parameter    = parameter;
        timer->timeout      = time;
        timer->flag         = flag;
        timer->flag &= ~RT_TIMER_FLAG_ACTIVATED;
        sem_init(&timer->run_sem, 0, 0);
        pthread_mutex_init(&timer->mutex, NULL);
        {
            pthread_condattr_t  attr;
            pthread_condattr_init(&attr);
            pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
            pthread_cond_init(&timer->c_ctrl, &attr);
            pthread_condattr_destroy(&attr);
        }
        pthread_create(&timer->tid, NULL, __rt_timer_worker, timer);
    }
}
                   
rt_timer_t rt_timer_create(const char *name,
                           void (*timeout)(void *parameter),
                           void       *parameter,
                           rt_time_t   time,
                           rt_uint8_t  flag)
{
    struct rt_timer *timer = malloc(sizeof(*timer));
    if (timer) {
        memset(timer, 0, sizeof(*timer));
        rt_timer_init(timer, name, timeout, parameter, time, flag);
    }
    return timer;
}

rt_err_t rt_timer_delete(rt_timer_t timer)
{
    if (timer) {
        sem_destroy(&timer->run_sem);
        if (timer->tid != 0) {
            pthread_cancel(timer->tid);
            pthread_join(timer->tid, NULL);
        }
        free(timer);
        return -RT_EOK;
    }
    return -RT_ERROR;
}

rt_err_t rt_timer_start(rt_timer_t timer)
{
    if (timer) {
        pthread_mutex_lock(&timer->mutex);
        timer->flag |= RT_TIMER_FLAG_ACTIVATED;
        pthread_cond_signal(&timer->c_ctrl);
        sem_trywait(&timer->run_sem);
        sem_post(&timer->run_sem);
        pthread_mutex_unlock(&timer->mutex);
        return -RT_EOK;
    }
    return -RT_ERROR;
}

rt_err_t rt_timer_stop(rt_timer_t timer)
{
    if (timer) {
        pthread_mutex_lock(&timer->mutex);
        timer->flag &= ~RT_TIMER_FLAG_ACTIVATED;
        pthread_cond_signal(&timer->c_ctrl);
        sem_trywait(&timer->run_sem);
        pthread_mutex_unlock(&timer->mutex);
        return -RT_EOK;
    }
    return -RT_ERROR;
}

rt_err_t rt_timer_control(rt_timer_t timer, rt_uint8_t cmd, void *arg)
{
    if (timer) {
        pthread_mutex_lock(&timer->mutex);
        switch (cmd) {
        case RT_TIMER_CTRL_SET_TIME:
            timer->timeout  = *(rt_time_t *)arg;
            break;
        case RT_TIMER_CTRL_SET_ONESHOT:
            timer->flag &= ~RT_TIMER_FLAG_PERIODIC;
            break;
        case RT_TIMER_CTRL_SET_PERIODIC:
            timer->flag |= RT_TIMER_FLAG_PERIODIC;
            break;
        }
        pthread_mutex_unlock(&timer->mutex);
        return -RT_EOK;
    }
    return -RT_ERROR;
}

rt_timer_t rt_timer_self(void)
{
    rt_timer_t timer = (rt_timer_t)pthread_getspecific(g_th_key);
    return ((timer->tid == pthread_self()) ? timer : NULL);
}

int das_serial_select(int fd, int usec)
{
    int s_rc;
    fd_set rset;
    struct timeval tv = { .tv_sec = usec / 1000000, .tv_usec = usec % 1000000 };
    FD_ZERO(&rset); FD_SET(fd, &rset);
    
    while ((s_rc = select(fd + 1, &rset, NULL, NULL, &tv)) == -1) {
        if (errno == EINTR) {
            FD_ZERO(&rset);
            FD_SET(fd, &rset);
        } else {
            return -1;
        }
    }
    return s_rc;
}

void das_printf_buffer(const uint8_t *buffer, int nb)
{
    int n;
    for (n = 0; n < nb; n++) {
        printf("%02X ", buffer[n]);
        if (n > 0 && n % 10 == 0) printf("\n");
    }
    printf("\n");
}

#if 0

void __test_timeout(void *parameter)
{
    printf("__test_timeout : %s\n", rt_timer_self()->name);
}

int main(void)
{
    das_os_init();
    
    rt_timer_t timer = rt_timer_create("test", __test_timeout, NULL, 1100000, RT_TIMER_FLAG_PERIODIC);
    rt_timer_t timer1 = rt_timer_create("test1", __test_timeout, NULL, 1100000, RT_TIMER_FLAG_PERIODIC);
    rt_timer_start(timer);
    rt_timer_start(timer1);
    sleep(5);
    int i;
    for(i = 0; i < 5; i++) {
        sleep(1);
        rt_timer_start(timer);
    }
    for(;;) {
        sleep(1);
    }
    return 0;
}

#endif

#if 0

void __test_thread(void *parameter)
{
    printf("__test_thread : %s\n", rt_thread_self()->name);
}

int main(void)
{
    das_os_init();
    
    rt_thread_t thread = rt_thread_create("test", __test_thread, NULL, 0, 0, 0);
    sleep(5);
    rt_thread_startup(thread);
    for(;;) {
        sleep(1);
    }
    return 0;
}

#endif

