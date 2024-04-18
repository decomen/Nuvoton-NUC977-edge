
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "block_msg_queue.h"

#define B_MQ_ALLOC(_n)          calloc(_n, 1)
#define B_MQ_FREE(_mem)         if((_mem)) {free((_mem));(_mem)=NULL;}

extern int pthread_condattr_setclock(pthread_condattr_t *attr,clockid_t clock_id);

b_mq_t b_mq_create(int msg_size, int max_msgs)
{
    if (max_msgs > 0) {
        b_mq_t mq = B_MQ_ALLOC(sizeof(struct block_msg_queue));
        if (mq) {
            int i = 0;
            mq->msg_size = msg_size;
            mq->max_msgs = max_msgs + 1;
            mq->head = 0; mq->tail = 0;
            mq->msg_pool = B_MQ_ALLOC(sizeof(void *)*(mq->max_msgs));
            for (i = 0; i < mq->max_msgs; i++) {
                mq->msg_pool[i] = B_MQ_ALLOC(mq->msg_size);
            }
            pthread_mutex_init(&mq->mutex, NULL);
            pthread_condattr_init(&mq->c_attr);
            pthread_condattr_setclock(&mq->c_attr, CLOCK_MONOTONIC);
            pthread_cond_init(&mq->c_empty, &mq->c_attr);
            pthread_cond_init(&mq->c_full, &mq->c_attr);
        }
        return mq;
    }

    return NULL;
}

int b_mq_delete(b_mq_t mq)
{
    if (mq) {
        int i = 0;
        for (i = 0; i < mq->max_msgs; i++) {
            B_MQ_FREE(mq->msg_pool[i]);
        }
        B_MQ_FREE(mq->msg_pool);
        pthread_mutex_destroy(&mq->mutex);  
        pthread_cond_destroy(&mq->c_full);  
        pthread_cond_destroy(&mq->c_empty);
        pthread_condattr_destroy(&mq->c_attr);
        B_MQ_FREE(mq);
    }
    return B_MQ_ERR_OK;
}

int b_mq_reset(b_mq_t mq)
{
    if (mq) {
        pthread_mutex_lock(&mq->mutex);
        mq->head = 0; mq->tail = 0;
        pthread_cond_signal(&mq->c_full);
        pthread_mutex_unlock(&mq->mutex);
    }
    return B_MQ_ERR_OK;
}

int b_mq_msgs(b_mq_t mq)
{
    int msgs = 0;
    if (mq) {
        pthread_mutex_lock(&mq->mutex);
        msgs = ((mq->tail - mq->head + mq->max_msgs) % mq->max_msgs);
        pthread_mutex_unlock(&mq->mutex);
    }
    return msgs;
}

int b_mq_remain(b_mq_t mq)
{
    int remian = 0;
    if (mq) {
        pthread_mutex_lock(&mq->mutex);
        remian = mq->max_msgs - 1 - ((mq->tail - mq->head + mq->max_msgs) % mq->max_msgs);
        pthread_mutex_unlock(&mq->mutex);
    }
    return remian;
}

int b_mq_send(b_mq_t mq, void *buffer, int size)
{
    int ret = B_MQ_ERR_NULL;
    
    if (mq) {
        pthread_mutex_lock(&mq->mutex);
        if (size <= mq->msg_size) {
            if((mq->tail + 1) % mq->max_msgs == mq->head) {
                ret = B_MQ_ERR_FULL;
            } else {
                memset(mq->msg_pool[mq->tail], 0, mq->msg_size);
                memcpy(mq->msg_pool[mq->tail], buffer, size);
                mq->tail = (mq->tail + 1) % mq->max_msgs;
                pthread_cond_signal(&mq->c_empty);
                ret = B_MQ_ERR_OK;
            }
        } else {
            ret = B_MQ_ERR_SIZE;
        }
        pthread_mutex_unlock(&mq->mutex);
    }

    return ret;
}

int b_mq_recv(b_mq_t mq, void *buffer, int size, int timeout)
{
    int ret = B_MQ_ERR_NULL;
    
    if (mq) {
        pthread_mutex_lock(&mq->mutex);
        if (size <= mq->msg_size) {
            if (mq->head == mq->tail) {
                if (0 == timeout) {
                    ret = B_MQ_ERR_TIMEOUT;
                } else if (timeout > 0) {
                    struct timespec _abstime;
                    struct timespec now;
                    clock_gettime(CLOCK_MONOTONIC, &now);
                    int nsec = now.tv_nsec + (timeout % 1000) * 1000000;
                    _abstime.tv_nsec = nsec % 1000000000;
                    _abstime.tv_sec = now.tv_sec + nsec / 1000000000 + timeout / 1000;
                    if (0 == pthread_cond_timedwait(&mq->c_empty, &mq->mutex, &_abstime)) {
                        ret = B_MQ_ERR_OK;
                    } else {
                        ret = B_MQ_ERR_TIMEOUT;
                    }
                } else {
                    if (0 == pthread_cond_wait(&mq->c_empty, &mq->mutex)) {
                        ret = B_MQ_ERR_OK;
                    } else {
                        ret = B_MQ_ERR_OTHER;
                    }
                }
            } else {
                ret = B_MQ_ERR_OK;
            }
            
            if (B_MQ_ERR_OK == ret) {
                memcpy(buffer, mq->msg_pool[mq->head], size);
                mq->head = (mq->head + 1) % mq->max_msgs;
                pthread_cond_signal(&mq->c_full);
            }
        } else {
            ret = B_MQ_ERR_SIZE;
        }
        pthread_mutex_unlock(&mq->mutex);
    }

    return ret;
}

