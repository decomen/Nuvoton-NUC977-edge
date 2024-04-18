
#include <board.h>
#include <stdio.h>

gprs_net_cfg_t g_gprs_net_cfg;
gprs_work_cfg_t g_gprs_work_cfg;

#define in_range(c, lo, up) ((rt_uint8_t)c >= lo && (rt_uint8_t)c <= up)
#ifndef isdigit
#define isdigit(c)          in_range(c, '0', '9')
#endif
#ifndef isxdigit
#define isxdigit(c)         (isdigit(c) || in_range(c, 'a', 'f') || in_range(c, 'A', 'F'))
#endif
#ifndef islower
#define islower(c)          in_range(c, 'a', 'z')
#endif
#define getxnum(c)          (isdigit(c)?(c-'0'):(10+(islower(c)?(c-'a'):(c-'A'))))

static void prvSetGPRSNetCfgDefault(void)
{
    memset(&g_gprs_net_cfg, 0, sizeof(g_gprs_net_cfg));
}

static void prvSetGPRSWorkCfgDefault(void)
{
    memset(&g_gprs_work_cfg, 0, sizeof(g_gprs_work_cfg));
}

static void prvReadGPRSNetCfgFromFs(void)
{
    if (!board_cfg_read(GPRS_NET_CFG_NAME, &g_gprs_net_cfg, sizeof(g_gprs_net_cfg))) {
        prvSetGPRSNetCfgDefault();
    }
}

static void prvSaveGPRSNetCfgToFs(void)
{
    if (!board_cfg_write(GPRS_NET_CFG_NAME, &g_gprs_net_cfg, sizeof(g_gprs_net_cfg))) {
        ; //prvSetGPRSNetCfgDefault();
    }
}

static void prvReadGPRSWorkCfgFromFs(void)
{
    if (!board_cfg_read(GPRS_WORK_CFG_NAME, &g_gprs_work_cfg, sizeof(g_gprs_work_cfg))) {
        prvSetGPRSWorkCfgDefault();
    }
}

static void prvSaveGPRSWorkCfgToFs(void)
{
    if (!board_cfg_write(GPRS_WORK_CFG_NAME, &g_gprs_work_cfg, sizeof(g_gprs_work_cfg))) {
        ; //prvSetGPRSWorkCfgDefault();
        g_gprs_work_cfg.eWMode = GPRS_WM_SHUTDOWN;
    }
}

rt_err_t gprs_cfg_init(void)
{
    prvReadGPRSNetCfgFromFs();
    prvReadGPRSWorkCfgFromFs();

    return RT_EOK;
}

void jsonGPRSNetCfg(int n, cJSON *pItem)
{
    cJSON_AddStringToObject(pItem, "apn", g_gprs_net_cfg.szAPN);
    cJSON_AddStringToObject(pItem, "user", g_gprs_net_cfg.szUser);
    cJSON_AddStringToObject(pItem, "psk", g_gprs_net_cfg.szPsk);
    cJSON_AddStringToObject(pItem, "apnno", g_gprs_net_cfg.szAPNNo);
    cJSON_AddStringToObject(pItem, "msgno", g_gprs_net_cfg.szMsgNo);
}

// for webserver
DEF_CGI_HANDLER(getGPRSNetCfg)
{
    char *szRetJSON = RT_NULL;
    cJSON *pItem = cJSON_CreateObject();
    if(pItem) {
        cJSON_AddNumberToObject(pItem, "ret", RT_EOK);
        jsonGPRSNetCfg(0, pItem);
        szRetJSON = cJSON_PrintUnformatted(pItem);
        if(szRetJSON) {
            WEBS_PRINTF(szRetJSON);
            rt_free(szRetJSON);
        }
    }
    cJSON_Delete(pItem);

    WEBS_DONE(200);
}

