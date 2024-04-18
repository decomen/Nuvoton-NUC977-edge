
#include "dm101.h"
#include "hjt212.h"
#include "cc_bjdc.h"

char *__readline(int fd, int bufsz, char *line)
{
    int cur = lseek(fd, 0, SEEK_CUR);
    int len = read(fd, line, bufsz-1);
    int index = 0;

    line[bufsz-1] = '\0';
    if( len > 0 ) {
        while(line[index++] != '\n' && index < len);
        if(index < len) {
            line[index] = '\0';
            lseek(fd, cur + index, SEEK_SET);
        }
        if(index >= 1 && line[index-1] == '\n') line[index-1] = '\0';
        if(index >= 2 && line[index-2] == '\r') line[index-2] = '\0';
        return line;
    }
    return RT_NULL;
}

char *__check_cfg_line(char *line)
{
    int len = strlen(line);
    if( line[0] == '#' && 
        line[1] == '#' && 
        line[2] == '{' && 
        line[len-3] == '}' && 
        line[len-2] == '#' && 
        line[len-1] == '#' )
     {
        line[len-2] = '\0';

        return &line[2];
     }

     return RT_NULL;
}

int __parse_cfg_ver(char *line)
{
    int ver = CFG_VER;
    cJSON *json = cJSON_Parse(line);
    if(json) {
        ver = cJSON_GetInt(json, "ver", -1);
    }
    cJSON_Delete(json);

    return ((ver>=0)?(ver):(CFG_VER));
}

extern void setNetCfgWithJson(cJSON *pCfg);
extern void setTcpipCfgWithJson(cJSON *pCfg);
extern void setUartCfgWithJson(cJSON *pCfg);
extern void cfgSetVarManageExtDataWithJson(cJSON *pCfg);
extern void setGPRSNetCfgWithJson(cJSON *pCfg);
extern rt_err_t setGPRSWorkCfgWithJson(cJSON *pCfg);
extern void setAuthCfgWithJson(cJSON *pCfg);
extern rt_err_t setStorageCfgWithJson(cJSON *pCfg);
extern void setHostCfgWithJson(cJSON *pCfg);
extern rt_err_t setZigbeeCfgWithJson(cJSON *pCfg);
extern rt_bool_t setXferUartCfgWithJson(cJSON *pXferUCfg, rt_bool_t save_flag);
extern void varmanage_free_all(void);

void __parse_ini_line(cJSON *ini_item);

static rt_bool_t _ext_var_first = RT_TRUE;
static int       _ext_var_cnt = 0;

rt_bool_t __parse_cfg_line(char *line, int ver)
{
    rt_bool_t ret = RT_FALSE;
    cJSON *json = cJSON_Parse(line);
    if(json) {
        const char *type = cJSON_GetString(json, "type", RT_NULL);
        cJSON *cfg = cJSON_GetObjectItem(json, "cfg");
        if(type && cfg) {
            if(0 == strcasecmp(type, "net")) {
                setNetCfgWithJson(cfg);
            } else if(0 == strcasecmp(type, "tcpip")) {
                setTcpipCfgWithJson(cfg);
            } else if(0 == strcasecmp(type, "uart")) {
                setUartCfgWithJson(cfg);
            } else if(0 == strcasecmp(type, "extvar")) {
                if(_ext_var_first) {
                    if(board_cfg_varext_del_all()) {
                        varmanage_free_all();
                    }
                    _ext_var_first = RT_FALSE;
                }
                cfgSetVarManageExtDataWithJson(cfg);
                _ext_var_cnt++;
#if NET_HAS_GPRS
            } else if(0 == strcasecmp(type, "gprs_net")) {
                setGPRSNetCfgWithJson(cfg);
            } else if(0 == strcasecmp(type, "gprs_work")) {
                setGPRSWorkCfgWithJson(cfg);
#endif
            } else if(0 == strcasecmp(type, "auth")) {
                setAuthCfgWithJson(cfg);
            } else if(0 == strcasecmp(type, "storage")) {
                setStorageCfgWithJson(cfg);
            } else if(0 == strcasecmp(type, "host")) {
                setHostCfgWithJson(cfg);
            } else if(0 == strcasecmp(type, "zigbee")) {
                setZigbeeCfgWithJson(cfg);
            } else if(0 == strcasecmp(type, "uart_adds")) {
                setXferUartCfgWithJson(cfg, RT_TRUE);
            } else if(0 == strcasecmp(type, "ini")) {
                __parse_ini_line(cfg);
            }
        }
    }
    cJSON_Delete(json);
    return ret;
}

