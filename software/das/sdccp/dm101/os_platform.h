#ifndef _NET_COMM_H_
#define _NET_COMM_H_

#define F_READ 0
#define F_WRITE 1

#include <stdint.h>

void *dm101_malloc(int size);
void dm101_free(void *ptr);

void *dm101_work_init(void *args);
void dm101_work_reset(void *args);
void dm101_work_close(void *args);
int dm101_work_read(void *args, void *buf, uint32_t len, int8_t rw_flag,uint32_t timeout);
int dm101_work_write(void *args, void *buf, uint32_t len, int8_t rw_flag);

void dm101_fill_base_info(struct Base_info *info);

uint64_t get_utc_time(void);

struct dm101_args {
    int index;
	void *fifo;
};


#endif

