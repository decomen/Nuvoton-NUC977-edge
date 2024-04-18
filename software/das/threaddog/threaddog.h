#ifndef __THREAD_DOG_H__
#define __THREAD_DOG_H__

typedef enum {
    THD_DOG_S_RUNNING,  //运行状态
    THD_DOG_S_SUSPEND,  //线程挂起
    THD_DOG_S_DIE,      //线程死亡
    THD_DOG_S_EXIT,     //线程结束
} thddog_state_t;

typedef struct {
    char        name[24];       //thread_name
    long        use         :1; //是否在使用
    long        is_over     :1; //是否溢出
    long        over_sec    :8; //初始化设置溢出时长(S)
    long        tick_reg;       //注册tick
    long        tick_feed;      //最后喂狗tick
    const char  *func;          //最后函数
    int         line;           //最后文件行
    const char  *run_desc;      //最后描述
    thddog_state_t state;       //当前状态
    const void  *data;          //附加数据
} thddog_t;     // 44 byte

void threaddog_init(void);
thddog_t *threaddog_register(const char *thread_name, int sec, const void *data);
void threaddog_unregister(thddog_t *dog);
void threaddog_update(thddog_t *dog, const char *func, const int line, const char *desc, thddog_state_t state);

#define threaddog_feed(_dog, _desc) threaddog_update(_dog, __FUNCTION__, __LINE__, _desc, THD_DOG_S_RUNNING)
#define threaddog_suspend(_dog, _desc) threaddog_update(_dog, __FUNCTION__, __LINE__, _desc, THD_DOG_S_SUSPEND)
#define threaddog_resume(_dog, _desc) threaddog_update(_dog, __FUNCTION__, __LINE__, _desc, THD_DOG_S_RUNNING)
#define threaddog_exit(_dog, _desc) threaddog_update(_dog, __FUNCTION__, __LINE__, _desc, THD_DOG_S_EXIT)

const void *threaddog_check(void);

#endif

