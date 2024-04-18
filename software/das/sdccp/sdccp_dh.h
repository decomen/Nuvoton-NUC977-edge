
#ifndef __HJT212_H__
#define __HJT212_H__

#include "varmanage.h"
#include "das_json.h"

#define DH_INI_CFG_PATH_PREFIX         BOARD_CFG_PATH"rtu_dh_"

#define DH_BUF_SIZE         (4096)
#define DH_INBUF_SIZE       (2048)
#define DH_PARSE_STACK      (2048)      //解析任务内存占用

typedef struct {
	char            *api;           //  /omdev/smdat0.asp
    int             method;        // 0:GET 1:POST
} dh_cfg_t;

void dh_global_init(void);

rt_bool_t dh_open(rt_uint8_t index);
void dh_close(rt_uint8_t index);
cJSON *dh_get_rsp(rt_uint8_t index, int timeout, uint32_t sid, uint32_t itype);
bool dh_is_rsp_error(cJSON *root);
int dh_get_rsp_reccnt(cJSON *root);
double dh_get_rsp_value(cJSON *root, const int sid, const int itype, const char *key);
void dh_free_rsp(cJSON *root);

void dh_try_create_default_config_file(const char *path);

#endif

