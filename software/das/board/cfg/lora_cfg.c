
#include <board.h>
const lora_cfg_t c_lora_default_cfg = {
    .info = {
        //.ak         = "00000000000000000000000000000000", 
        .pow        = 0x14, 
        .bw         = LORA_BW_125K, 
        .cr         = LORA_CR_4_5, 
        .crc        = 0x01, 
        .tfreq      = 506500000, 
        .rfreq      = 475500000, 
        .tsf        = LORA_FS_7, 
        .rsf        = LORA_FS_7, 
        .net_type   = LORA_FREQ_MODE_FIXED, 
        .mode       = LORA_WORK_MODE_LORA, 
        .sync       = 0x12, 
        .tprem      = 0x0008, 
        .rprem      = 0x000A, 
        .ldr        = 0x00, 
        .tiq        = 0x00, 
        .riq        = 0x00, 
        .sip        = LORA_SIP_NONE, 
        .brate      = LORA_SERIAL_RATE_9600, 
        .par        = LORA_SERIAL_PAR_NONE, 
        .data_type  = LORA_DATA_TYPE_SIMPLER, 
        .lcp        = 0x0000, 
        .lft        = 0x0000, 
        .lat        = 0x0000, 
        .lgt        = 0x0000, 
        .el         = 0xA8C0, 
    },
    .dst_addr = 0x00000000,
    .work_mode = LORA_WORK_CENTRAL, 
    .learnstep = 30,
    .slave_addr = 0,
    .proto_type = LORA_SN_T_MODBUS_RTU,
    .tmode = LORA_TM_GW
};

lora_cfg_t g_lora_cfg;

static void prvSetLoRaCfgDefault(void)
{
    g_lora_cfg = c_lora_default_cfg;
}

static void prvReadLoRaCfgFromFs(void)
{
    if (!board_cfg_read(LORA_CFG_NAME, &g_lora_cfg, sizeof(g_lora_cfg))) {
        prvSetLoRaCfgDefault();
    }
    g_lora_cfg.info.net_type    = LORA_FREQ_MODE_FIXED;
    g_lora_cfg.info.mode        = LORA_WORK_MODE_LORA;
    g_lora_cfg.info.sync        = 0x12;
    g_lora_cfg.info.tprem       = 0x0008;
    g_lora_cfg.info.rprem       = 0x000A;
    g_lora_cfg.info.ldr         = 0x00;
    g_lora_cfg.info.tiq         = 0x00;
    g_lora_cfg.info.riq         = 0x00;
    g_lora_cfg.info.sip         = LORA_SIP_NONE;
    g_lora_cfg.info.data_type   = LORA_DATA_TYPE_SIMPLER;
    g_lora_cfg.info.lcp         = 0x0000;
    g_lora_cfg.info.lft         = 0x0000;
    g_lora_cfg.info.lat         = 0x0000;
    g_lora_cfg.info.lgt         = 0x0000;
    g_lora_cfg.info.el          = 0xA8C0;
    g_lora_cfg.work_mode        = LORA_WORK_CENTRAL;
    g_lora_cfg.proto_type       = LORA_SN_T_MODBUS_RTU;
    g_lora_cfg.tmode            = LORA_TM_GW;
    
    //g_lora_cfg.info.net_type   = LORA_FREQ_MODE_FIXED;
    if (g_lora_cfg.learnstep < 5) g_lora_cfg.learnstep = 5;
}

static void prvSaveLoRaCfgToFs(void)
{
    if (!board_cfg_write(LORA_CFG_NAME, &g_lora_cfg, sizeof(g_lora_cfg))) {
        ; //prvSetLoRaCfgDefault();
    }
}

rt_err_t lora_cfg_init(void)
{
    prvReadLoRaCfgFromFs();

    return RT_EOK;
}

