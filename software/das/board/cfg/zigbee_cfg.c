
#include <board.h>

const zigbee_cfg_t c_zigbee_default_cfg = {
    .xInfo = {
        .WorkMode = ZIGBEE_WORK_END_DEVICE,
        .Chan = 11,
        .MsgMode = ZIGBEE_MSG_MODE_SINGLE
    },
    .ulLearnStep = 30,
    .btSlaveAddr = 0,
    .nProtoType = ZGB_SN_T_MODBUS_RTU,
    .tmode = ZGB_TM_GW
};

zigbee_cfg_t g_zigbee_cfg;

static void prvSetZigbeeCfgDefault(void)
{
    g_zigbee_cfg = c_zigbee_default_cfg;
}

static void prvReadZigbeeCfgFromFs(void)
{
    if (!board_cfg_read(ZIGBEE_CFG_NAME, &g_zigbee_cfg, sizeof(g_zigbee_cfg))) {
        prvSetZigbeeCfgDefault();
    }
    if (g_zigbee_cfg.ulLearnStep < 5) g_zigbee_cfg.ulLearnStep = 5;
    g_zigbee_cfg.nProtoType = ZGB_SN_T_MODBUS_RTU;
}

static void prvSaveZigbeeCfgToFs(void)
{
    if (!board_cfg_write(ZIGBEE_CFG_NAME, &g_zigbee_cfg, sizeof(g_zigbee_cfg))) {
        ; //prvSetAuthCfgDefault();
    }
}

rt_err_t zigbee_cfg_init(void)
{
    prvReadZigbeeCfgFromFs();

    return RT_EOK;
}

void jsonZigbeeCfg(int n,cJSON *pItem)
{
    char buf[64] = { 0 };
    char *mac = RT_NULL;

    cJSON_AddNumberToObject(pItem, "wmd", g_zigbee_cfg.xInfo.WorkMode);
    cJSON_AddNumberToObject(pItem, "pid", g_zigbee_cfg.xInfo.PanID);
    cJSON_AddNumberToObject(pItem, "src", g_zigbee_cfg.xInfo.Addr);

    mac = g_zigbee_cfg.xInfo.Mac; buf[0] = '\0';
    sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3],
            mac[4], mac[5], mac[6], mac[7]);
    cJSON_AddStringToObject(pItem, "smac", buf);

    cJSON_AddNumberToObject(pItem, "dst", g_zigbee_cfg.xInfo.DstAddr);

    mac = g_zigbee_cfg.xInfo.DstMac; buf[0] = '\0';
    sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3],
            mac[4], mac[5], mac[6], mac[7]);
    cJSON_AddStringToObject(pItem, "dmac", buf);

    cJSON_AddNumberToObject(pItem, "ch", g_zigbee_cfg.xInfo.Chan);
    cJSON_AddNumberToObject(pItem, "mmd", g_zigbee_cfg.xInfo.MsgMode);

    memset(buf, 0, sizeof(buf));
    extern rt_bool_t g_zigbee_init;
    if (g_zigbee_init) {
        sprintf(buf, "%X.%02X", (g_zigbee_cfg.usVer >> 8) & 0xFF, g_zigbee_cfg.usVer & 0xFF);
    }
    cJSON_AddStringToObject(pItem, "ver", buf);
    cJSON_AddNumberToObject(pItem, "lstep", g_zigbee_cfg.ulLearnStep);
    cJSON_AddNumberToObject(pItem, "slvad", g_zigbee_cfg.btSlaveAddr);
    cJSON_AddNumberToObject(pItem, "po", g_zigbee_cfg.nProtoType);
    cJSON_AddNumberToObject(pItem, "tm", g_zigbee_cfg.tmode);

    if(ZGB_TM_TRT == g_zigbee_cfg.tmode) {
        cJSON_AddNumberToObject(pItem, "dt", g_zigbee_cfg.dst_type);
        cJSON_AddNumberToObject(pItem, "uty", g_zigbee_cfg.dst_cfg.uart_cfg.type);
        cJSON_AddNumberToObject(pItem, "ubr", g_zigbee_cfg.dst_cfg.uart_cfg.cfg.baud_rate);
        cJSON_AddNumberToObject(pItem, "udb", g_zigbee_cfg.dst_cfg.uart_cfg.cfg.data_bits);
        cJSON_AddNumberToObject(pItem, "usb", g_zigbee_cfg.dst_cfg.uart_cfg.cfg.stop_bits);
        cJSON_AddNumberToObject(pItem, "upy", g_zigbee_cfg.dst_cfg.uart_cfg.cfg.parity);
    }
}