static rt_bool_t s_cfg_recover_ing = RT_FALSE;

rt_bool_t cfg_recover_busy(void)
{
    return s_cfg_recover_ing;
}

rt_bool_t cfg_recover_with_json(const char *path)
{
#define _LINE_BUFSZ     (10240+8)
    rt_bool_t ret = RT_FALSE;
    rt_bool_t first = RT_FALSE;
    int ver;
    int fd;

    s_cfg_recover_ing = RT_TRUE;

    fd = open(path, O_RDONLY, 0666);
    if (fd < 0) {
        return RT_FALSE;
    }

    {
        char *line_buf = rt_malloc(_LINE_BUFSZ);
        if(line_buf) {
            char *line = RT_NULL;
            _ext_var_cnt = 0;
            _ext_var_first = RT_TRUE;
            while(1) {
                line = __readline(fd, _LINE_BUFSZ, line_buf);
                if(line) {
                    line = __check_cfg_line(line);
                    if(line) {
                        if(!first) {
                            ver = __parse_cfg_ver(line);
                            first = RT_TRUE;
                        } else {
                            __parse_cfg_line(line, ver);
                        }
                    }
                } else {
                    break;
                }
            }
            rt_free(line_buf);
            if(!_ext_var_first) {
                bStorageDoCommit();
            }
            ret = first;
        }
    }
    close(fd);

    s_cfg_recover_ing = RT_FALSE;

    return ret;
}

typedef void (*cfgfunc)(int, cJSON *);

void __write_cfg_line(int fd, const char *type, cfgfunc cfunc, int n)
{
    cJSON *item = cJSON_CreateObject();
    cJSON *cfg = item?cJSON_CreateObject():RT_NULL;
    if(item && cfg) {
        cJSON_AddStringToObject(item, "type", type);
        cJSON_AddItemToObject(item, "cfg", cfg);
        if(cfunc) cfunc(n, cfg);
        {
            char *szJson = cJSON_PrintUnformatted(item);
            if(szJson) {
                write(fd, "##", 2);
                write(fd, szJson, strlen(szJson));
                write(fd, "##\n", 3);
            }
            rt_free(szJson);
        }
    }
    if(item) cJSON_Delete(item);
}

void __write_ini_line(int fd, int n, const char *path_prefix)
{
    cJSON *item = cJSON_CreateObject();
    cJSON *cfg = item?cJSON_CreateObject():RT_NULL;
    if(item && cfg) {
        cJSON_AddStringToObject(item, "type", "ini");
        cJSON_AddItemToObject(item, "cfg", cfg);
        {
            char path[128] = "";
            sprintf(path, "%s%d.ini", path_prefix, n);
            char *ini_buffer = das_read_text_file(path);
            if (ini_buffer) {
                char *p = ini_buffer;
                while (*p) {
                    if (*p == '\r') *p = 0x01;
                    if (*p == '\n') *p = 0x02;
                    p++;
                }
                cJSON_AddStringToObject(cfg, "path", path);
                cJSON_AddStringToObject(cfg, "data", ini_buffer);
                {
                    char *szJson = cJSON_PrintUnformatted(item);
                    if(szJson) {
                        write(fd, "##", 2);
                        write(fd, szJson, strlen(szJson));
                        write(fd, "##\n", 3);
                        rt_free(szJson);
                    }
                }
                free(ini_buffer);
            }
        }
    }
    if(item) cJSON_Delete(item);
}

