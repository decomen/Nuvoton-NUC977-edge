
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

#include "bfifo.h"

/* easy min max */
#define _fifo_min(_x,_y)   ((_x)<(_y)?(_x):(_y))
#define _fifo_max(_x,_y)   ((_x)<(_y)?(_y):(_x))

static void *__fifo_malloc(int size)
{
    return malloc(size);
}

static void __fifo_free(void *p)
{
    if (p) free(p);
}

static void *__fifo_lock_create(void)
{
    pthread_mutex_t *m = __fifo_malloc(sizeof(*m));
    if (m) pthread_mutex_init(m, NULL);
    return m;
}

static int __fifo_lock(void *lock)
{
    return (lock && 0 == pthread_mutex_lock((pthread_mutex_t *)lock));
}

static int __fifo_unlock(void *lock)
{
    return (lock && 0 == pthread_mutex_unlock((pthread_mutex_t *)lock));
}

static void __fifo_lock_destroy(void *lock)
{
    if (lock) {
        pthread_mutex_destroy((pthread_mutex_t *)lock);
        __fifo_free(lock);
    }
}

int pthread_condattr_setclock(pthread_condattr_t *attr, clockid_t clock_id); 

static void *__fifo_cond_create(void)
{
    pthread_cond_t *c = __fifo_malloc(sizeof(*c));
    if (c) {
        pthread_condattr_t attr;
        pthread_condattr_init(&attr);
        pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
        pthread_cond_init(c, &attr);
        pthread_condattr_destroy(&attr);
    }
    return c;
}

static int __fifo_cond_timedwait(void *lock, void *cond, long timeout)
{
    if (lock && cond) {
        if (timeout > 0) {
            struct timespec _abstime;
            struct timespec _now;
            clock_gettime(CLOCK_MONOTONIC, &_now);
            long nsec = _now.tv_nsec + (timeout % 1000000) * 1000;
            _abstime.tv_nsec = nsec % 1000000000;
            _abstime.tv_sec = _now.tv_sec + nsec / 1000000000 + timeout / 1000000;
            return 0 == pthread_cond_timedwait((pthread_cond_t *)cond, (pthread_mutex_t *)lock, &_abstime);
        } else {
            return 0 == pthread_cond_wait((pthread_cond_t *)cond, (pthread_mutex_t *)lock);
        }
    }
    return 0;
}

/*
static int __fifo_cond_wait(void *lock, void *cond)
{
    return __fifo_cond_timedwait(lock, cond, -1);
}
*/

static int __fifo_cond_signal(void *cond)
{
    return (cond && pthread_cond_signal((pthread_cond_t *)cond));
}

static void __fifo_cond_destroy(void *cond)
{
    if (cond) {
        pthread_cond_destroy((pthread_cond_t *)cond);
        __fifo_free(cond);
    }
}

int bfifo_init(bfifo_t f, int size)
{
    if (f) {
        f->in = f->out  = 0;
        f->size         = size;
        f->lock         = __fifo_lock_create();
        f->c_not_empty  = __fifo_cond_create();
        f->c_not_full   = __fifo_cond_create();
        return 0;
    }
    return -1;
}

bfifo_t bfifo_create(int size)
{
    bfifo_t f = __fifo_malloc(sizeof(struct bfifo) + size);
    if (f) {
        bfifo_init(f, size);
    }
    return f;
}

void bfifo_destroy(bfifo_t f)
{
    if (f) {
        __fifo_lock_destroy(f->lock);
        __fifo_cond_destroy(f->c_not_empty);
        __fifo_free(f);
    }
}

int bfifo_reset(bfifo_t f)
{
    if (f && __fifo_lock(f->lock)) {
        __fifo_cond_signal(f->c_not_empty);
        __fifo_cond_signal(f->c_not_full);
        f->in = f->out = 0;
        __fifo_unlock(f->lock);
        return 0;
    }
    return -1;
}

int bfifo_push_wait(bfifo_t f, long timeout)
{
    if (f && __fifo_lock(f->lock)) {
        int push_flag = 1;
        if(f->size == f->in - f->out) {
            if (timeout == 0) {
                push_flag = 0;
            } else {
                push_flag = __fifo_cond_timedwait(f->lock, f->c_not_full, timeout);
            }
        }
        __fifo_unlock(f->lock);
        return push_flag ? 1 : 0;
    }
    return -1;
}

int bfifo_push(bfifo_t f, const unsigned char *buffer, unsigned int len, long timeout)
{
    if (f && __fifo_lock(f->lock)) {
        int push_flag = 1;
        if(f->size == f->in - f->out) {
            if (timeout == 0) {
                push_flag = 0;
            } else {
                push_flag = __fifo_cond_timedwait(f->lock, f->c_not_full, timeout);
            }
        }
        if (push_flag) {
            unsigned int l;
            int pos = f->in % f->size;
            
            len = _fifo_min(len, f->size - f->in + f->out);
            l = _fifo_min(len, f->size - pos);
            
            if (l > 0) 
                memcpy(&f->buffer[pos], buffer, l);
            if (len > l) 
                memcpy(&f->buffer[0], buffer + l, len - l);
            
            f->in += len;
            if (len > 0) {
                __fifo_cond_signal(f->c_not_empty);
            }
        } else {
            len = 0;
        }
        __fifo_unlock(f->lock);
        return len;
    }
    return -1;
}