// for webserver
DEF_CGI_HANDLER(getZigbeeCfg)
{
    char *szRetJSON = RT_NULL;
    cJSON *pItem = cJSON_CreateObject();
    if(pItem) {
        extern rt_bool_t g_zigbee_init;
        if (g_zigbee_init) {
            cJSON_AddNumberToObject(pItem, "ret", RT_EOK);
            jsonZigbeeCfg(0, pItem);
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

rt_err_t setZigbeeCfgWithJson(cJSON *pCfg)
{
    rt_err_t err = RT_EOK;
    int wmd = cJSON_GetInt(pCfg, "wmd", -1);
    int pid = cJSON_GetInt(pCfg, "pid", -1);
    int src = cJSON_GetInt(pCfg, "src", -1);
    const char *smac = cJSON_GetString(pCfg, "smac", VAR_NULL);
    int dst = cJSON_GetInt(pCfg, "dst", -1);
    const char *dmac = cJSON_GetString(pCfg, "dmac", VAR_NULL);
    int ch = cJSON_GetInt(pCfg, "ch", -1);
    int mmd = cJSON_GetInt(pCfg, "mmd", -1);
    int lstep = cJSON_GetInt(pCfg, "lstep", -1);
    int slvad = cJSON_GetInt(pCfg, "slvad", -1);
    int po = cJSON_GetInt(pCfg, "po", -1);
    int tm = cJSON_GetInt(pCfg, "tm", -1);

    zigbee_cfg_t cfg_bak = g_zigbee_cfg;
    if (wmd >= ZIGBEE_WORK_END_DEVICE && wmd <= ZIGBEE_WORK_COORDINATOR) g_zigbee_cfg.xInfo.WorkMode = wmd;
    if (pid >= 1 && pid <= 0xFFFF) g_zigbee_cfg.xInfo.PanID = pid;
    if (src >= 1 && src <= 0xFFFF) g_zigbee_cfg.xInfo.Addr = src;
    if (dst >= 1 && dst <= 0xFFFF) g_zigbee_cfg.xInfo.DstAddr = dst;
    if (ch >= 11 && ch <= 26) g_zigbee_cfg.xInfo.Chan = ch;
    if (mmd >= ZIGBEE_MSG_MODE_SINGLE && mmd <= ZIGBEE_MSG_MODE_BROAD) g_zigbee_cfg.xInfo.MsgMode = mmd;
    if (lstep >= 5) g_zigbee_cfg.ulLearnStep = lstep;
    if (slvad >= 0) g_zigbee_cfg.btSlaveAddr = slvad;
    if (po >= 0) g_zigbee_cfg.nProtoType = po;
    if (tm >= 0 && tm < ZGB_TM_NUM) g_zigbee_cfg.tmode = tm;
    if (smac && strlen(smac) < 3 * sizeof(g_zigbee_cfg.xInfo.Mac)) {
        int tmp[8] = { 0 };
        memset(g_zigbee_cfg.xInfo.Mac, 0, sizeof(g_zigbee_cfg.xInfo.Mac));
        if (sscanf(smac, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                   &tmp[0], &tmp[1], &tmp[2], &tmp[3],
                   &tmp[4], &tmp[5], &tmp[6], &tmp[7]) >= 0) {
            for (int i = 0; i < 8; i++) {
                g_zigbee_cfg.xInfo.Mac[i] = tmp[i] & 0xFF;
            }
        }
    }
    if (dmac && strlen(dmac) < 3 * sizeof(g_zigbee_cfg.xInfo.DstMac)) {
        int tmp[8] = { 0 };
        memset(g_zigbee_cfg.xInfo.DstMac, 0, sizeof(g_zigbee_cfg.xInfo.DstMac));
        if (sscanf(smac, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                   &tmp[0], &tmp[1], &tmp[2], &tmp[3],
                   &tmp[4], &tmp[5], &tmp[6], &tmp[7]) >= 0) {
            for (int i = 0; i < 8; i++) {
                g_zigbee_cfg.xInfo.DstMac[i] = tmp[i] & 0xFF;
            }
        }
    }

    if(ZIGBEE_WORK_COORDINATOR == g_zigbee_cfg.xInfo.WorkMode) {
        g_zigbee_cfg.tmode = ZGB_TM_GW;
    }

    if(ZGB_TM_TRT == g_zigbee_cfg.tmode) {
        int dt = cJSON_GetInt( pCfg, "dt", -1 );
        int ubr = cJSON_GetInt( pCfg, "ubr", -1 );
        int udb = cJSON_GetInt( pCfg, "udb", -1 );
        int usb = cJSON_GetInt( pCfg, "usb", -1 );
        int upy = cJSON_GetInt( pCfg, "upy", -1 );

        if( dt >= 0 ) g_zigbee_cfg.dst_type = dt;
        if( ubr >= 0 ) g_zigbee_cfg.dst_cfg.uart_cfg.cfg.baud_rate = ubr;
        if( udb >= 0 ) g_zigbee_cfg.dst_cfg.uart_cfg.cfg.data_bits = udb;
        if( usb >= 0 ) g_zigbee_cfg.dst_cfg.uart_cfg.cfg.stop_bits = usb;
        if( upy >= 0 ) g_zigbee_cfg.dst_cfg.uart_cfg.cfg.parity = upy;
    }

    if (memcmp(&cfg_bak, &g_zigbee_cfg, sizeof(g_zigbee_cfg)) != 0) {
        if (ZIGBEE_ERR_OK == eZigbeeSetDevInfo(cfg_bak.xInfo.Addr, g_zigbee_cfg.xInfo)) {
            vZigbeeReset(cfg_bak.xInfo.Addr, g_zigbee_cfg.usType);
            prvSaveZigbeeCfgToFs();

            vZigbeeLearnNow();
        } else {
            g_zigbee_cfg = cfg_bak;
            err = RT_ERROR;
        }
    }
}

DEF_CGI_HANDLER(setZigbeeCfg)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        err = setZigbeeCfgWithJson(pCfg);
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}