void jsonLoRaCfg(int n,cJSON *pItem)
{
    char buf[64] = { 0 };

    cJSON_AddNumberToObject(pItem, "wmd", g_lora_cfg.work_mode);
    cJSON_AddNumberToObject(pItem, "lstep", g_lora_cfg.learnstep);

    memset(buf, 0, sizeof(buf));
    sprintf(buf, "%02X%02X%02X%02X%02X%02X%02X%02X",
            g_lora_cfg.info.id[0], g_lora_cfg.info.id[1], g_lora_cfg.info.id[2], g_lora_cfg.info.id[3],
            g_lora_cfg.info.id[4], g_lora_cfg.info.id[5], g_lora_cfg.info.id[6], g_lora_cfg.info.id[7]);
    cJSON_AddStringToObject(pItem, "id", buf);

    memset(buf, 0, sizeof(buf));
    sprintf(buf, "%08X", g_lora_cfg.info.maddr);
    cJSON_AddStringToObject(pItem, "maddr", buf);
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "%08X", g_lora_cfg.info.addr);
    cJSON_AddStringToObject(pItem, "addr", buf);
    cJSON_AddNumberToObject(pItem, "bw", g_lora_cfg.info.bw);
    cJSON_AddNumberToObject(pItem, "cr", g_lora_cfg.info.cr);
    cJSON_AddNumberToObject(pItem, "pow", g_lora_cfg.info.pow);
    cJSON_AddNumberToObject(pItem, "tfreq", g_lora_cfg.info.tfreq);
    cJSON_AddNumberToObject(pItem, "rfreq", g_lora_cfg.info.rfreq);
    cJSON_AddNumberToObject(pItem, "tsf", g_lora_cfg.info.tsf);
    cJSON_AddNumberToObject(pItem, "rsf", g_lora_cfg.info.rsf);
    cJSON_AddNumberToObject(pItem, "it", g_lora_cfg.interval);

    extern rt_bool_t g_lora_init;
    if (g_lora_init) {
        cJSON_AddStringToObject(pItem, "ver", g_lora_ver);
    } else {
        cJSON_AddStringToObject(pItem, "ver", "");
    }
}

// for webserver
DEF_CGI_HANDLER(getLoRaCfg)
{
    char *szRetJSON = RT_NULL;
    cJSON *pItem = cJSON_CreateObject();
    if(pItem) {
        extern rt_bool_t g_lora_init;
        if (g_lora_init) {
            cJSON_AddNumberToObject(pItem, "ret", RT_EOK);
            jsonLoRaCfg(0, pItem);
        } else {
            cJSON_AddNumberToObject(pItem, "ret", RT_ERROR);
        }
        szRetJSON = cJSON_PrintUnformatted(pItem);
        if(szRetJSON) {
            WEBS_PRINTF(szRetJSON);
            rt_free(szRetJSON);
        }
    }
    cJSON_Delete(pItem);

    WEBS_DONE(200);
}

rt_err_t setLoRaCfgWithJson(cJSON *pCfg)
{
    rt_err_t err = RT_EOK;
    lora_cfg_t cfg_bak = g_lora_cfg;
    const char *str = NULL;
    
    g_lora_cfg.learnstep    = cJSON_GetInt(pCfg, "lstep",   g_lora_cfg.learnstep);
    str = cJSON_GetString(pCfg, "maddr", NULL);
    if (str) g_lora_cfg.info.maddr = strtol(str, NULL, 16);
    str = cJSON_GetString(pCfg, "addr", NULL);
    if (str) g_lora_cfg.info.addr = strtol(str, NULL, 16);
    g_lora_cfg.info.bw      = cJSON_GetInt(pCfg, "bw",      g_lora_cfg.info.bw);
    g_lora_cfg.info.cr      = cJSON_GetInt(pCfg, "cr",      g_lora_cfg.info.cr);
    g_lora_cfg.info.pow     = cJSON_GetInt(pCfg, "pow",     g_lora_cfg.info.pow);
    g_lora_cfg.info.tfreq   = cJSON_GetInt(pCfg, "tfreq",   g_lora_cfg.info.tfreq);
    g_lora_cfg.info.rfreq   = cJSON_GetInt(pCfg, "rfreq",   g_lora_cfg.info.rfreq);
    g_lora_cfg.info.tsf     = cJSON_GetInt(pCfg, "tsf",     g_lora_cfg.info.tsf);
    g_lora_cfg.info.rsf     = cJSON_GetInt(pCfg, "rsf",     g_lora_cfg.info.rsf);
    g_lora_cfg.interval     = cJSON_GetInt(pCfg, "it",      g_lora_cfg.interval);

    if (g_lora_cfg.interval < 100) g_lora_cfg.interval = 100;

    if (memcmp(&cfg_bak, &g_lora_cfg, sizeof(g_lora_cfg)) != 0) {
        prvSaveLoRaCfgToFs();
        lora_at_req_all();
        lora_at_set_all();
        lora_at_req_all_cfg();
        lora_learn_now();
    }
}

DEF_CGI_HANDLER(setLoRaCfg)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        err = setLoRaCfgWithJson(pCfg);
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}