int bfifo_pull_wait(bfifo_t f, long timeout)
{
    if (f && __fifo_lock(f->lock)) {
        int pull_flag = 1;
        if (f->in == f->out) {
            if (timeout == 0) {
                pull_flag = 0;
            } else {
                pull_flag = __fifo_cond_timedwait(f->lock, f->c_not_empty, timeout);
            }
        }
        __fifo_unlock(f->lock);
        return pull_flag ? 1 : 0;
    }
    return -1;
}


int bfifo_pull(bfifo_t f, unsigned char *buffer, unsigned int len, long timeout)
{
    if (f && __fifo_lock(f->lock)) {
        int pull_flag = 1;
        if (f->in == f->out) {
            if (timeout == 0) {
                pull_flag = 0;
            } else {
                pull_flag = __fifo_cond_timedwait(f->lock, f->c_not_empty, timeout);
            }
        }
        if (pull_flag) {
            unsigned int l;
            int pos = f->out % f->size;
            
            len = _fifo_min(len, f->in - f->out);
            l = _fifo_min(len, f->size - pos);
            
            if (l > 0) 
                memcpy(buffer, &f->buffer[pos], l);
            if (len > l) 
                memcpy(buffer + l, &f->buffer[0], len - l);
            
            f->out += len;
            if (len > 0) {
                __fifo_cond_signal(f->c_not_full);
            }
        } else {
            len = 0;
        }
        __fifo_unlock(f->lock);
        return len;
    }
    return -1;
}

int bfifo_peek(bfifo_t f, unsigned char *buffer, unsigned int len)
{
    if (f && __fifo_lock(f->lock)) {
        if (f->in == f->out) {
            len = 0;
        } else {
            unsigned int l;
            int pos = f->out % f->size;
            
            len = _fifo_min(len, f->in - f->out);
            l = _fifo_min(len, f->size - pos);
            
            if (l > 0) 
                memcpy(buffer, &f->buffer[pos], l);
            if (len > l) 
                memcpy(buffer + l, &f->buffer[0], len - l);
            
            __fifo_unlock(f->lock);
            return len;
        }
    }
    return 0;
}

int bfifo_count(bfifo_t f)
{
    if (f && __fifo_lock(f->lock)) {
        unsigned int len = f->in - f->out;
        __fifo_unlock(f->lock);
        return len;
    }
    return -1;
}

/** simulation file descriptor : none block*/

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

struct _bfifo_node {
    bfifo_t in;
    bfifo_t out;
    int flag;
};

typedef struct _bfifo_node * _bfifo_node_t;

static pthread_mutex_t s_bfifos_lock = PTHREAD_MUTEX_INITIALIZER;
static _bfifo_node_t s_bfifos[BFIFO_MAX_FD];

static int __get_free_bfifo_node(void)
{
    int i;
    for (i = 0; i < BFIFO_MAX_FD; i++) {
        if (s_bfifos[i] == NULL) 
            return i;
    }
    return -1;
}

static _bfifo_node_t __get_bfifo_node(int fd)
{
    return (fd >= 0 && fd < BFIFO_MAX_FD) ? s_bfifos[fd] : NULL;
}

static void __free_bfifo_node(int fd)
{
    if (fd >= 0 && fd < BFIFO_MAX_FD) {
        __fifo_free(s_bfifos[fd]);
        s_bfifos[fd] = NULL;
    }
}

bfifo_t bfifo_get_in_fifo(int fd)
{
    bfifo_t f = NULL;
    pthread_mutex_lock(&s_bfifos_lock);
    {
        int fd = __get_free_bfifo_node();
        if (fd >= 0) {
            _bfifo_node_t bf = __get_bfifo_node(fd);
            if (bf) f = bf->in;
        }
    }
    pthread_mutex_unlock(&s_bfifos_lock);

    return f;
}

bfifo_t bfifo_get_out_fifo(int fd)
{
    bfifo_t f = NULL;
    pthread_mutex_lock(&s_bfifos_lock);
    {
        int fd = __get_free_bfifo_node();
        if (fd >= 0) {
            _bfifo_node_t bf = __get_bfifo_node(fd);
            if (bf) f = bf->out;
        }
    }
    pthread_mutex_unlock(&s_bfifos_lock);

    return f;
}