void setGPRSNetCfgWithJson(cJSON *pCfg)
{
    const char *apn = cJSON_GetString(pCfg, "apn", VAR_NULL);
    const char *user = cJSON_GetString(pCfg, "user", VAR_NULL);
    const char *psk = cJSON_GetString(pCfg, "psk", VAR_NULL);
    const char *apnno = cJSON_GetString(pCfg, "apnno", VAR_NULL);
    const char *msgno = cJSON_GetString(pCfg, "msgno", VAR_NULL);

    gprs_net_cfg_t gprs_net_cfg_bak = g_gprs_net_cfg;
    if (apn && strlen(apn) < 16) {
        memset(g_gprs_net_cfg.szAPN, 0, 16); strcpy(g_gprs_net_cfg.szAPN, apn);
    }
    if (user && strlen(user) < 16) {
        memset(g_gprs_net_cfg.szUser, 0, 16); strcpy(g_gprs_net_cfg.szUser, user);
    }
    if (psk && strlen(psk) < 16) {
        memset(g_gprs_net_cfg.szPsk, 0, 16); strcpy(g_gprs_net_cfg.szPsk, psk);
    }
    if (apnno && strlen(apnno) < 22) {
        memset(g_gprs_net_cfg.szAPNNo, 0, 22); strcpy(g_gprs_net_cfg.szAPNNo, apnno);
    }
    if (msgno && strlen(msgno) < 22) {
        memset(g_gprs_net_cfg.szMsgNo, 0, 22); strcpy(g_gprs_net_cfg.szMsgNo, msgno);
    }

    if (memcmp(&gprs_net_cfg_bak, &g_gprs_net_cfg, sizeof(g_gprs_net_cfg)) != 0) {
        prvSaveGPRSNetCfgToFs();
    }
}

DEF_CGI_HANDLER(setGPRSNetCfg)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        setGPRSNetCfgWithJson(pCfg);
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}

void jsonGPRSWorkCfg(int n, cJSON *pItem)
{
    char buf[32 * 3];
    char tmp[4];
    cJSON_AddNumberToObject(pItem, "wm", g_gprs_work_cfg.eWMode);
    cJSON_AddNumberToObject(pItem, "om", g_gprs_work_cfg.eOMode);
    cJSON_AddNumberToObject(pItem, "dl", g_gprs_work_cfg.btDebugLvl);
    cJSON_AddStringToObject(pItem, "simno", g_gprs_work_cfg.szSIMNo);
    cJSON_AddNumberToObject(pItem, "it", g_gprs_work_cfg.ulInterval);
    cJSON_AddNumberToObject(pItem, "rt", g_gprs_work_cfg.ulRetry);
    buf[0] = '\0';
    for (int i = 0; i < g_gprs_work_cfg.btRegLen; i++) {
        snprintf(tmp, 4, "%02X ", g_gprs_work_cfg.btRegBuf[i]);
        strcat(buf, tmp);
    }
    buf[strlen(buf) - 1] = '\0';
    cJSON_AddStringToObject(pItem, "reg", buf);
    buf[0] = '\0';
    for (int i = 0; i < g_gprs_work_cfg.btHeartLen; i++) {
        snprintf(tmp, 4, "%02X ", g_gprs_work_cfg.btHeartBuf[i]);
        strcat(buf, tmp);
    }
    buf[strlen(buf) - 1] = '\0';
    cJSON_AddStringToObject(pItem, "hrt", buf);

}

// for webserver
DEF_CGI_HANDLER(getGPRSWorkCfg)
{
    char *szRetJSON = RT_NULL;
    cJSON *pItem = cJSON_CreateObject();
    if(pItem) {
        cJSON_AddNumberToObject(pItem, "ret", RT_EOK);
        jsonGPRSWorkCfg(0, pItem);
        szRetJSON = cJSON_PrintUnformatted(pItem);
        if(szRetJSON) {
            WEBS_PRINTF(szRetJSON);
            rt_free(szRetJSON);
        }
    }
    cJSON_Delete(pItem);

    WEBS_DONE(200);
}

