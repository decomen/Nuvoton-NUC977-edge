

/*
 * File      : webnet.c
 * This file is part of RT-Thread RTOS/WebNet Server
 * COPYRIGHT (C) 2011, Shanghai Real-Thread Technology Co., Ltd
 *
 * All rights reserved.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-08-02     Bernard      the first version
 */

#include "webnet.h"
#include "session.h"
#include "module.h"

#include "board.h"
#include "board_cgi.c"

DEF_CGI_HANDLER(diskFormat);

DEF_CGI_HANDLER(devReset);

DEF_CGI_HANDLER(factoryReset);

DEF_CGI_HANDLER(getDevInfo);
DEF_CGI_HANDLER(setDevInfo);

DEF_CGI_HANDLER(setDefaultMac);
DEF_CGI_HANDLER(setTime);

DEF_CGI_HANDLER(getNetInfo);
DEF_CGI_HANDLER(getNetCfg);
DEF_CGI_HANDLER(setNetCfg);
DEF_CGI_HANDLER(getAllNetInfo);

DEF_CGI_HANDLER(getTcpipCfg);
DEF_CGI_HANDLER(setTcpipCfg);

DEF_CGI_HANDLER(getProtoManage);

DEF_CGI_HANDLER(setUartCfg);
DEF_CGI_HANDLER(getUartCfg);

DEF_CGI_HANDLER(getVarManageExtData);
DEF_CGI_HANDLER(setVarManageExtData);
DEF_CGI_HANDLER(delVarManageExtData);
DEF_CGI_HANDLER(getVarManageExtDataVals);

DEF_CGI_HANDLER(searchVarManageExtGroupStart);
DEF_CGI_HANDLER(searchVarManageExtGroupStatus);
DEF_CGI_HANDLER(searchVarManageExtGroupResult);
DEF_CGI_HANDLER(addVarManageExtGroup);
DEF_CGI_HANDLER(delVarManageExtGroup);
DEF_CGI_HANDLER(setVarManageDoValue);
DEF_CGI_HANDLER(getVarManageAIValue);

DEF_CGI_HANDLER(setGPRSNetCfg);
DEF_CGI_HANDLER(getGPRSNetCfg);

DEF_CGI_HANDLER(setGPRSWorkCfg);
DEF_CGI_HANDLER(getGPRSWorkCfg);

DEF_CGI_HANDLER(getCpuUsage);
DEF_CGI_HANDLER(getGPRSState);
DEF_CGI_HANDLER(getTcpipState);

DEF_CGI_HANDLER(getAuthCfg);
DEF_CGI_HANDLER(setAuthCfg);

DEF_CGI_HANDLER(getStorageCfg);
DEF_CGI_HANDLER(setStorageCfg);

DEF_CGI_HANDLER(getHostCfg);
DEF_CGI_HANDLER(setHostCfg);

DEF_CGI_HANDLER(getZigbeeCfg);
DEF_CGI_HANDLER(setZigbeeCfg);
DEF_CGI_HANDLER(doZigbeeLearnNow);
DEF_CGI_HANDLER(getZigbeeList);

DEF_CGI_HANDLER(getLoRaCfg);
DEF_CGI_HANDLER(setLoRaCfg);
DEF_CGI_HANDLER(doLoRaLearnNow);
DEF_CGI_HANDLER(getLoRaList);

DEF_CGI_HANDLER(setXferUartCfg);
DEF_CGI_HANDLER(getXferUartCfg);

DEF_CGI_HANDLER(setWebsocketCfg);


DEF_CGI_HANDLER(reggetid);
DEF_CGI_HANDLER(doreg);

DEF_CGI_HANDLER(listDir);
DEF_CGI_HANDLER(delFile);

DEF_CGI_HANDLER(getLogData);
DEF_CGI_HANDLER(delLogData);

DEF_CGI_HANDLER(saveCfgWithJson);

DEF_CGI_HANDLER(getRuleList);
DEF_CGI_HANDLER(setRule);
DEF_CGI_HANDLER(addRule);
DEF_CGI_HANDLER(delRule);

DEF_CGI_HANDLER(get_monitor_cfg);
DEF_CGI_HANDLER(set_monitor_cfg);

static int s_webnet_listenfd = -1;
static int s_webnet_stop = 0;

/**
 * webnet thread entry
 */
