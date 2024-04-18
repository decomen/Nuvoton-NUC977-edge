#ifndef __BLOCK_MSG_QUEUE_H__
#define __BLOCK_MSG_QUEUE_H__

#include <pthread.h>

struct block_msg_queue {
    int             max_msgs;
    int             msg_size;
    void            **msg_pool;
    int             head, tail;
    pthread_mutex_t     mutex;
    pthread_condattr_t  c_attr;
    pthread_cond_t      c_full;
    pthread_cond_t      c_empty;
};

typedef struct block_msg_queue  *b_mq_t;

#define B_MQ_ERR_NULL       -1
#define B_MQ_ERR_OK         0
#define B_MQ_ERR_OTHER      1
#define B_MQ_ERR_MEM        2
#define B_MQ_ERR_TIMEOUT    3
#define B_MQ_ERR_EMPTY      4
#define B_MQ_ERR_FULL       5
#define B_MQ_ERR_SIZE       6

b_mq_t b_mq_create(int msg_size, int max_msgs);
int b_mq_delete(b_mq_t mq);
int b_mq_reset(b_mq_t mq);
int b_mq_msgs(b_mq_t mq);
int b_mq_remain(b_mq_t mq);
int b_mq_send(b_mq_t mq, void *buffer, int size);
int b_mq_recv(b_mq_t mq, void *buffer, int size, int timeout);

#endif

