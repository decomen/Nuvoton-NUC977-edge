#include "board.h"
#include "dm_lib.h"
#include "os_platform.h"
#include "sdccp_net.h"
#include "net_helper.h"
#include "dm101.h"
#include "bfifo.h"

void *dm101_malloc(int size)
{
    return rt_malloc(size);
}

void dm101_free(void *ptr)
{
    if (ptr) rt_free(ptr);
}

void *dm101_work_init(void *args)
{
    return args;
}

void dm101_work_close(void *args)
{
    return ;
}

void dm101_work_reset(void *args)
{
    if (args) {
        struct bfifo *fifo = (struct bfifo *)((struct dm101_args *)args)->fifo;
        if (fifo)
            bfifo_reset(fifo);
    }
}

int dm101_work_read(void *args, void *buf, uint32_t len, int8_t rw_flag, uint32_t timeout)
{
    if (args) {
        struct bfifo *fifo = (struct bfifo *)((struct dm101_args *)args)->fifo;
        int num = 0;
        if (fifo) {
            num = bfifo_pull(fifo, (unsigned char *)buf, (unsigned int)len, timeout);
        }
        return num;
    }
    return 0;
}

int dm101_work_write(void *args, void *buf, uint32_t len, int8_t rw_flag)
{
   // printf("dm101_work_write:%d\n",len);

    if (args) {
        int index = ((struct dm101_args *)args)->index;
        int num = 0;
        if (index >= 0) {
            num = net_send(index, 0, (const rt_uint8_t *)buf, (rt_uint16_t)len);
        }
        return num;
    }
    return 0;
}

uint64_t get_utc_time(void)
{
    time_t timep;
    time(&timep);
    return timep;
}

void dm101_fill_base_info(struct Base_info *info)
{
    strcpy(info->type, PRODUCT_MODEL);
    sprintf(info->ver, "v%d.%02d", SW_VER_MAJOR, SW_VER_MINOR);
    memcpy(info->name, g_sys_info.SN, sizeof(g_sys_info.SN));
}

//for linux end