int bfifo_open(int size, int flag)
{
    int fd = 0;
    pthread_mutex_lock(&s_bfifos_lock);
    fd = __get_free_bfifo_node();
    if (fd >= 0) {
        _bfifo_node_t bf = (_bfifo_node_t)__fifo_malloc(sizeof(*bf));
        if (bf) {
            bf->in = bfifo_create(size);
            bf->out = bfifo_create(size);
            bf->flag = flag;
            if (bf->in && bf->out) {
                s_bfifos[fd] = bf;
            } else {
                bfifo_destroy(bf->in);
                bfifo_destroy(bf->out);
                __fifo_free(bf);
                fd = -1;
            }
        } else {
            fd = -1;
        }
    }
    pthread_mutex_unlock(&s_bfifos_lock);

    return fd;
}

int bfifo_in_select(int fd, struct timeval *tv)
{
    int rc = -1;
    
    pthread_mutex_lock(&s_bfifos_lock);
    {
        _bfifo_node_t bf = __get_bfifo_node(fd);
        if (bf) {
            rc = bfifo_push_wait(bf->in, tv ? tv->tv_usec + tv->tv_sec * 1000000 : -1);
            if (rc == 0) {
                errno = ETIMEDOUT;
            }
        } else {
            errno = EBADF;
        }
    }
    pthread_mutex_unlock(&s_bfifos_lock);

    return rc;
}

int bfifo_out_select(int fd, struct timeval *tv)
{
    int rc = -1;
    
    pthread_mutex_lock(&s_bfifos_lock);
    {
        _bfifo_node_t bf = __get_bfifo_node(fd);
        if (bf) {
            rc = bfifo_push_wait(bf->out, tv ? tv->tv_usec + tv->tv_sec * 1000000 : -1);
            if (rc == 0) {
                errno = ETIMEDOUT;
            }
        } else {
            errno = EBADF;
        }
    }
    pthread_mutex_unlock(&s_bfifos_lock);

    return rc;
}

int bfifo_read(int fd, void *buf, int count)
{
    int rc = -1;
    
    pthread_mutex_lock(&s_bfifos_lock);
    {
        _bfifo_node_t bf = __get_bfifo_node(fd);
        if (bf) {
            rc = bfifo_pull(bf->in, (unsigned char *)buf, count, (bf->flag | O_NONBLOCK) ? 0 : -1);
        } else {
            errno = EBADF;
        }
    }
    pthread_mutex_unlock(&s_bfifos_lock);

    return rc;
}

int bfifo_push_in(int fd, const void *buf, int count)
{
    int rc = -1;
    
    pthread_mutex_lock(&s_bfifos_lock);
    {
        _bfifo_node_t bf = __get_bfifo_node(fd);
        if (bf) {
            rc = bfifo_push(bf->in, (const unsigned char *)buf, count, (bf->flag | O_NONBLOCK) ? 0 : -1);
        } else {
            errno = EBADF;
        }
    }
    pthread_mutex_unlock(&s_bfifos_lock);

    return rc;
}

int bfifo_reset_in(int fd)
{
    int rc = -1;
    
    pthread_mutex_lock(&s_bfifos_lock);
    {
        _bfifo_node_t bf = __get_bfifo_node(fd);
        if (bf) {
            rc = bfifo_reset(bf->in);
        } else {
            errno = EBADF;
        }
    }
    pthread_mutex_unlock(&s_bfifos_lock);

    return rc;
}

int bfifo_write(int fd, const void *buf, int count)
{
    int rc = -1;
    
    pthread_mutex_lock(&s_bfifos_lock);
    {
        _bfifo_node_t bf = __get_bfifo_node(fd);
        if (bf) {
            rc = bfifo_push(bf->out, (const unsigned char *)buf, count, (bf->flag | O_NONBLOCK) ? 0 : -1);
        } else {
            errno = EBADF;
        }
    }
    pthread_mutex_unlock(&s_bfifos_lock);

    return rc;
}

int bfifo_pull_out(int fd, void *buf, int count)
{
    int rc = -1;
    
    pthread_mutex_lock(&s_bfifos_lock);
    {
        _bfifo_node_t bf = __get_bfifo_node(fd);
        if (bf) {
            rc = bfifo_pull(bf->out, (unsigned char *)buf, count, (bf->flag | O_NONBLOCK) ? 0 : -1);
        } else {
            errno = EBADF;
        }
    }
    pthread_mutex_unlock(&s_bfifos_lock);

    return rc;
}

int bfifo_reset_out(int fd)
{
    int rc = -1;
    
    pthread_mutex_lock(&s_bfifos_lock);
    {
        _bfifo_node_t bf = __get_bfifo_node(fd);
        if (bf) {
            rc = bfifo_reset(bf->out);
        } else {
            errno = EBADF;
        }
    }
    pthread_mutex_unlock(&s_bfifos_lock);

    return rc;
}

int bfifo_close(int fd)
{
    int rc = -1;
    
    pthread_mutex_lock(&s_bfifos_lock);
    {
        _bfifo_node_t bf = __get_bfifo_node(fd);
        if (bf) {
            bfifo_destroy(bf->in);
            bfifo_destroy(bf->out);
            __free_bfifo_node(fd);
            rc = 0;
        } else {
            errno = EBADF;
        }
    }
    pthread_mutex_unlock(&s_bfifos_lock);

    return rc;
}