rt_err_t setGPRSWorkCfgWithJson(cJSON *pCfg)
{
    rt_err_t err = RT_EOK;
    int wm = cJSON_GetInt(pCfg, "wm", -1);
    int om = cJSON_GetInt(pCfg, "om", -1);
    int dl = cJSON_GetInt(pCfg, "dl", -1);
    int it = cJSON_GetInt(pCfg, "it", -1);
    int rt = cJSON_GetInt(pCfg, "rt", -1);
    const char *simno = cJSON_GetString(pCfg, "simno", VAR_NULL);
    const char *reg = cJSON_GetString(pCfg, "reg", VAR_NULL);
    const char *hrt = cJSON_GetString(pCfg, "hrt", VAR_NULL);

    gprs_work_cfg_t gprs_cfg_bak = g_gprs_work_cfg;
    char tmp_str[128];
    char *tmp;
    if (wm >= 0 && wm < GPRS_WM_MAX) g_gprs_work_cfg.eWMode = wm;
    if (om >= 0 && om < GPRS_OM_MAX) g_gprs_work_cfg.eOMode = om;
    if (dl >= 0 && dl < 3) g_gprs_work_cfg.btDebugLvl = dl;
    if (it >= 0) g_gprs_work_cfg.ulInterval = it;
    if (rt >= 0) g_gprs_work_cfg.ulRetry = rt;
    if (simno && strlen(simno) < 12) {
        memset(g_gprs_work_cfg.szSIMNo, 0, 12); strcpy(g_gprs_work_cfg.szSIMNo, simno);
    }

    tmp_str[0] = '\0';
    tmp = RT_NULL;
    if (reg) {
        rt_strncpy(tmp_str, reg, sizeof(tmp_str) - 1); tmp = rt_trim(tmp_str);
    }
    if (tmp && strlen(tmp) < 3 * 32) {
        if ('\0' == tmp[0]) {
            g_gprs_work_cfg.btRegLen = 0;
        } else {
            int len = strlen(tmp);
            g_gprs_work_cfg.btRegLen = (len + 1) / 3;
            for (int i = 0; i < len; i += 3) {
                if ((tmp[i + 2] && tmp[i + 2] != ' ') || !isxdigit(tmp[i]) || !isxdigit(tmp[i + 1])) {
                    err = RT_ERROR;
                    break;
                } else {
                    g_gprs_work_cfg.btRegBuf[i / 3] = (rt_uint8_t)(getxnum(tmp[i]) << 4) | (rt_uint8_t)getxnum(tmp[i + 1]);
                }
            }
        }
    }
    if (RT_EOK == err) {
        tmp_str[0] = '\0';
        tmp = RT_NULL;
        if (hrt) {
            rt_strncpy(tmp_str, hrt, sizeof(tmp_str) - 1); tmp = rt_trim(tmp_str);
        }
        if (tmp && strlen(tmp) < 3 * 32) {
            if ('\0' == tmp[0]) {
                g_gprs_work_cfg.btHeartLen = 0;
            } else {
                int len = strlen(tmp);
                g_gprs_work_cfg.btHeartLen = (len + 1) / 3;
                for (int i = 0; i < len; i += 3) {
                    if ((tmp[i + 2] && tmp[i + 2] != ' ') || !isxdigit(tmp[i]) || !isxdigit(tmp[i + 1])) {
                        err = RT_ERROR;
                        break;
                    } else {
                        g_gprs_work_cfg.btHeartBuf[i / 3] = (rt_uint8_t)(getxnum(tmp[i]) << 4) | (rt_uint8_t)getxnum(tmp[i + 1]);
                    }
                }
            }
        }
    }

    if (RT_EOK == err) {
        if (memcmp(&gprs_cfg_bak, &g_gprs_work_cfg, sizeof(g_gprs_work_cfg)) != 0) {
            prvSaveGPRSWorkCfgToFs();
        }
    } else {
        g_gprs_work_cfg = gprs_cfg_bak;
    }
}

