#ifndef __B_FIFO_H__
#define __B_FIFO_H__

/**
 timeout : usec
*/

#define BFIFO_MAX_FD        (1024)

// 原型
struct bfifo {
    unsigned int    in;         /** in位置 */
    unsigned int    out;        /** out位置 */
    unsigned int    size;
    void            *lock;
    void            *c_not_empty;
    void            *c_not_full;
    unsigned char   buffer[0];  /** buffer */
};

typedef struct bfifo * bfifo_t;

int bfifo_init(bfifo_t f, int size);
bfifo_t bfifo_create(int size);
void bfifo_destroy(bfifo_t f);
int bfifo_reset(bfifo_t f);
int bfifo_push_wait(bfifo_t f, long timeout);
int bfifo_push(bfifo_t f, const unsigned char *buffer, unsigned int len, long timeout);
int bfifo_pull_wait(bfifo_t f, long timeout);
int bfifo_pull(bfifo_t f, unsigned char *buffer, unsigned int len, long timeout);
int bfifo_peek(bfifo_t f, unsigned char *buffer, unsigned int len);
int bfifo_count(bfifo_t f);

/** simulation file descriptor*/
int bfifo_open(int size, int flag);
int bfifo_close(int fd);

int bfifo_in_select(int fd, struct timeval *tv);
int bfifo_out_select(int fd, struct timeval *tv);

/** 读写数据 */
int bfifo_read(int fd, void *buf, int count);
int bfifo_write(int fd, const void *buf, int count);

/** 填充,取出数据 */
int bfifo_push_in(int fd, const void *buf, int count);
int bfifo_pull_out(int fd, void *buf, int count);

int bfifo_reset_in(int fd);
int bfifo_reset_out(int fd);

bfifo_t bfifo_get_in_fifo(int fd);
bfifo_t bfifo_get_out_fifo(int fd);

#endif


