
#include "board.h"
#include "mxml.h"
#include "cc_md5.h"
#include "aes.h"

#include "sdccp_net.h"

/*
typedef struct {
    rt_uint8_t      n;
    rt_uint32_t     time;
    rt_uint32_t     count;
    var_double_t    *val_avg;
    var_double_t    *val_cur;
    char            **sections;
} CC_BJDC_Relate_t;
*/

const rt_uint8_t c_xAESKeyAndIV[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
static eCC_BJDC_RcvState_t s_eRcvState[BOARD_TCPIP_MAX];
static rt_thread_t s_bjdc_work_thread[BOARD_TCPIP_MAX];
static rt_uint32_t s_ulIdeStartTime[BOARD_TCPIP_MAX];
static eCC_BJDC_VerifyState_t s_eVerifyState[BOARD_TCPIP_MAX];

static void cc_bjdc_work_task(void *parameter);
mxml_node_t *cc_bjdc_data_create_reply_or_report_head(
    rt_uint8_t index,
    const char *building_id,
    const char *gateway_id,
    rt_int32_t  sequence,
    rt_bool_t   report
    );
int cc_bjdc_data_add_data(
    mxml_node_t  *xml,
    const char  *meter_id,
    const char  *func_idex,
    double      value
    );
static void cc_bjdc_send_xml(rt_uint8_t index, mxml_node_t *xml, int size);

static ini_t *s_xCC_BJDC_cfg[BOARD_TCPIP_MAX];
static int s_nCC_BJDC_DescId[BOARD_TCPIP_MAX];

//static CC_BJDC_Relate_t s_xCC_BJDC_Relate[BOARD_TCPIP_MAX];

rt_bool_t cc_bjdc_open(rt_uint8_t index)
{
    s_nCC_BJDC_DescId[index] = -1;
    RT_KERNEL_FREE(s_pCCBuffer[index][0]);
    s_pCCBuffer[index][0] = RT_KERNEL_CALLOC(CC_BJDC_BUF_SIZE);
    s_CCBufferPos[index][0] = 0;
    s_cc_reinit_flag[index][0] = RT_FALSE;
    s_eRcvState[index] = CC_BJDC_R_S_HEAD;
    AES_Init(c_xAESKeyAndIV);

    {
        char buf[64] = "";
        sprintf(buf, CC_BJDC_INI_CFG_PATH_PREFIX "%d" ".ini", index);
        s_xCC_BJDC_cfg[index] = ini_load(buf);

        /*[index].n = 0;
        for (int i = 0; i < 100; i++) {
            sprintf(buf, "relate_%02d", i);
            if (ini_find_section(s_xCC_BJDC_cfg[index], buf)) {
                s_xCC_BJDC_Relate[index].n++;
            }
        }
        if (s_xCC_BJDC_Relate[index].n > 0) {
            s_xCC_BJDC_Relate[index].val_avg = rt_calloc(sizeof(double), s_xCC_BJDC_Relate[index].n);
            s_xCC_BJDC_Relate[index].val_cur = rt_calloc(sizeof(double), s_xCC_BJDC_Relate[index].n);
            s_xCC_BJDC_Relate[index].sections = rt_calloc(sizeof(char *), s_xCC_BJDC_Relate[index].n);
            for (int i = 0; i < s_xCC_BJDC_Relate[index].n; i++) {
                sprintf(buf, "relate_%02d", i);
                inisection_t *section = ini_find_section(s_xCC_BJDC_cfg[index], buf);
                if (section) {
                    s_xCC_BJDC_Relate[index].sections[i] = section->name;
                }
            }
        }*/
    }

    BOARD_CREAT_NAME(szMq, "bjmq_%d", index);
    s_CCDataQueue[index] = rt_mq_create(szMq, sizeof(CC_BJDC_Data_t), 3, RT_IPC_FLAG_PRIO);

    return RT_TRUE;
}

void cc_bjdc_close(rt_uint8_t index)
{
    for (int i = 0; i < 5 * 10; i++) {
        if (s_bjdc_work_thread[index]) {
            rt_thread_delay(RT_TICK_PER_SECOND / 10);
        } else {
            break;
        }
    }

    cc_bjdc_exitwork(index);

    if (s_CCDataQueue[index]) {
        rt_mq_delete(s_CCDataQueue[index]);
        s_CCDataQueue[index] = RT_NULL;
    }
    RT_KERNEL_FREE(s_pCCBuffer[index][0]);
    s_pCCBuffer[index][0] = RT_NULL;
}

void cc_bjdc_startwork(rt_uint8_t index)
{
    if(RT_NULL == s_bjdc_work_thread[index]) {
        BOARD_CREAT_NAME(szWork, "bjwork_%d", index);
        s_bjdc_work_thread[index] = \
            rt_thread_create(szWork,
                                    cc_bjdc_work_task,
                                    (void *)(long)index,
                                    2048,
                                    20, 20);
        if (s_bjdc_work_thread[index] != RT_NULL) {
            rt_thddog_register(s_bjdc_work_thread[index], 30);
            rt_thread_startup(s_bjdc_work_thread[index]);
        }
    }
}

void cc_bjdc_exitwork(rt_uint8_t index)
{
    s_cc_reinit_flag[index][0] = RT_TRUE;
}

rt_uint16_t const _CC_Table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7, 0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6, 0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485, 0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4, 0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823, 0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12, 0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41, 0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70, 0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f, 0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e, 0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d, 0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c, 0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a, 0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8, 0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

rt_uint16_t CC_GetCrc16(rt_uint8_t *pData, rt_uint32_t ulLen)
{
    rt_uint16_t crc = 0, by; rt_uint32_t i;
    for (i = 0; i < ulLen; i++) {
        by = (crc >> 8) & 0xff; crc = (crc & 0xffff) << 8;
        crc = (crc ^ _CC_Table[(pData[i] ^ by) & 0xff]) & 0xffff;
    }
    return crc;
}

static void __CC_BJDC_ParsePack(rt_uint8_t index, CC_BJDC_Package_t *pPack);
rt_err_t CC_BJDC_PutBytes(rt_uint8_t index, rt_uint8_t *pBytes, rt_uint16_t usBytesLen)
{
    {
        rt_uint8_t *pBuffer = s_pCCBuffer[index][0];
        rt_base_t nPos = s_CCBufferPos[index][0];
        eCC_BJDC_RcvState_t eRcvState = s_eRcvState[index];

        for (int n = 0; n < usBytesLen; n++) {
            pBuffer[nPos++] = pBytes[n];
            if (nPos >= CC_BJDC_BUF_SIZE) nPos = 0;
            if (CC_BJDC_R_S_HEAD == eRcvState) {
                if (4 == nPos) {
                    rt_uint32_t pre = 0; memcpy(&pre, pBuffer, 4);
                    if (CC_BJDC_PRE == pre) {
                        eRcvState = CC_BJDC_R_S_EOM;
                    } else {
                        nPos = 2;
                    }
                }
            } else if (CC_BJDC_R_S_EOM == eRcvState) {
                if (nPos >= 8) {
                    rt_uint32_t eom = 0; memcpy(&eom, &pBuffer[nPos - 4], 4);
                    if (CC_BJDC_EOM == eom && nPos >= 18) {
                        CC_BJDC_Package_t xPack;
                        memcpy(&xPack.ulPre, &pBuffer[0], 12);
                        xPack.xMsg.pData = &pBuffer[12];
                        memcpy(&xPack.usCheck, &pBuffer[nPos - 6], 6);

                        rt_uint32_t ulLen = cc_bjdc_htonl(xPack.ulLen);
                        rt_uint16_t usCheck = cc_bjdc_htons(xPack.usCheck);
                        if ((ulLen + 14) == nPos && CC_GetCrc16((rt_uint8_t *)&pBuffer[8], ulLen) == usCheck) {
                            // do parse
                            AES_Decrypt(
                                (unsigned char *)xPack.xMsg.pData,
                                (const unsigned char *)xPack.xMsg.pData,
                                ulLen - 4,
                                c_xAESKeyAndIV
                                );
                            xPack.xMsg.pData[ulLen - 4] = '\0';
                            __CC_BJDC_ParsePack(index, &xPack);
                        }
                        eRcvState = CC_BJDC_R_S_HEAD;
                        nPos = 0;
                    }
                }
            }
        }

        s_CCBufferPos[index][0] = nPos;
        s_eRcvState[index] = eRcvState;
    }

    return RT_EOK;
}

static rt_bool_t __xml_str(mxml_node_t *node, const char *str)
{
    return (node && node->child && (0 == strcmp(node->child->value.text.string, str)));
}

static void __CC_BJDC_ParsePack(rt_uint8_t index, CC_BJDC_Package_t *pPack)
{
    CC_BJDC_Data_t xData;
    mxml_node_t *xml = mxmlLoadString(RT_NULL, (const char *)pPack->xMsg.pData, MXML_NO_CALLBACK);
    mxml_node_t *root;
    mxml_node_t *common;
    mxml_node_t *type;
    mxml_node_t *node;
    const char *op;

    //rt_kprintf("----------recv:----------\n");
    //rt_kprintf("%s\n", (const char *)pPack->xMsg.pData);
    //rt_kprintf("-------------------------\n");

    if (!xml) return;

    xData.eType = CC_BJDC_PT_UNKNOWN;
    root = mxmlFindElement(xml, xml, "root", NULL, NULL, MXML_DESCEND);
    common = mxmlFindElement(root, root, "common", NULL, NULL, MXML_DESCEND);
    type = mxmlFindElement(common, root, "type", NULL, NULL, MXML_DESCEND);

    if (__xml_str(type, "sequence")) {
        node = mxmlFindElement(root, xml, "id_validate", NULL, NULL, MXML_DESCEND);
        node = mxmlFindElement(node, root, "sequence", NULL, NULL, MXML_DESCEND);
        if (node && node->child) {
            xData.eType = CC_BJDC_PT_VERIFY;
            xData.xData.xVerify.btState = CC_BJDC_VERIFY_SEQ;
            strncpy(xData.xData.xVerify.xParam.szSequence, node->child->value.text.string, 9);
        }
    } else if (__xml_str(type, "result")) {
        node = mxmlFindElement(root, xml, "id_validate", NULL, NULL, MXML_DESCEND);
        node = mxmlFindElement(node, root, "result", NULL, NULL, MXML_DESCEND);
        if (node && node->child) {
            xData.eType = CC_BJDC_PT_VERIFY;
            xData.xData.xVerify.btState = CC_BJDC_VERIFY_RESULT;
            xData.xData.xVerify.xParam.bPass = (0 == strcmp("pass", node->child->value.text.string));
        }
    } else if (__xml_str(type, "device_ack")) {
        xData.eType = CC_BJDC_PT_DEVINFO;
    } else if (__xml_str(type, "time")) {
        node = mxmlFindElement(root, xml, "heart_beat", NULL, NULL, MXML_DESCEND);
        node = mxmlFindElement(node, root, "time", NULL, NULL, MXML_DESCEND);
        if (node && node->child) {
            xData.eType = CC_BJDC_PT_HEARTBEAT;
            strncpy(xData.xData.xHeartbeat.szDate, node->child->value.text.string, 15);
        }
    } else if (__xml_str(type, "period")) {
        node = mxmlFindElement(root, xml, "config", NULL, NULL, MXML_DESCEND);
        op = mxmlElementGetAttr(node, "operation");
        node = mxmlFindElement(root, xml, "period", NULL, NULL, MXML_DESCEND);
        if (node && node->child) {
            xData.eType = CC_BJDC_PT_PERIOD_CFG;
            xData.xData.xPeriod.ulPeriod = node->child->value.integer;
        }
    } else if (__xml_str(type, "restart")) {
        node = mxmlFindElement(root, xml, "Instruction", NULL, NULL, MXML_DESCEND);
        op = mxmlElementGetAttr(node, "operation");
        if (0 == strcmp(op, "restart")) {
            xData.eType = CC_BJDC_PT_RESTART;
        }
    } else if (__xml_str(type, "query")) {
        xData.eType = CC_BJDC_PT_DATA_QUERY;
    } else if (__xml_str(type, "continuous")) {
        xData.eType = CC_BJDC_PT_DATA_CONTINUE;
    }

    if (xData.eType != CC_BJDC_PT_UNKNOWN) {
        rt_mq_send(s_CCDataQueue[index], &xData, sizeof(CC_BJDC_Data_t));
    }
_END:
    mxmlDelete(xml);
}

// 以下暂时为测试使用
//VarAvg_t s_Avg[BOARD_TCPIP_MAX];

static void cc_bjdc_work_task(void *parameter)
{
    rt_uint8_t index = (int)(long)parameter;
    CC_BJDC_Data_t xData;
    rt_uint32_t ulHeartbeatTick;
    tcpip_cfg_t *pcfg = &g_tcpip_cfgs[index];
    const ini_t *ini = s_xCC_BJDC_cfg[index];
    //CC_BJDC_Relate_t *relate = &s_xCC_BJDC_Relate[index];

_START:
    if (ini) {
        const char *building_id = ini_getstring(ini, "common", "building_id", "");
        const char *gateway_id = ini_getstring(ini, "common", "gateway_id", "");
        const char *build_name = ini_getstring(ini, "device", "build_name", "");
        const char *build_no = ini_getstring(ini, "device", "build_no", "");
        const char *dev_no = ini_getstring(ini, "device", "dev_no", "");
        int dev_num = ini_getint(ini, "device", "dev_num", 1);
        //int period = ini_getint(ini, "device", "period", 1);   // 默认 1 分钟
        const char *address = ini_getstring(ini, "device", "address", "");
        var_int32_t up_interval = lVarManage_GetExtDataUpInterval(PROTO_DEV_NET_TYPE(index), PROTO_DEV_NET_INDEX(index));
        int period = up_interval>0?(up_interval/60):180;

        rt_kprintf("...start verify\n");
        while (1) {
            rt_thddog_feed("");
            s_eVerifyState[index] = CC_BJDC_VERIFY_SEQ;
            rt_kprintf("cc_bjdc_verify_req\n");
            rt_thddog_suspend("cc_bjdc_verify_req");
            cc_bjdc_verify_req(index, building_id, gateway_id);
            rt_thddog_suspend("rt_mq_recv verify");
            if (RT_EOK == rt_mq_recv(s_CCDataQueue[index], &xData, sizeof(CC_BJDC_Data_t), 10 * RT_TICK_PER_SECOND)) {
                rt_thddog_resume();
                if (CC_BJDC_PT_VERIFY == xData.eType && CC_BJDC_VERIFY_SEQ == xData.xData.xVerify.btState) {
                    CC_MD5_CTX xmdContext;
                    char md5[33];
                    {
                        CC_MD5Init(&xmdContext);
                        CC_MD5Update(
                            &xmdContext,
                            (unsigned char *)xData.xData.xVerify.xParam.szSequence,
                            strlen(xData.xData.xVerify.xParam.szSequence)
                            );
                        CC_MD5Update(&xmdContext, CC_BJDC_MD5_KEY, strlen(CC_BJDC_MD5_KEY));
                        CC_MD5Final(&xmdContext);
                        rt_sprintf(md5, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                                   xmdContext.digest[0], xmdContext.digest[1], xmdContext.digest[2], xmdContext.digest[3],
                                   xmdContext.digest[4], xmdContext.digest[5], xmdContext.digest[6], xmdContext.digest[7],
                                   xmdContext.digest[8], xmdContext.digest[9], xmdContext.digest[10], xmdContext.digest[11],
                                   xmdContext.digest[12], xmdContext.digest[13], xmdContext.digest[14], xmdContext.digest[15]);
                    }
                    s_eVerifyState[index] = CC_BJDC_VERIFY_RESULT;
                    rt_kprintf("cc_bjdc_checkmd5_req\n");
                    rt_thddog_suspend("cc_bjdc_checkmd5_req");
                    cc_bjdc_checkmd5_req(index, building_id, gateway_id, md5);
                    rt_thddog_suspend("rt_mq_recv checkmd5");
                    if (RT_EOK == rt_mq_recv(s_CCDataQueue[index], &xData, sizeof(CC_BJDC_Data_t), 5 * RT_TICK_PER_SECOND)) {
                        rt_thddog_resume();
                        if (CC_BJDC_PT_VERIFY == xData.eType && CC_BJDC_VERIFY_RESULT == xData.xData.xVerify.btState) {
                            if (xData.xData.xVerify.xParam.bPass) {
                                s_eVerifyState[index] = CC_BJDC_VERIFY_PASS;
                                break;
                            }
                            rt_thddog_suspend("cc_net_disconnect");
                            cc_net_disconnect(index);
                            rt_thddog_resume();
                            rt_thread_delay(5 * RT_TICK_PER_SECOND);
                            goto _START;
                        }
                    } else {
                        rt_thddog_resume();
                    }
                }
            } else {
                rt_thddog_suspend("cc_net_disconnect");
                cc_net_disconnect(index);
                rt_thddog_resume();
                rt_thread_delay(5 * RT_TICK_PER_SECOND);
                goto _START;
            }
        }
        s_cc_reinit_flag[index][0] = RT_FALSE;

        rt_kprintf("...verify ok !\n");
        {
            rt_thddog_suspend("cc_bjdc_deviceinfo_req");
            cc_bjdc_deviceinfo_req(
                index, building_id, gateway_id,
                build_name, build_no, dev_no,
                g_sys_info.SN, 
                g_sys_info.HW_VER, 
                g_sys_info.SW_VER,
                pcfg->peer, pcfg->port,
                pcfg->peer, pcfg->port,
                dev_num, period,
                address
                );
            rt_thddog_resume();
        }
        ulHeartbeatTick = rt_tick_get();
//      for (int i = 0; i < relate->n; i++) {
//          relate->val_avg[i] = 0; relate->val_cur[i] = 0;
//      }
//      relate->count = 0; relate->time = rt_tick_get();
        while (!s_cc_reinit_flag[index][0]) {
            ExtData_t *node = RT_NULL;
            rt_thddog_feed("");
            rt_enter_critical();
            {
                while (!s_cc_reinit_flag[index][0]) {
                    node = pVarManage_GetExtDataWithUpProto(node, PROTO_DEV_NET_TYPE(index), PROTO_DEV_NET_INDEX(index), PROTO_CC_BJDC);
                    if (node) {
                        var_double_t ext_value = 0;
                        if (node->xUp.bEnable) {
                            if (bVarManage_GetExtValue(node, node->xIo.btOutVarType, &ext_value)) {
                                node->xUp.xAvgUp.val_avg += ext_value;
                                node->xUp.xAvgUp.val_cur = ext_value;
                                node->xUp.xAvgUp.count++;
                            }
                        }
                    } else {
                        break;
                    }
                }
            }
            rt_exit_critical();

            rt_thddog_suspend("rt_mq_recv wait");
            if (RT_EOK == rt_mq_recv(s_CCDataQueue[index], &xData, sizeof(CC_BJDC_Data_t), 2 * RT_TICK_PER_SECOND)) {
                switch (xData.eType) {
                case CC_BJDC_PT_DEVINFO:
                {
                    rt_kprintf("recv deviceinfo rsp !\n");
                    break;
                }

                case CC_BJDC_PT_HEARTBEAT:
                {
                    rt_kprintf("recv heartbeat rsp, time:%s\n", xData.xData.xHeartbeat.szDate);
                    ulHeartbeatTick = rt_tick_get();
                    break;
                }

                case CC_BJDC_PT_PERIOD_CFG:
                {
                    rt_kprintf("recv period cfg !\n");
                    period = xData.xData.xPeriod.ulPeriod;
                    if (period > 0) {
                        vVarManage_SetExtDataUpInterval(PROTO_DEV_NET_TYPE(index), PROTO_DEV_NET_INDEX(index), period * 60);
                    }
                    break;
                }

                case CC_BJDC_PT_RESTART:
                {
                    rt_kprintf("recv restart msg\n");
                    vDoSystemReset();
                    break;
                }
                case CC_BJDC_PT_DATA_QUERY:
                {
                    rt_kprintf("recv data query msg\n");
                    mxml_node_t *xml = cc_bjdc_data_create_reply_or_report_head(index, building_id, gateway_id, 1, RT_FALSE);
                    int total_size = 256 + strlen(building_id) + strlen(gateway_id);
                    
                    rt_thddog_suspend("cc_bjdc_data_add_data");
                    rt_enter_critical();
                    {
                        node = RT_NULL;
                        while (!s_cc_reinit_flag[index][0]) {
                            node = pVarManage_GetExtDataWithUpProto(node, PROTO_DEV_NET_TYPE(index), PROTO_DEV_NET_INDEX(index), PROTO_CC_BJDC);
                            if (node) {
                                var_double_t ext_value = 0;
                                if (node->xUp.bEnable) {
                                    total_size += cc_bjdc_data_add_data(xml, EXT_DATA_GET_NID(node->xUp.szNid), node->xUp.szFid, node->xUp.xAvgUp.val_cur);
                                }
                            } else {
                                break;
                            }
                        }
                    }
                    rt_exit_critical();
                    
                    rt_thddog_suspend("cc_bjdc_send_xml");
                    cc_bjdc_send_xml(index, xml, total_size);
                    break;
                }
                }
            }
            rt_thddog_resume();

            if (rt_tick_get() - ulHeartbeatTick >= rt_tick_from_millisecond(20 * 1000)) {
                rt_thddog_suspend("cc_bjdc_heartbeat_req");
                cc_bjdc_heartbeat_req(index, building_id, gateway_id);
                rt_thddog_resume();
            }

            {
                up_interval = lVarManage_GetExtDataUpInterval(PROTO_DEV_NET_TYPE(index), PROTO_DEV_NET_INDEX(index));
                if (up_interval > 0 && up_interval < 180 * 60) {
                    mxml_node_t *xml = VAR_NULL;
                    int total_size = 256 + strlen(building_id) + strlen(gateway_id);;
                    node = RT_NULL;
                    rt_thddog_suspend("cc_bjdc_data_add_data up_interval");
                    rt_enter_critical();
                    {
                        while (!s_cc_reinit_flag[index][0]) {
                            node = pVarManage_GetExtDataWithUpProto(node, PROTO_DEV_NET_TYPE(index), PROTO_DEV_NET_INDEX(index), PROTO_CC_BJDC);
                            if (node) {
                                var_double_t ext_value = 0;
                                if (node->xUp.bEnable) {
                                    if (up_interval > 0 && rt_tick_get() - node->xUp.xAvgUp.time >= rt_tick_from_millisecond(up_interval * 1000)) {
                                        if (node->xUp.xAvgUp.count > 0) {
                                            var_double_t value = (node->xUp.xAvgUp.val_avg / node->xUp.xAvgUp.count);
                                            if (VAR_NULL == xml) {
                                                xml = cc_bjdc_data_create_reply_or_report_head(index, building_id, gateway_id, 1, RT_TRUE);
                                            }
                                            total_size += cc_bjdc_data_add_data(xml, EXT_DATA_GET_NID(node->xUp.szNid), node->xUp.szFid, value);
                                            node->xUp.xAvgUp.val_avg = 0;
                                            node->xUp.xAvgUp.count = 0;
                                            node->xUp.xAvgUp.time = rt_tick_get();
                                        }
                                    }
                                }
                            } else {
                                break;
                            }
                        }
                    }
                    rt_exit_critical();
                    if (xml) {
                        rt_thddog_suspend("cc_bjdc_send_xml up_interval");
                        cc_bjdc_send_xml(index, xml, total_size);
                    }
                    rt_thddog_resume();
                }
            }
            rt_thddog_feed("");
        }
        if (s_cc_reinit_flag[index][0]) goto _START;
    }

    s_bjdc_work_thread[index] = RT_NULL;
    rt_thddog_unreg_inthd();
}

rt_bool_t cc_bjdc_send_xmlpack(rt_uint8_t index, const char *xml)
{
//  rt_kprintf("----------send:----------\n");
//  rt_kprintf("%s\n", xml);
//  rt_kprintf("-------------------------\n");
    if (xml != RT_NULL) {
        rt_uint32_t ulLen = (4 + RT_ALIGN(strlen(xml), 16));
        CC_BJDC_Package_t xPack = {
            .ulPre = CC_BJDC_PRE,
            .ulLen = cc_bjdc_htonl(ulLen),
            .xMsg.ulType = 0,
            .xMsg.pData = RT_NULL,
            .usCheck = 0,
            .ulEom = CC_BJDC_EOM

        };
        unsigned char *buf = rt_calloc(1, ulLen + 14);
        if (buf != RT_NULL) {
            unsigned char *pData = buf;
            memcpy(pData, &xPack, 12); pData += 12;
            strcpy(pData, xml);
            AES_Encrypt((const unsigned char *)pData, (unsigned char *)pData, ulLen - 4, c_xAESKeyAndIV);
            pData += (ulLen - 4);
            xPack.usCheck = CC_GetCrc16(&buf[8], ulLen);
            xPack.usCheck = cc_bjdc_htons(xPack.usCheck);
            memcpy(pData, &xPack.usCheck, 6); pData += 6;
            cc_net_send(index, 0, (const rt_uint8_t *)buf, ulLen + 14);
            rt_free(buf);

            return RT_TRUE;
        }
    }

    return RT_FALSE;
}

mxml_node_t* __create_common(mxml_node_t *root, const char *type, const char *building_id, const char *gateway_id)
{
    mxml_node_t *common;
    mxml_node_t *node;

    common = mxmlNewElement(root, "common");
    mxmlNewText(mxmlNewElement(common, "building_id"), 0, building_id);
    mxmlNewText(mxmlNewElement(common, "gateway_id"), 0, gateway_id);
    mxmlNewText(mxmlNewElement(common, "type"), 0, type);

    return common;
}

rt_bool_t cc_bjdc_verify_req(rt_uint8_t index, const char *building_id, const char *gateway_id)
{
#undef _CC_BUF_SZ
#define _CC_BUF_SZ     (512)
    mxml_node_t *xml;
    mxml_node_t *root;
    mxml_node_t *common;
    mxml_node_t *node;
    mxml_node_t *id_validate;

    xml = mxmlNewXML("1.0");
    root = mxmlNewElement(xml, "root");
    common = __create_common(root, "request", building_id, gateway_id);

    id_validate = mxmlNewElement(root, "id_validate");
    mxmlElementSetAttr(id_validate, "operation", "request");

    {
        char *xmlbuf = rt_malloc(_CC_BUF_SZ);
        int nLen = mxmlSaveString(xml, xmlbuf, _CC_BUF_SZ, MXML_NO_CALLBACK);
        cc_bjdc_send_xmlpack(index, xmlbuf);
        rt_free(xmlbuf);
    }
    mxmlDelete(xml);

    return RT_TRUE;
}

rt_bool_t cc_bjdc_checkmd5_req(rt_uint8_t index, const char *building_id, const char *gateway_id, const char *md5)
{
#undef _CC_BUF_SZ
#define _CC_BUF_SZ     (512)
    mxml_node_t *xml;
    mxml_node_t *root;
    mxml_node_t *common;
    mxml_node_t *node;
    mxml_node_t *id_validate;

    xml = mxmlNewXML("1.0");
    root = mxmlNewElement(xml, "root");
    common = __create_common(root, "md5", building_id, gateway_id);

    id_validate = mxmlNewElement(root, "id_validate");
    mxmlElementSetAttr(id_validate, "operation", "md5");
    mxmlNewText(mxmlNewElement(id_validate, "md5"), 0, md5);

    {
        char *xmlbuf = rt_malloc(_CC_BUF_SZ);
        int nLen = mxmlSaveString(xml, xmlbuf, _CC_BUF_SZ, MXML_NO_CALLBACK);
        cc_bjdc_send_xmlpack(index, xmlbuf);
        rt_free(xmlbuf);
    }
    mxmlDelete(xml);

    return RT_TRUE;
}

rt_bool_t cc_bjdc_deviceinfo_req
(
    rt_uint8_t      index,
    const char      *building_id,
    const char      *gateway_id,
    const char      *build_name,
    const char      *build_no,
    const char      *dev_no,
    const char      *factory,
    const char      *hardware,
    const char      *software,
    const char      *server,
    const int        port,
    const char      *host,
    const int        com,
    const int        dev_num,
    const int        period,
    const char      *address
    )
{
#undef _CC_BUF_SZ
#define _CC_BUF_SZ     (1024)
    mxml_node_t *xml;
    mxml_node_t *root;
    mxml_node_t *common;
    mxml_node_t *node;
    mxml_node_t *device;
    char buf[18];

    xml = mxmlNewXML("1.0");
    root = mxmlNewElement(xml, "root");
    common = __create_common(root, "device", building_id, gateway_id);

    device = mxmlNewElement(root, "device");
    mxmlElementSetAttr(device, "operation", "device");
    mxmlNewText(mxmlNewElement(device, "build_name"), 0, build_name);
    mxmlNewText(mxmlNewElement(device, "build_no"), 0, build_no);
    mxmlNewText(mxmlNewElement(device, "dev_no"), 0, dev_no);
    mxmlNewText(mxmlNewElement(device, "factory"), 0, factory);
    mxmlNewText(mxmlNewElement(device, "hardware"), 0, hardware);
    mxmlNewText(mxmlNewElement(device, "software"), 0, software);

    {
        struct das_net_list_node net;
        const char *interface_name = das_do_get_net_driver_name(DAS_NET_TYPE_ETH, 0);
        memset(&net, 0, sizeof(net));
        das_do_get_net_info(DAS_NET_TYPE_ETH, 0, &net);
        if (!net.DHCP) strcpy(net.GATEWAY, g_net_cfg.gw);
        mxmlNewText(mxmlNewElement(device, "mac"), 0, net.MAC);
        mxmlNewText(mxmlNewElement(device, "ip"), 0, net.IP);
        mxmlNewText(mxmlNewElement(device, "mask"), 0, net.MASK);
        mxmlNewText(mxmlNewElement(device, "gate"), 0, net.GATEWAY);
    }

    mxmlNewText(mxmlNewElement(device, "server"), 0, server);
    mxmlNewInteger(mxmlNewElement(device, "port"), port);
    mxmlNewText(mxmlNewElement(device, "host"), 0, host);
    mxmlNewInteger(mxmlNewElement(device, "com"), com);
    mxmlNewInteger(mxmlNewElement(device, "dev_num"), dev_num);
    mxmlNewInteger(mxmlNewElement(device, "period"), period);
    {
        struct tm lt;
        rt_time_t t = time(NULL);
        char date[16] = { 0 };
        das_localtime_r(&t, &lt);
        rt_sprintf(date, "%04d%02d%02d%02d%02d%02d",
                   lt.tm_year + 1900,
                   lt.tm_mon + 1,
                   lt.tm_mday,
                   lt.tm_hour,
                   lt.tm_min,
                   lt.tm_sec
                  );
        mxmlNewText(mxmlNewElement(device, "begin_time"), 0, date);
    }
    mxmlNewText(mxmlNewElement(device, "address"), 0, address);

    {
        char *xmlbuf = rt_malloc(_CC_BUF_SZ);
        int nLen = mxmlSaveString(xml, xmlbuf, _CC_BUF_SZ, MXML_NO_CALLBACK);
        cc_bjdc_send_xmlpack(index, xmlbuf);
        rt_free(xmlbuf);
    }
    mxmlDelete(xml);

    return RT_TRUE;
}

rt_bool_t cc_bjdc_heartbeat_req(rt_uint8_t index, const char *building_id, const char *gateway_id)
{
#undef _CC_BUF_SZ
#define _CC_BUF_SZ     (256)
    mxml_node_t *xml;
    mxml_node_t *root;
    mxml_node_t *common;
    mxml_node_t *heart_beat;

    xml = mxmlNewXML("1.0");
    root = mxmlNewElement(xml, "root");
    common = __create_common(root, "notify", building_id, gateway_id);

    heart_beat = mxmlNewElement(root, "heart_beat");
    mxmlElementSetAttr(heart_beat, "operation", "notify");
    mxmlNewText(mxmlNewElement(heart_beat, "notify"), 0, "master");

    {
        char *xmlbuf = rt_malloc(_CC_BUF_SZ);
        int nLen = mxmlSaveString(xml, xmlbuf, _CC_BUF_SZ, MXML_NO_CALLBACK);
        cc_bjdc_send_xmlpack(index, xmlbuf);
        rt_free(xmlbuf);
    }
    mxmlDelete(xml);

    return RT_TRUE;
}

rt_bool_t cc_bjdc_period_rsp(rt_uint8_t index, const char *building_id, const char *gateway_id, rt_bool_t bPass)
{
#undef _CC_BUF_SZ
#define _CC_BUF_SZ     (256)
    mxml_node_t *xml;
    mxml_node_t *root;
    mxml_node_t *common;
    mxml_node_t *config;

    xml = mxmlNewXML("1.0");
    root = mxmlNewElement(xml, "root");
    common = __create_common(root, "period_ack", building_id, gateway_id);

    config = mxmlNewElement(root, "config");
    mxmlElementSetAttr(config, "operation", "period_ack");
    mxmlNewText(mxmlNewElement(config, "period_ack"), 0, bPass ? "pass" : "fail");

    {
        char *xmlbuf = rt_malloc(_CC_BUF_SZ);
        int nLen = mxmlSaveString(xml, xmlbuf, _CC_BUF_SZ, MXML_NO_CALLBACK);
        cc_bjdc_send_xmlpack(index, xmlbuf);
        rt_free(xmlbuf);
    }
    mxmlDelete(xml);

    return RT_TRUE;
}

mxml_node_t *cc_bjdc_data_create_reply_or_report_head(
    rt_uint8_t index,
    const char *building_id,
    const char *gateway_id,
    rt_int32_t  sequence,
    rt_bool_t   report
    )
{
    mxml_node_t *xml;
    mxml_node_t *root;
    mxml_node_t *common;
    mxml_node_t *data;

    xml = mxmlNewXML("1.0");
    root = mxmlNewElement(xml, "root");
    common = __create_common(root, report ? "report" : "reply", building_id, gateway_id);

    data = mxmlNewElement(root, "data");
    mxmlElementSetAttr(data, "operation", report ? "report" : "reply");

    mxmlNewInteger(mxmlNewElement(data, "sequence"), sequence);
    mxmlNewText(mxmlNewElement(data, "parser"), 0, "yes");

    {
        struct tm lt;
        rt_time_t t = time(NULL);
        char date[16] = { 0 };
        das_localtime_r(&t, &lt);
        rt_sprintf(date, "%04d%02d%02d%02d%02d%02d",
                   lt.tm_year + 1900,
                   lt.tm_mon + 1,
                   lt.tm_mday,
                   lt.tm_hour,
                   lt.tm_min,
                   lt.tm_sec
                  );
        mxmlNewText(mxmlNewElement(data, "time"), 0, date);
    }

    return xml;
}

int cc_bjdc_data_add_data(
    mxml_node_t  *xml,
    const char  *meter_id,
    const char  *func_idex,
    double      value
    )
{
    int size = 0;
    mxml_node_t *root;
    mxml_node_t *data;
    mxml_node_t *meter;
    mxml_node_t *function;
    mxml_node_t *functionex;

    root = mxmlFindElement(xml, xml, "root", NULL, NULL, MXML_DESCEND);
    data = mxmlFindElement(root, root, "data", NULL, NULL, MXML_DESCEND);

    if (meter_id && func_idex) {
        // find the same meter with id
        meter = data;
        while (1) {
            meter = mxmlFindElement(meter, data, "meter", NULL, NULL, MXML_DESCEND);
            if (meter) {
                const char *id = mxmlElementGetAttr(meter, "id");
                if (0 == strcmp(id, meter_id)) {
                    break;
                }
            } else {
                break;
            }
        }

        if (!meter) {
            meter = mxmlNewElement(data, "meter");
            mxmlElementSetAttr(meter, "id", meter_id);
            size += (32 + strlen(meter_id));
            //mxmlElementSetAttr(meter, "addr", meter_addr);
            //mxmlElementSetAttr(meter, "tp", meter_tp);
            //mxmlElementSetAttr(meter, "name", meter_name);
        }

        /*function = mxmlNewElement(meter, "function");
        mxmlElementSetAttr(function, "id", function_id);
        mxmlElementSetAttr(function, "coding", function_coding);
        mxmlElementSetAttr(function, "error", "192");
        mxmlNewReal(function, value);*/

        functionex = mxmlNewElement(meter, "functionex");
        mxmlElementSetAttr(functionex, "idex", func_idex);
        //mxmlElementSetAttr(functionex, "tpex", func_tpex);
        //mxmlElementSetAttr(functionex, "equipidex", func_equipidex);
        mxmlNewReal(functionex, value);
        size += (32 + strlen(func_idex) + 16);
    }
    return size;
}

static void cc_bjdc_send_xml(rt_uint8_t index, mxml_node_t *xml, int size)
{
    char *xmlbuf = rt_malloc(size);
    int nLen = mxmlSaveString(xml, xmlbuf, size, MXML_NO_CALLBACK);
    cc_bjdc_send_xmlpack(index, xmlbuf);
    rt_free(xmlbuf);
    mxmlDelete(xml);
}

rt_bool_t cc_bjdc_data_continuous(rt_uint8_t index, const char *building_id, const char *gateway_id, rt_bool_t bPass)
{

}