void __parse_ini_line(cJSON *ini_item)
{
    const char *path = cJSON_GetString(ini_item, "path", NULL);
    const char *data = cJSON_GetString(ini_item, "data", NULL);
    printf("path = %s, data = %s\n", path, data);
    if (path && data && data[0]) {
        char *ini_data = das_strdup(NULL, data);
        if (ini_data) {
            char *p = ini_data;
            while (*p) {
                if (*p == 0x01) *p = '\r';
                if (*p == 0x02) *p = '\n';
                p++;
            }
            das_write_text_file(path, (const char *)ini_data, strlen(ini_data));
            free(ini_data);
        }
    }
}

extern void jsonNetCfg(int n, cJSON *pItem);
extern void jsonTcpipCfg(int n, cJSON *pItem);
extern void jsonUartCfg(int n, cJSON *pItem);
extern int nExtDataListCnt(void);
extern void jsonFillExtDataInfoWithNum(int n, cJSON *pItem);
extern void jsonGPRSNetCfg(int n, cJSON *pItem);
extern void jsonGPRSWorkCfg(int n, cJSON *pItem);
extern void jsonAuthCfg(int n, cJSON *pItem);
extern void jsonStorageCfg(int n, cJSON *pItem);
//extern void jsonHostCfg(int n, cJSON *pItem);
extern void __jsonHostCfg(int n, cJSON *pItem);
extern void jsonZigbeeCfg(int n, cJSON *pItem);
extern void jsonXferUartCfg(int n, cJSON *pItem);

rt_bool_t cfg_save_with_json(const char *path)
{
#define _LINE_BUFSZ     (10240+8)
    rt_bool_t ret = RT_FALSE;
    int fd;

    fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0666);
    if (fd < 0) {
        return RT_FALSE;
    }

    // VER
    {
        char szJson[64] = "";
        rt_sprintf(szJson, "##{\"ver\":%d}##\n", CFG_VER);
        write(fd, szJson, strlen(szJson));
    }


    // others
    __write_cfg_line( fd, "net", jsonNetCfg, 0);
    for (int n = 0; n < BOARD_TCPIP_MAX; n++) {
        __write_cfg_line( fd, "tcpip", jsonTcpipCfg, n);
    }
    for (int n = 0; n <= PROTO_DEV_RS_MAX; n++) {
        __write_cfg_line( fd, "uart", jsonUartCfg, n);
    }
    {
        int cnt = nExtDataListCnt();
        for (int n = 0; n < cnt; n++) {
            __write_cfg_line( fd, "extvar", jsonFillExtDataInfoWithNum, n);
        }
    }
#if NET_HAS_GPRS
    __write_cfg_line( fd, "gprs_net", jsonGPRSNetCfg, 0);
    __write_cfg_line( fd, "gprs_work", jsonGPRSWorkCfg, 0);
#endif
    __write_cfg_line( fd, "auth", jsonAuthCfg, 0);
    __write_cfg_line( fd, "storage", jsonStorageCfg, 0);
   // __write_cfg_line( fd, "host", jsonHostCfg, 0);
    __write_cfg_line( fd, "host", __jsonHostCfg, 0);
    __write_cfg_line( fd, "zigbee", jsonZigbeeCfg, 0);
    __write_cfg_line( fd, "uart_adds", jsonXferUartCfg, 0);

    // dm101
    for (int n = 0; n <= BOARD_TCPIP_MAX; n++) {
        __write_ini_line(fd, n, DM101_INI_CFG_PATH_PREFIX);
    }
    
    // hjt212
    for (int n = 0; n <= BOARD_TCPIP_MAX; n++) {
        __write_ini_line(fd, n, HJT212_INI_CFG_PATH_PREFIX);
    }
    
    // cc_bjdc
    for (int n = 0; n <= BOARD_TCPIP_MAX; n++) {
        __write_ini_line(fd, n, CC_BJDC_INI_CFG_PATH_PREFIX);
    }

    close(fd);
    
    return ret;
}

DEF_CGI_HANDLER(saveCfgWithJson)
{
    rt_err_t err = RT_EOK;
    /*const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        setTcpipCfgWithJson(pCfg);
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);*/

    cfg_save_with_json(BOARD_CFG_PATH "rtu_board_json.cfg");

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}