static void webnet_thread(void *parameter)
{
    while (1) {
        fd_set readset, tempfds;
        fd_set writeset;
        int i, maxfdp1;
        struct sockaddr_in webnet_saddr;
        int on = 1;

        rt_thddog_feed("webnet_init");
        /* First acquire our socket for listening for connections */
        s_webnet_listenfd = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        LWIP_ASSERT("webnet_thread(): Socket create failed.", s_webnet_listenfd >= 0);

        /*
        const char *intfc_name = das_do_get_net_driver_name(DAS_NET_TYPE_ETH, 0);
        if (intfc_name && intfc_name[0]) {
            struct ifreq intfc; memset(&intfc, 0, sizeof(intfc));
            strncpy(intfc.ifr_ifrn.ifrn_name, intfc_name, strlen(intfc_name));  
            if (setsockopt(s_webnet_listenfd, SOL_SOCKET, SO_BINDTODEVICE, (const void *)&intfc, sizeof(intfc)) < 0) {
                goto _END;
            }
        }*/
        
        memset(&webnet_saddr, 0, sizeof(webnet_saddr));
        webnet_saddr.sin_family = AF_INET;
        webnet_saddr.sin_addr.s_addr = htonl(INADDR_ANY);
        webnet_saddr.sin_port = htons(WEBNET_PORT);     /* webnet server port */

        on = 1;
        if (lwip_setsockopt(s_webnet_listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
            LWIP_ASSERT("webnet_thread(): socket SO_REUSEADDR failed.", 0);
            goto _END;
        }

        if (lwip_bind(s_webnet_listenfd, (struct sockaddr *)&webnet_saddr, sizeof(webnet_saddr)) == -1) {
            LWIP_ASSERT("webnet_thread(): socket bind failed.", 0);
            perror("socket bind failed:");
            goto _END;
        }

        /* Put socket into listening mode */
        if (lwip_listen(s_webnet_listenfd, MAX_SERV) == -1) {
            LWIP_ASSERT("webnet_thread(): listen failed.", 0);
            goto _END;
        }

        /* initalize module (no session at present) */
        rt_thddog_feed("webnet_module_handle_event");
        webnet_module_handle_event(RT_NULL, WEBNET_EVENT_INIT);

        webnet_auth_set(g_auth_cfg.szUser, g_auth_cfg.szPsk);

        webnet_cgi_set_root("cgi-bin");

        webnet_cgi_register("reggetid", reggetid);
        webnet_cgi_register("doreg", doreg);

        webnet_cgi_register("diskFormat", diskFormat);

        webnet_cgi_register("devReset", devReset);

        webnet_cgi_register("factoryReset", factoryReset);

        webnet_cgi_register("getDevInfo", getDevInfo);
        //webnet_cgi_register("setDevInfo", setDevInfo);
        //webnet_cgi_register("setDefaultMac", setDefaultMac);
        webnet_cgi_register("setTime", setTime);

        webnet_cgi_register("getNetInfo", getNetInfo);
        webnet_cgi_register("getAllNetInfo", getAllNetInfo);

        webnet_cgi_register("getNetCfg", getNetCfg);
        webnet_cgi_register("setNetCfg", setNetCfg);

        webnet_cgi_register("getTcpipCfg", getTcpipCfg);
        webnet_cgi_register("setTcpipCfg", setTcpipCfg);


        webnet_cgi_register("getProtoManage", getProtoManage);

        webnet_cgi_register("setUartCfg", setUartCfg);
        webnet_cgi_register("getUartCfg", getUartCfg);

        webnet_cgi_register("getVarManageExtData", getVarManageExtData);
        webnet_cgi_register("setVarManageExtData", setVarManageExtData);
        webnet_cgi_register("delVarManageExtData", delVarManageExtData);
        webnet_cgi_register("getVarManageExtDataVals", getVarManageExtDataVals);
        webnet_cgi_register("searchVarManageExtGroupStart", searchVarManageExtGroupStart);
        webnet_cgi_register("searchVarManageExtGroupStatus", searchVarManageExtGroupStatus);
        webnet_cgi_register("searchVarManageExtGroupResult", searchVarManageExtGroupResult);
        webnet_cgi_register("addVarManageExtGroup", addVarManageExtGroup);
        webnet_cgi_register("delVarManageExtGroup", delVarManageExtGroup);
        webnet_cgi_register("setVarManageDoValue", setVarManageDoValue);
        webnet_cgi_register("getVarManageAIValue", getVarManageAIValue);

#if NET_HAS_GPRS
        webnet_cgi_register("setGPRSNetCfg", setGPRSNetCfg);
        webnet_cgi_register("getGPRSNetCfg", getGPRSNetCfg);

        webnet_cgi_register("setGPRSWorkCfg", setGPRSWorkCfg);
        webnet_cgi_register("getGPRSWorkCfg", getGPRSWorkCfg);
        
        webnet_cgi_register("getGPRSState", getGPRSState);
#endif
        webnet_cgi_register("getCpuUsage", getCpuUsage);


        webnet_cgi_register("getTcpipState", getTcpipState);

        webnet_cgi_register("getAuthCfg", getAuthCfg);
        webnet_cgi_register("setAuthCfg", setAuthCfg);

        webnet_cgi_register("getStorageCfg", getStorageCfg);
        webnet_cgi_register("setStorageCfg", setStorageCfg);

        webnet_cgi_register("getHostCfg", getHostCfg);
        webnet_cgi_register("setHostCfg", setHostCfg);

        /*webnet_cgi_register("getZigbeeCfg", getZigbeeCfg);
        webnet_cgi_register("setZigbeeCfg", setZigbeeCfg);
        webnet_cgi_register("doZigbeeLearnNow", doZigbeeLearnNow);

        webnet_cgi_register("getZigbeeList", getZigbeeList);*/
        
        webnet_cgi_register("getLoRaCfg", getLoRaCfg);
        webnet_cgi_register("setLoRaCfg", setLoRaCfg);
        webnet_cgi_register("doLoRaLearnNow", doLoRaLearnNow);

        webnet_cgi_register("getLoRaList", getLoRaList);

        webnet_cgi_register("getXferUartCfg", getXferUartCfg);
        webnet_cgi_register("setXferUartCfg", setXferUartCfg);

        webnet_cgi_register("setWebsocketCfg", setWebsocketCfg);
        
        webnet_cgi_register("listDir", listDir);
        webnet_cgi_register("delFile", delFile);
        
        webnet_cgi_register("getLogData", getLogData);
        webnet_cgi_register("delLogData", delLogData);
        
        webnet_cgi_register("saveCfgWithJson", saveCfgWithJson);
        
        webnet_cgi_register("getRuleList", getRuleList);
        webnet_cgi_register("setRule", setRule);
        webnet_cgi_register("addRule", addRule);
        webnet_cgi_register("delRule", delRule);
        
        webnet_cgi_register("set_monitor_cfg", set_monitor_cfg);
        webnet_cgi_register("get_monitor_cfg", get_monitor_cfg);

        /* Wait forever for network input: This could be connections or data */
        for (;;) {
            /* Determine what sockets need to be in readset */
            FD_ZERO(&readset);
            FD_ZERO(&writeset);
            FD_SET(s_webnet_listenfd, &readset);

            /* set fds in each sessions */
            maxfdp1 = webnet_sessions_set_fds(&readset, &writeset);
            if (maxfdp1 < s_webnet_listenfd + 1) maxfdp1 = s_webnet_listenfd + 1;

            /* use temporary fd set in select */
            tempfds = readset;
            /* Wait for data or a new connection */
            rt_thddog_suspend("main lwip_select");
            i = lwip_select(maxfdp1, &tempfds, 0, 0, 0);
            rt_thddog_resume();
            if (i == 0) continue;

            /* At least one descriptor is ready */
            if (FD_ISSET(s_webnet_listenfd, &tempfds)) {
                struct webnet_session *accept_session;
                /* We have a new connection request */
                accept_session = webnet_session_create(s_webnet_listenfd);

                if (accept_session == RT_NULL) {
                    /* create session failed, just accept and then close */
                    int sock;
                    struct sockaddr cliaddr;
                    socklen_t clilen;

                    clilen = sizeof(struct sockaddr_in);
                    sock = lwip_accept(s_webnet_listenfd, &cliaddr, &clilen);
                    if (sock >= 0) {
				        lwip_shutdown(sock, SHUT_RDWR);
                        lwip_close(sock);
			        }
                } else {

                    /* add read fdset */
                    FD_SET(accept_session->socket, &readset);
                }
            }

            rt_thddog_feed("webnet_sessions_handle_fds");
            webnet_sessions_handle_fds(&tempfds, &writeset);
        }

    _END:
        if (s_webnet_listenfd >= 0) {
            shutdown(s_webnet_listenfd, SHUT_RDWR);
            close(s_webnet_listenfd);
            s_webnet_listenfd = -1;
        }
        if (s_webnet_stop) break;
        sleep(5);
    }
    rt_thddog_exit();
}

int das_romfs_web_init(const char *dir_path);


static const char *__gprs_type_string(void)
{
    vCheckCellNetType();
    
    if (g_xCellNetType == E_GPRS_M26) {
        return "GRPS";
    } else if (g_xCellNetType == E_4G_EC20) {
        return "LTE";
    } else if (g_xCellNetType == E_NBIOT_BC26) {
        return "NB-IOT";
    } else if (g_xCellNetType == E_AIR720) {
        return "LTE";
    } else if (g_xCellNetType == E_4G_EC200S) {
        return "LTE";
    }

    return "NONE";
}

static void __gprs_create_type_js(const char *root)
{
    char buffer[256] = {0};
    sprintf(buffer, "echo 'var GPRS_OR_NBIOT = \"%s\";' > %s/js/gprs_type.js", __gprs_type_string(), root);
    system(buffer);
}

void webnet_init(void)
{
    rt_thread_t tid;

    s_webnet_stop = 0;

    das_romfs_web_init(WEBNET_ROOT);
    __gprs_create_type_js(WEBNET_ROOT);

    tid = rt_thread_create(WEBNET_THREAD_NAME,
                           webnet_thread, RT_NULL,
                           WEBNET_THREAD_STACKSIZE, WEBNET_PRIORITY, 5);

    if (tid != RT_NULL) {
        rt_thddog_register(tid, 60);
        rt_thread_startup(tid);
    }
}

void webnet_exit(void)
{
    s_webnet_stop = 1;
    webnet_session_close_all();
    if (s_webnet_listenfd >= 0) {
        shutdown(s_webnet_listenfd, SHUT_RDWR);
        close(s_webnet_listenfd);
    }
}