DEF_CGI_HANDLER(setGPRSWorkCfg)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        err = setGPRSWorkCfgWithJson(pCfg);
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}


static const char *ppp_info_path = "/tmp/ppp_info.json";
static char g_btCOPS[32] = {0};
//{"g_xGPRS_COPS":"CHINA MOBILE","g_nGPRS_CSQ":31,"g_nGPRS_CREG":1}

void get_ppp_info(void)
{
    FILE* fp = NULL;
    fp = fopen(ppp_info_path,"r+");
    if(fp){
        char fmt[128] = {0};
        fread(fmt,128,1,fp);
        //printf("fmt: %s\r\n",fmt);

        cJSON *pCfg = cJSON_Parse(fmt);
        if (pCfg) {
            g_nGPRS_CREG = cJSON_GetInt(pCfg, "g_nGPRS_CREG", -1);
            g_nGPRS_CSQ= cJSON_GetInt(pCfg, "g_nGPRS_CSQ", -1);
            const char *simno = cJSON_GetString(pCfg, "g_xGPRS_COPS", VAR_NULL);
            if(simno){
                memset(g_btCOPS,0,sizeof(g_btCOPS));
                strcpy(g_btCOPS,simno);
            }
        }
        cJSON_Delete(pCfg);
        fclose(fp);
       // printf("g_xGPRS_COPS:%s,g_nGPRS_CSQ:%d,g_nGPRS_CREG:%d\r\n",g_btCOPS,g_nGPRS_CSQ,g_nGPRS_CREG);         
    }else {
        printf("fopen err:%s\r\n",ppp_info_path);
    }
}


void jsonGPRSState(cJSON *pItem)
{
    char tmp[128] = { 0 };

    get_ppp_info();

    cJSON_AddNumberToObject(pItem, "pwr", g_gprs_work_cfg.eWMode != GPRS_WM_SHUTDOWN ? 1 : 0);

    cJSON_AddNumberToObject(pItem, "csq", g_nGPRS_CSQ);
    cJSON_AddNumberToObject(pItem, "creg", g_nGPRS_CREG);
    cJSON_AddStringToObject(pItem, "alphan", strlen(g_btCOPS) > 0 ? g_btCOPS : "");

    snprintf(tmp, sizeof(tmp),
             "MNC:%d,LAC:%04x,cell:%04x,BSIC:%02d,RxLev:%d dB",
             g_xGPRS_SMOND.xSMOND.xSCI.MNC,
             g_xGPRS_SMOND.xSMOND.xSCI.LAC,
             g_xGPRS_SMOND.xSMOND.xSCI.cell,
             g_xGPRS_SMOND.xSMOND.xSCI.BSIC,
             g_xGPRS_SMOND.xSMOND.xSCI.RxLev);
    cJSON_AddStringToObject(pItem, "area", tmp);

    {
            struct das_net_list_node net;
            memset(&net, 0, sizeof(net));
            das_do_get_net_info(DAS_NET_TYPE_GPRS, 0, &net);
            cJSON_AddStringToObject(pItem, "ip", net.IP);                   //I地址

            cJSON_AddNumberToObject(pItem, "n_type", E_4G_EC20); //网口类型GPRS/LTE/NBIOT,目前暂不支持NB-IOT
    }
    
    cJSON_AddNumberToObject(pItem, "adpt", net_adapter_get());
}

DEF_CGI_HANDLER(getGPRSState)
{
    char *szRetJSON = RT_NULL;
    cJSON *pItem = cJSON_CreateObject();
    if(pItem) {
        cJSON_AddNumberToObject(pItem, "ret", RT_EOK);
        jsonGPRSState(pItem);
        szRetJSON = cJSON_PrintUnformatted(pItem);
        if(szRetJSON) {
            WEBS_PRINTF(szRetJSON);
            rt_free(szRetJSON);
        }
    }
    cJSON_Delete(pItem);

    WEBS_DONE(200);
}

