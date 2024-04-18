#include <board.h>

#define CFG_MAGIC               0xD6BCF4ED

static void prvReadDevInfoFromFs(void)
{
    int fd = open("/tmp/deviceInfo.json", O_RDONLY, 0666);
    if (fd > 0) {
        int len = lseek(fd, 0, SEEK_END);
        if (len > 0) {
            char *buffer = rt_calloc(1, len + 1);
            if (buffer) {
                lseek(fd, 0, SEEK_SET);
                read(fd, buffer, len);
                close(fd);
                {
                    cJSON *dev_info = cJSON_Parse(buffer);
                    if (dev_info) {
                        das_strcpy_s(g_sys_info.DEV_ID, cJSON_GetString(dev_info, "DeviceName", ""));
                        das_strcpy_s(g_sys_info.SN, cJSON_GetString(dev_info, "DeviceSn", ""));
                        das_strcpy_s(g_sys_info.HW_VER, cJSON_GetString(dev_info, "DeviceHwVer", ""));
                        das_strcpy_s(g_sys_info.SW_VER, cJSON_GetString(dev_info, "DeviceSwVer", ""));
                        das_strcpy_s(g_sys_info.HW_ID, cJSON_GetString(dev_info, "DeviceHwId", ""));
                        das_strcpy_s(g_sys_info.PROD_DATE, cJSON_GetString(dev_info, "DeviceProDate", ""));
                        das_strcpy_s(g_sys_info.DESC, cJSON_GetString(dev_info, "VerDesc", ""));
                        g_sys_info.DEV_MODEL = cJSON_GetInt(dev_info, "DeviceModel", 0);
                        g_sys_ver.HW_VER = (uint32_t)atoi(g_sys_info.HW_VER);
                        g_sys_ver.SW_VER = (uint32_t)atoi(g_sys_info.SW_VER);
                        cJSON_Delete(dev_info);
                    }
                }
                rt_free(buffer);
            }
        }
    }
    vDevRefreshTime();
    g_sys_info.REG_STATUS   = 0;
    g_sys_info.TEST_REMAIN  = 0;
}

void vDevCfgInit(void)
{
    prvReadDevInfoFromFs();
}

void vDevRefreshTime(void)
{
    char tmp[64] = {0};
    time_t t = time(0);
    struct tm tm;
    das_localtime_r(&t, &tm);
    sprintf(tmp, "%04d/%02d/%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    das_strcpy_s(g_sys_info.SYS_DATE, (const char *)tmp);
    g_sys_info.RUNTIME =  das_sys_time() / 60;
}

// for webserver
DEF_CGI_HANDLER(getDevInfo)
{
    rt_err_t err = RT_EOK;
    char *szRetJSON = RT_NULL;
    char buf[128];
    cJSON *pItem = cJSON_CreateObject();

    if(pItem) {
        cJSON_AddNumberToObject(pItem, "ret", RT_EOK);
        cJSON_AddStringToObject(pItem, "id", g_host_cfg.szId);
        cJSON_AddStringToObject(pItem, "sn", g_sys_info.SN);
        cJSON_AddStringToObject(pItem, "om", "");
        {
            struct das_net_list_node net;
            memset(&net, 0, sizeof(net));
            das_do_get_net_info(DAS_NET_TYPE_ETH, 0, &net);
            cJSON_AddStringToObject(pItem, "mc", net.MAC);
        }

        cJSON_AddStringToObject(pItem, "hw", g_sys_info.HW_VER);
        cJSON_AddStringToObject(pItem, "sw", g_sys_info.SW_VER);

        vDevRefreshTime();
        cJSON_AddStringToObject(pItem, "dt", g_sys_info.SYS_DATE);

        memset(buf, 0, sizeof(buf));
        /*extern rt_bool_t g_zigbee_init;
        if (g_zigbee_init) {
            sprintf(buf, "%X.%02X", (g_zigbee_cfg.usVer >> 8) & 0xFF, g_zigbee_cfg.usVer & 0xFF);
        }
        cJSON_AddStringToObject(pItem, "zgbver", buf);*/
        cJSON_AddStringToObject(pItem, "loraver", g_lora_ver);

        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%02d hours, %02d min", g_sys_info.RUNTIME / 60, g_sys_info.RUNTIME % 60);
        cJSON_AddStringToObject(pItem, "rt", buf);
        
        cJSON_AddStringToObject(pItem, "desc", g_sys_info.DESC);

        sprintf(buf, "V%d.%02d, build %s, %s\n", DAS_VER_VERCODE / 100, DAS_VER_VERCODE % 100, __DATE__, __TIME__);
        cJSON_AddStringToObject(pItem, "das_desc", buf);

        cJSON_AddNumberToObject(pItem, "reg", g_isreg);
        cJSON_AddNumberToObject(pItem, "tta", REG_TEST_OVER_TIME);
        cJSON_AddNumberToObject(pItem, "ttt", g_reg_info.test_time);

        szRetJSON = cJSON_PrintUnformatted(pItem);
        if(szRetJSON) {
            WEBS_PRINTF(szRetJSON);
            rt_free(szRetJSON);
        }
        cJSON_Delete(pItem);
    }

    WEBS_DONE(200);
}

DEF_CGI_HANDLER(setTime)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        struct tm lt = { 0 };
        int ye = cJSON_GetInt(pCfg, "ye", -1);
        int mo = cJSON_GetInt(pCfg, "mo", -1);
        int da = cJSON_GetInt(pCfg, "da", -1);
        int dh = cJSON_GetInt(pCfg, "dh", -1);
        int hm = cJSON_GetInt(pCfg, "hm", -1);
        int ms = cJSON_GetInt(pCfg, "ms", -1);

        lt.tm_year = ye - 1900;
        lt.tm_mon = mo - 1;
        lt.tm_mday = da;
        lt.tm_hour = dh;
        lt.tm_min = hm;
        lt.tm_sec = ms;
        das_set_time(mktime(&lt), g_host_cfg.nTimezone);
        my_system("hwclock -w -u");
        printf("set time\n");
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}

