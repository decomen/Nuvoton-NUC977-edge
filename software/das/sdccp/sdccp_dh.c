
#include "board.h"

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "sdccp_net.h"
#include "net_helper.h"
#include "sdccp_dh.h"

#include "sdccp_dh_ini.cc"
const char *_dh_default_ini = def_dh_default_ini;

const char *__method_list[] = {"GET", "POST"};

void dh_try_create_default_config_file(const char *path)
{
    if (das_string_startwith(path, DH_INI_CFG_PATH_PREFIX, 1)) {
        if (das_get_file_length(path) < 20) {
            das_write_text_file(path, _dh_default_ini, strlen(_dh_default_ini));
        }
    }
}

static dh_cfg_t             *s_dh_cfgs[BOARD_TCPIP_MAX];

static void __dh_default_cfg(int index)
{
    memset(s_dh_cfgs[index], 0, sizeof(dh_cfg_t));
	s_dh_cfgs[index]->api = das_strdup(NULL, "/omdev/smdat0.asp");
	s_dh_cfgs[index]->method = 1;
}

static void __dh_cfg_free(int index)
{
    if (s_dh_cfgs[index]) {
        RT_KERNEL_FREE(s_dh_cfgs[index]->api);
        RT_KERNEL_FREE(s_dh_cfgs[index]);
    }
}

void dh_global_init(void)
{
    static int _init_flag = 0;
    
    if (0 == _init_flag) {
        _init_flag = 1;
    }
}

rt_bool_t dh_open(rt_uint8_t index)
{
    dh_close(index);

    __dh_cfg_free(index);
    s_dh_cfgs[index] = RT_KERNEL_CALLOC(sizeof(dh_cfg_t));

    s_cc_reinit_flag[index][0] = RT_FALSE;
    __dh_default_cfg(index);
    {
        char buf[64] = "";
        ini_t *ini;
        sprintf(buf, DH_INI_CFG_PATH_PREFIX "%d" ".ini", index);
        ini = ini_load(buf);

        if (ini) {
            const char *str = ini_getstring(ini, "common", "api", "/omdev/smdat0.asp");
            if (str && str[0]) s_dh_cfgs[index]->api = das_strdup(NULL, str);
            
            s_dh_cfgs[index]->method = ini_getint(ini, "common", "method", 1);

            rt_kprintf("dh[%d], api: %s, method: %s\n", index, s_dh_cfgs[index]->api, __method_list[s_dh_cfgs[index]->method]);

            ini_free(ini);
        } else {
            rt_kprintf(" %s load failed\r\n", buf);
        }
    }
    return RT_TRUE;
}

void dh_close(rt_uint8_t index)
{
    
}

cJSON *dh_get_rsp(rt_uint8_t index, int timeout, uint32_t sid, uint32_t itype)
{
    cJSON *rsp = NULL;
    dh_cfg_t *dh_cfg = s_dh_cfgs[index];
    tcpip_cfg_t *tcpip_cfg = &g_tcpip_cfgs[index];
    ft_http_client_t* http = ft_http_new();

    printf("index = %d\n", index);
    if (http && dh_cfg) {
        char url[128] = {0};
        char post_data[64] = {0};
        const char *body = NULL;
        if (tcpip_cfg->port == 80) {
            if (dh_cfg->api[0] == '/') {
                snprintf(url, sizeof(url) - 1, "%s%s", tcpip_cfg->peer, dh_cfg->api);
            } else {
                snprintf(url, sizeof(url) - 1, "%s/%s", tcpip_cfg->peer, dh_cfg->api);
            }
        } else {
            if (dh_cfg->api[0] == '/') {
                snprintf(url, sizeof(url) - 1, "%s:%u%s", tcpip_cfg->peer, tcpip_cfg->port, dh_cfg->api);
            } else {
                snprintf(url, sizeof(url) - 1, "%s:%u/%s", tcpip_cfg->peer, tcpip_cfg->port, dh_cfg->api);
            }
        }
        das_trim_all(url);
        printf("url = %s\n", url);
        snprintf(post_data, sizeof(post_data) - 1, "itype=%u&sid=%u", itype, sid);
        printf("post_data = %s\n", post_data);
        ft_http_set_timeout(http, timeout);
        body = ft_http_sync_request(http, url, dh_cfg->method, post_data, strlen(post_data));
        printf("http.error_code = %d\n", ft_http_get_error_code(http));
        if (body) {
            printf("body = %s\n", body);
            rsp = cJSON_Parse(body);
        }
    }

    if (http) ft_http_destroy(http);

    return rsp;
}

bool dh_is_rsp_error(cJSON *root)
{
    return dh_get_rsp_reccnt(root) <= 0;
}

int dh_get_rsp_reccnt(cJSON *root)
{
    int reccnt = 0;
    if (root) {
        reccnt = cJSON_GetInt(root, "reccnt", 0);
    }
    return reccnt;
}

double dh_get_rsp_value(cJSON *root, const int sid, const int itype, const char *key)
{
    if (root) {
        cJSON *recs = cJSON_GetObjectItem(root, "recs");
        int cnt = cJSON_GetArraySize(recs);
        if (cnt == 1) {
            cJSON *item = cJSON_GetArrayItem(recs, 0);
            if (item) {
                if (itype == -1 || itype == cJSON_GetInt(item, "Type", -1)) {
                    return cJSON_GetDouble(item, key, 0.0/0.0);
                }
            }
        } else if (cnt > 1) {
            for (int idx = 0; idx < cnt; idx++) {
                cJSON *item = cJSON_GetArrayItem(recs, idx);
                bool sid_match = true, itype_match = true;
                if (sid != -1 && sid != cJSON_GetInt(item, "SID", -1)) {
                    sid_match = false;
                }
                if (itype != -1 && itype != cJSON_GetInt(item, "Type", -1)) {
                    itype_match = false;
                }
                if (sid_match && itype_match) {
                    return cJSON_GetDouble(item, key, 0.0/0.0);
                }
            }
        }
    }

    return 0.0/0.0;
}

void dh_free_rsp(cJSON *root)
{
    if (root) cJSON_Delete(root);
}

