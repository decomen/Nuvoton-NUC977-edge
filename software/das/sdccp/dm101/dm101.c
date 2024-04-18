#include "board.h"
#include "dm_lib.h"
#include "os_platform.h"
#include "sdccp_net.h"

#include "dm101.h"
#include "bfifo.h"

#include "net_helper.h"

static struct dm101_cfg     *s_dm101_cfg_data[BOARD_TCPIP_MAX];
static rt_thread_t          s_dm101_work_thread[BOARD_TCPIP_MAX];
static struct bfifo        *s_dm101_fifo[BOARD_TCPIP_MAX];

void showtime(void)
{
    time_t now;
    struct tm tm_now;
         
    time(&now);
    das_localtime_r(&now, &tm_now);
                  
    dm101_debug("now datetime: %d-%d-%d %d:%d:%d\n", tm_now.tm_year+1900, tm_now.tm_mon+1, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
    
    return;
}

rt_err_t dm101_put_bytes(rt_uint8_t index, rt_uint8_t *buffer, rt_uint16_t len)
{
    if (s_dm101_fifo[index]) {
        int ofs = 0;
        while (len > 0) {
            int num = bfifo_push(s_dm101_fifo[index], &buffer[ofs], len, -1);
            if (num > 0) {
                len -= num;
                ofs += num;
            }
        }
    }
    return RT_EOK;
}

int64_t client_handle(struct Dm101_context *context)
{
    struct Dm101pkg dm101pkg;
    int32_t rsize;
    char *buf;
    
    memset(&dm101pkg, 0, sizeof(dm101pkg));
    
    rsize = read_pkg(context, &dm101pkg);   //接收消息
    if(rsize < 0){
        dm101_debug("read_pkg error\n");
        return -1;
    }

   // dm101_debug("recv\n");
  //  show_head(&(dm101pkg.head), NULL , NULL);
  //  dm101_debug("recv end\n");

    dm101_debug("type: 0x%x, msgcode: 0x%x, code: 0x%0x\n",dm101pkg.head.type, dm101pkg.head.msgcode,dm101pkg.head.code);

    /*if(dm101pkg.head.type != F_MSG_ACK){
        dm101_debug("sddccp error,type: %d", dm101pkg.head.type);
        return 0;
    }*/

    /*if(dm101pkg.head.code != CODE_STATUS_OK){
        dm101_debug("dm101 err code = 0x%x\r\n", dm101pkg.head.code);
        return 0;
    }*/

    
    switch(dm101pkg.head.msgcode) {
  
    case CODE_REQ_TIME: {

        if (dm101pkg.head.payload_length > 0 && dm101pkg.pload_buf) {
            dm101_debug("length = %d, pload_buf = %s\n",
                dm101pkg.head.payload_length, dm101pkg.pload_buf);
        }
       
        dm101_debug("CODE_REQ_TIME: type : 0x%x, code = 0x%x\r\n" , dm101pkg.head.type, dm101pkg.head.code);
        
        // {"time":1513071477}
        if (dm101pkg.head.payload_length > 0 && dm101pkg.pload_buf) {
            cJSON *json = cJSON_Parse(dm101pkg.pload_buf);
            if (json) {
                rt_uint32_t time = cJSON_GetDouble(json, "time", 0);
                if (time > 0) {
                    dm101_debug("$$$$$$$$$$$$$$$ set_timestamp : %u\n", (uint32_t)time);
                    das_set_time(time, 28800);
                    my_system("hwclock -w -u");
                }
                cJSON_Delete(json);
            }
        }
        break;
    }
    case CODE_SERVER_FIND: {
        dm101_debug("CODE_SERVER_FIND: type : 0x%x, code = 0x%x\r\n" , dm101pkg.head.type, dm101pkg.head.code);
        
        if (dm101pkg.head.payload_length > 0 && dm101pkg.pload_buf) {
            dm101_debug("length = %d, pload_buf = %s\n",
                dm101pkg.head.payload_length, dm101pkg.pload_buf);
        }
        
        // {"id":1,"name":"test","addr":"118.31.78.162","port":16969}

        break;
    }

    case CODE_REPORT_INFO: {
        dm101_debug("CODE_REPORT_INFO: type : 0x%x, code = 0x%x\r\n" , dm101pkg.head.type, dm101pkg.head.code);
        
        if (dm101pkg.head.payload_length > 0) {
            dm101_debug("length = %d, pload_buf = %s\n",
                dm101pkg.head.payload_length, dm101pkg.pload_buf);
        }
        
        break;
    }

    case CODE_REPORT_DATA:{
        dm101_debug("CODE_REPORT_DATA: type : 0x%x, code = 0x%x\r\n" , dm101pkg.head.type, dm101pkg.head.code);
        if (dm101pkg.head.payload_length > 0) {
            dm101_debug("length = %d, pload_buf = %s\n",
                dm101pkg.head.payload_length, dm101pkg.pload_buf);
        }
        break;
    }

     case CODE_HAERT:{
        if (dm101pkg.head.payload_length > 0) {
            dm101_debug("length = %d, pload_buf = %s\n",
                dm101pkg.head.payload_length, dm101pkg.pload_buf);
        }
        dm101_debug("CODE_HAERT: type : 0x%x, code = 0x%x\r\n" , dm101pkg.head.type, dm101pkg.head.code);
        break;
    }

    case CODE_SET_INFO:
        dm101_debug("CODE_SET_INFO: type : 0x%x, code = 0x%x\r\n" , dm101pkg.head.type, dm101pkg.head.code);
        if (dm101pkg.head.payload_length > 0) {
            dm101_debug("length = %d, pload_buf = %s\n", dm101pkg.head.payload_length, dm101pkg.pload_buf);
        }
        break;
    case CODE_UPDATE:{ //服务端下发升级包信息

        dm101_debug("CODE_UPDATE: type : 0x%x, code = 0x%x\r\n" , dm101pkg.head.type, dm101pkg.head.code);
        if (dm101pkg.head.payload_length > 0) {
            dm101_debug("length = %d, pload_buf = %s\n", dm101pkg.head.payload_length, dm101pkg.pload_buf);
        }
       

        if(dm101pkg.head.type == F_MSG_NEED_ACK){
            make_ack_pkg(context, &dm101pkg, CODE_STATUS_OK, NULL, 0);
            send_pkg(context, &dm101pkg);
        }
        break;
    }   
    case CODE_CHECK_VER:{ //设备端查询最新版本固件信息回复
        dm101_debug("CODE_CHECK_VER: type : 0x%x, code = 0x%x\r\n" , dm101pkg.head.type, dm101pkg.head.code);
        if (dm101pkg.head.payload_length > 0) {
            dm101_debug("length = %d, pload_buf = %s\n", dm101pkg.head.payload_length, dm101pkg.pload_buf);
        }
        break;
    }
    case CODE_DOWNLOAD_REC_FILE:{ //服务端下发源文件信息
        dm101_debug("CODE_DOWNLOAD_REC_FILE: type : 0x%x, code = 0x%x\r\n" , dm101pkg.head.type, dm101pkg.head.code); 
        if (dm101pkg.head.payload_length > 0) {
            dm101_debug("length = %d, pload_buf = %s\n", dm101pkg.head.payload_length, dm101pkg.pload_buf);
        }
        break;
    }
    case CODE_REMOTE_REBOOT:
        dm101_debug("CODE_REMOTE_REBOOT: type : 0x%x, code = 0x%x\r\n" , dm101pkg.head.type, dm101pkg.head.code); 
        if (dm101pkg.head.payload_length > 0) {
            dm101_debug("length = %d, pload_buf = %s\n", dm101pkg.head.payload_length, dm101pkg.pload_buf);
        }
        vDoSystemReset();
       
        break;
    case CODE_SYS_TIME:
        dm101_debug("CODE_SYS_TIME: type : 0x%x, code = 0x%x\r\n" , dm101pkg.head.type, dm101pkg.head.code);  
        if (dm101pkg.head.payload_length > 0) {
            dm101_debug("length = %d, pload_buf = %s\n", dm101pkg.head.payload_length, dm101pkg.pload_buf);
            cJSON *json = cJSON_Parse(dm101pkg.pload_buf);
            if (json) {
                rt_uint32_t time = cJSON_GetDouble(json, "time", 0);
                if (time > 0) {
                    dm101_debug("$$$$$$$$$$$$$$$ set_timestamp : %u\n", (uint32_t)time);
                    das_set_time(time, 28800);
                    my_system("hwclock -w -u");
                }
                cJSON_Delete(json);
            }
        }
        break;
    case CODE_SET_CONFIG:
    case CODE_GET_CONFIG:
    case CODE_SET_GROUP_DATA:
    case CODE_GET_GROUP_DATA:
    case CODE_SET_REPORT_RULE:
     
    default:
        //make_ack_pkg(context, &dm101pkg, CODE_STATUS_OK, NULL, 0);
        break;
    }

    //if(dm101pkg.head.type != F_MSG_ACK){
    //    make_ack_pkg(context, &dm101pkg, CODE_STATUS_OK, NULL, 0);
    //    send_pkg(context, &dm101pkg);
    //}
    
    return 0;
}

rt_bool_t dm101_open(rt_uint8_t index)
{
    dm101_close(index);

    s_cc_reinit_flag[index][0] = RT_FALSE;

    RT_KERNEL_FREE(s_dm101_cfg_data[index]);
    s_dm101_cfg_data[index] = RT_KERNEL_CALLOC(sizeof(struct dm101_cfg));

    RT_KERNEL_FREE(s_pCCBuffer[index][0]);

    *s_dm101_cfg_data[index] = c_dm101_default_cfg;
    {
        char buf[64] = "";
        ini_t *ini;
        sprintf(buf, DM101_INI_CFG_PATH_PREFIX "%d" ".ini", index);
        ini = ini_load(buf);

        if (ini) {
            const char *str = ini_getstring(ini, "common", "auth", "");
            if(str[0]) strncpy(s_dm101_cfg_data[index]->auth, str, sizeof(s_dm101_cfg_data[index]->auth));
            s_dm101_cfg_data[index]->crypt = ini_getint(ini, "common", "crypt", 0);
            s_dm101_cfg_data[index]->srdid = ini_getlonglong(ini, "common", "srdid", 0);
            
            ini_free(ini);
        }
    }
    if (s_dm101_fifo[index]) bfifo_destroy(s_dm101_fifo[index]);
    s_dm101_fifo[index] = bfifo_create(DM101_FIFO_SIZE);
    return RT_TRUE;
}

void dm101_close(rt_uint8_t index)
{
    for (int i = 0; i < 5 * 10; i++) {
        if (s_dm101_work_thread[index]) {
            rt_thread_delay(RT_TICK_PER_SECOND / 10);
        } else {
            break;
        }
    }

    dm101_exitwork(index);

    if (s_CCDataQueue[index]) {
        rt_mq_delete(s_CCDataQueue[index]);
        s_CCDataQueue[index] = RT_NULL;
    }
    RT_KERNEL_FREE(s_pCCBuffer[index][0]);
    s_pCCBuffer[index][0] = RT_NULL;
    if (s_dm101_fifo[index]) bfifo_destroy(s_dm101_fifo[index]);
    s_dm101_fifo[index] = NULL;
}

static void dm101_work_task(void *parameter);

void dm101_startwork(rt_uint8_t index)
{
    if(RT_NULL == s_dm101_work_thread[index]) {
        BOARD_CREAT_NAME(szWork, "dm101_%d", index);
        s_dm101_work_thread[index] = \
            rt_thread_create(szWork,
                                dm101_work_task,
                                (void *)(long)index,
                                2048,
                                20, 20);
        if (s_dm101_work_thread[index] != RT_NULL) {
            rt_thddog_register(s_dm101_work_thread[index], 30);
            rt_thread_startup(s_dm101_work_thread[index]);
        }
    }
}

void dm101_exitwork(rt_uint8_t index)
{
    s_cc_reinit_flag[index][0] = RT_TRUE;
}

s_Dm101Status_t g_xDm101Status[BOARD_TCPIP_MAX];
static rt_uint32_t g_Dm101LastSetTime = 0;
static void dm101_work_task(void *parameter)
{
#define _DATA_COUNT      (64)
    rt_uint8_t index = (int)(long)parameter;
    struct Dm101_context *context = NULL;
    int tmp;
    struct dm101_args dm101_args = {
        .index = index,
        .fifo = s_dm101_fifo[index]
    };
    struct Dm101pkg dm101pkg;
    
    struct Data *datas = rt_malloc(sizeof(struct Data) * _DATA_COUNT);
    if (NULL == datas) {
        elog_e("malloc", "no memory : size(%d)", sizeof(struct Data) * _DATA_COUNT);
        return ;
    }

    context = dm101_context_init(
                    s_dm101_cfg_data[index],
                    dm101_work_init,
                    &dm101_args,
                    dm101_work_reset,
                    dm101_work_close,
                    dm101_work_write,
                    dm101_work_read);
    
    dm101_debug("\t--> auth: %8s\n \t--> crypt: %d\n \t--> srcid: 0x%lx\n" , s_dm101_cfg_data[index]->auth, s_dm101_cfg_data[index]->crypt, s_dm101_cfg_data[index]->srdid);
    if(context == NULL){
      dm101_debug("dm101_context_init failed\r\n");
    }
    memset(&g_xDm101Status[index],0,sizeof(s_Dm101Status_t));
    
    while (context) {
        int32_t ret;
_START:
        rt_thddog_feed("net_waitconnect");
        net_waitconnect(index);
        {
            if(g_xDm101Status[index].eInitStatus == E_DM101_BEGAIN){
                dm101_debug("##################### send CODE_SERVER_FIND\r\n");
                dm101_make_pkg(context, &dm101pkg, CODE_SERVER_FIND, NULL, 0);
                send_pkg(context, &dm101pkg);
                g_xDm101Status[index].eInitStatus = E_DM101_REPORT_INFO;
                rt_thread_delay(1000);
            }else if(g_xDm101Status[index].eInitStatus == E_DM101_REPORT_INFO){
                dm101_debug("##################### send CODE_REPORT_INFO\r\n");
                dm101_make_pkg(context, &dm101pkg, CODE_REPORT_INFO, NULL, 0);
                send_pkg(context, &dm101pkg);
                g_xDm101Status[index].eInitStatus = E_DM101_SET_TIME;
                rt_thread_delay(1000);
            }else if(g_xDm101Status[index].eInitStatus == E_DM101_SET_TIME){
                dm101_debug("##################### send CODE_REQ_TIME\r\n");
                dm101_make_pkg(context, &dm101pkg, CODE_REQ_TIME, NULL, 0);
                send_pkg(context, &dm101pkg);
                g_xDm101Status[index].eInitStatus = E_DM101_REPORT_DATA;
                rt_thread_delay(1000);
                
            }

            {
                static int check_ver = 1;
                if(check_ver){
                    dm101_debug("##################### send CODE_CHECK_VER\r\n");
                    dm101_make_pkg(context, &dm101pkg, CODE_CHECK_VER, NULL, 0);
                    send_pkg(context, &dm101pkg);
                    rt_thread_delay(1000);
                    check_ver = 0;
                }
            }
           
            s_cc_reinit_flag[index][0] = RT_FALSE;
            
            g_xDm101Status[index].dm101_lastheart =  rt_tick_get();
            g_xDm101Status[index].dm101_lastrecv = g_xDm101Status[index].dm101_lastheart;
            g_Dm101LastSetTime =  rt_tick_get();
            showtime();
            while (!s_cc_reinit_flag[index][0] && net_isconnect(index)) {
                ExtData_t *node = RT_NULL;

                while (1) {
                    int count = bfifo_count(s_dm101_fifo[index]);
                    if (count >= HEAD_LEN) {
                        uint8_t _magic[MAGIC_LEN] = {0};
                        if (MAGIC_LEN == bfifo_peek(s_dm101_fifo[index], (unsigned char *)_magic, MAGIC_LEN)) {
                            if (memcmp(_magic, MAGIC, MAGIC_LEN) == 0) {
                                client_handle(context);
                                break;
                            } else {
                                if (bfifo_pull(s_dm101_fifo[index], (unsigned char *)_magic, 1, 0) != 1) {
                                    break;
                                }
                            }
                        } else {
                            break;
                        }
                    } else {
                        rt_thread_delay(RT_TICK_PER_SECOND / 100);
                        break;
                    }
                }

                if(g_xDm101Status[index].eInitStatus != E_DM101_REPORT_DATA) {
                    goto _START;
                }

                if(g_xDm101Status[index].eInitStatus == E_DM101_REPORT_DATA) {
                
                    rt_thddog_feed("UpdateAvgValue");
                    rt_enter_critical();
                    {
                        while (!s_cc_reinit_flag[index][0] && net_isconnect(index)) {
                            node = pVarManage_GetExtDataWithUpProto(node, PROTO_DEV_NET_TYPE(index), PROTO_DEV_NET_INDEX(index), PROTO_DM101);
                            if (node) {
                                var_double_t ext_value = 0;
                                if (node->xUp.bEnable) {
                                    if (bVarManage_GetExtValue(node, node->xIo.btOutVarType, &ext_value)) {
                                        bVarManage_UpdateAvgValue(&node->xUp.xAvgUp, ext_value);
                                    }
                                }
                            } else {
                                break;
                            }
                        }
                    }
                    rt_exit_critical();

                    unsigned int ulDiff =  rt_tick_get() - g_xDm101Status[index].dm101_lastheart;
                    if (ulDiff >= rt_tick_from_millisecond(30000)) {
                        dm101_debug("free 30 s send heart,diff: %d, index: %d,cnt: %d\n",ulDiff, index, (int)(g_xDm101Status[index].dm101_lastheart / 1000));
                        showtime();
                        dm101_make_pkg(context, &dm101pkg, CODE_HAERT, NULL, 0);
                        send_pkg(context, &dm101pkg);
                        g_xDm101Status[index].dm101_lastheart = rt_tick_get();
                    }

                    ulDiff =  rt_tick_get() - g_xDm101Status[index].dm101_lastrecv;
                    if(ulDiff >= rt_tick_from_millisecond(110000)){
                        dm101_debug("free 110s no recive data\r\n");
                        net_disconnect(index);
                    }

                    /*if(get_utc_time() - g_Dm101LastSetTime >= 85){
                            g_Dm101LastSetTime = get_utc_time();
                            rt_thread_delay(RT_TICK_PER_SECOND / 100);
                            dm101_debug("##################### send CODE_REQ_TIME\r\n"); 
                            dm101_make_pkg(context, &dm101pkg, CODE_REQ_TIME, NULL, 0);
                            send_pkg(context, &dm101pkg);
                    }*/

                    {
                        var_int32_t up_interval = lVarManage_GetExtDataUpInterval(PROTO_DEV_NET_TYPE(index), PROTO_DEV_NET_INDEX(index));
                        //printf("up_interval: %d\n", up_interval);
                        if (up_interval <= 1000) up_interval = 1000;
    					//if (up_interval <= 4) up_interval = 4;
                        while (!s_cc_reinit_flag[index][0] && net_isconnect(index)) {
                            struct Data_pkg pkg;
                            pkg.data_cnt = 0;
                            pkg.data = datas;
                            node = RT_NULL;
                            rt_thddog_suspend("cc_bjdc_data_add_data up_interval");
                            rt_enter_critical();
                           // printf("begain send data\n");
                            {
                                while (1) {
                                    node = pVarManage_GetExtDataWithUpProto(node, PROTO_DEV_NET_TYPE(index), PROTO_DEV_NET_INDEX(index), PROTO_DM101);
                                    if (node) {
                                        var_double_t ext_value = 0;
                                        if (node->xUp.bEnable) {
                                            if (up_interval > 0 && rt_tick_get() - node->xUp.xAvgUp.time >= rt_tick_from_millisecond(up_interval)) {
                                                //var_double_t value = (node->xUp.xAvgUp.val_avg / node->xUp.xAvgUp.count);
                                                //if (node->xUp.xAvgUp.count > 0) 
                                                var_double_t value = 0;
                                                bVarManage_GetExtValue(node, node->xIo.btOutVarType, &value);
                                                {
                                                    memset(&pkg.data[pkg.data_cnt], 0, sizeof(struct Data));

                                                   // pkg.data[pkg.data_cnt].sid = (node->xUp.szNid ? strtol(node->xUp.szNid, NULL, 0) : 0);
                                                    if (node->xUp.szNid && node->xUp.szNid[0]) {
                                                        tmp = strlen(node->xUp.szNid);
                                                        memcpy(pkg.data[pkg.data_cnt].sid, node->xUp.szNid, (tmp < DATA_ID_LEN ? tmp : DATA_ID_LEN));
                                                    }
                                                    
                                                    if (node->xUp.szFid && node->xUp.szFid[0]) {
                                                        tmp = strlen(node->xUp.szFid);
                                                        memcpy(pkg.data[pkg.data_cnt].id, node->xUp.szFid, (tmp < DATA_ID_LEN ? tmp : DATA_ID_LEN));
                                                    }
                                                    pkg.data[pkg.data_cnt].value = value;
                                                    pkg.data[pkg.data_cnt].status = node->xIo.bErrFlag ? 0 : 1;
                                                    pkg.data[pkg.data_cnt].pi = node->xUp.pi;
                                                    pkg.data_cnt++;
                                                    pkg.timestamp = get_utc_time();
                                                    node->xUp.xAvgUp.val_avg = 0;
                                                    node->xUp.xAvgUp.count = 0;
                                                    node->xUp.xAvgUp.time = rt_tick_get();
                                                    if (pkg.data_cnt == _DATA_COUNT) {
                                                        break;
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        break;
                                    }
                                }
                            }
                            rt_exit_critical();
                            if (pkg.data_cnt > 0) {
                                dm101_make_pkg(context, &dm101pkg, CODE_REPORT_DATA, &pkg, 0);
                                dm101_debug("send data:%d\n",dm101pkg.head.payload_length);

                                send_pkg(context, &dm101pkg);
                                pkg.data_cnt = 0;
                            } else {
                                break;
                            }
                            rt_thddog_resume();
                        }
                    }
                }
            }
            if (s_cc_reinit_flag[index][0] || !net_isconnect(index)) goto _START;
        }
    }

    dm101_debug("error\n");
    if (context) dm101_context_close(context);
    if(datas) rt_free(datas);
    s_dm101_work_thread[index] = RT_NULL;
    rt_thddog_unreg_inthd();
}

