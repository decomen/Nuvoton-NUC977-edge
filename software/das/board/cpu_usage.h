
#ifndef _CPU_USAGE_H__
#define _CPU_USAGE_H__

typedef struct {
    rt_uint32_t cpu;
    rt_uint32_t mem_all;
    rt_uint32_t mem_max_use;
    rt_uint32_t mem_now_use;
    rt_size_t flash_size;
    rt_size_t flash_use;
    rt_size_t sd_size;
    rt_size_t sd_use;
} cpu_usage_t;

extern cpu_usage_t g_xCpuUsage;

void cpu_flash_usage_refresh(void);

#endif


