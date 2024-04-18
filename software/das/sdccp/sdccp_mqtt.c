
#include "board.h"

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "sdccp_mqtt.h"
#include "sdccp_net.h"
#include "net_helper.h"
#include "mqtt/MQTTClient.h"
#include "sdccp_mqtt.h"

#include "sdccp_mqtt_ini.cc"
const char *_mqtt_default_ini = def_mqtt_default_ini;

void mqtt_try_create_default_config_file(const char *path)
{
    if (das_string_startwith(path, MQTT_INI_CFG_PATH_PREFIX, 1)) {
        if (das_get_file_length(path) < 20) {
            das_write_text_file(path, _mqtt_default_ini, strlen(_mqtt_default_ini));
        }
    }
}

#define MQTT_RECONNECT_TIMES        (5)
#define MQTT_REPUBLISH_TIMES        (2)

static mqtt_cfg_t           *s_mqtt_cfgs[BOARD_TCPIP_MAX];
static mqtt_upload_t        s_mqtt_upload_data[BOARD_TCPIP_MAX];
static rt_thread_t          s_mqtt_work_thread[BOARD_TCPIP_MAX];
static int                  s_mqtt_run[BOARD_TCPIP_MAX];
static enum mqtt_state      s_mqtt_state[BOARD_TCPIP_MAX];

static rt_bool_t __mqtt_report_data(rt_uint8_t index, MQTTClient client, ExtData_t **ppnode, rt_time_t report_time, int mins)  //上传分钟数据
{
//    rt_kprintf("__mqtt_report_data: %d\r\n",mins);
    cJSON *package = cJSON_CreateObject();
	if (package != RT_NULL) {
		char tmp[128] = {0};
        struct tm lt;
        das_localtime_r(&report_time, &lt);
        sprintf(tmp, "%04d%02d%02d%02d%02d%02d", lt.tm_year + 1900, lt.tm_mon+1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
        cJSON_AddStringToObject(package, "time", tmp);
        cJSON_AddNumberToObject(package, "tag", mins);

        cJSON *list = cJSON_CreateArray();
        if (list) {
            ExtData_t* node = *ppnode;
            rt_enter_critical();
            {
                rt_bool_t first = RT_TRUE;
                while (1) {
                    node = pVarManage_GetExtDataWithUpProto(node, PROTO_DEV_NET_TYPE(index), PROTO_DEV_NET_INDEX(index), PROTO_MQTT);
                    if (node) {
                        var_double_t ext_value = 0;
                        if (node->xUp.bEnable) {
                            cJSON *data = cJSON_CreateObject();
                            if (data) {
                                // 分钟数据
                                int add_flag = 0;
                                int data_flag = 0;
                                if (node->xUp.szNid) {
                                    cJSON_AddStringToObject(data, "nid", node->xUp.szNid);
                                    add_flag = 1;
                                }
                                if (node->xUp.szFid) {
                                    cJSON_AddStringToObject(data, "fid", node->xUp.szFid);
                                    add_flag = 1;
                                }
                                if (add_flag) {
                                    VarAvg_t *avg = RT_NULL;
                                    int time_diff = 60 * 1000;
                                    if (mins == 0) {
                                        avg = &node->xUp.xAvgUp;
                                        time_diff = s_mqtt_cfgs[index]->real_interval;
                                    } else if (mins == 1) {
                                        avg = &node->xUp.xAvgMin;
                                        time_diff = 60 * 1000;
                                    } else if (mins == 5) {
                                        avg = &node->xUp.xAvg5Min;
                                        time_diff = 5 * 60 * 1000;
                                    } else if (mins == 60) {
                                        avg = &node->xUp.xAvgHour;
                                        time_diff = 60 * 60 * 1000;
                                    }
                                    if ((s_mqtt_cfgs[index]->sharp_flag && mins > 0) || ((int)(rt_tick_get() - avg->time) >= rt_tick_from_millisecond(time_diff))) {
                                        if (avg->count > 0) {
                                            var_double_t value = (avg->val_avg / avg->count);
                                            if (!s_mqtt_cfgs[index]->no_min) {
                                                cJSON_AddNumberToObject(data, "min", avg->val_min);
                                                data_flag = 1;
                                            }
                                            if (!s_mqtt_cfgs[index]->no_max) {
                                                cJSON_AddNumberToObject(data, "max", avg->val_max);
                                                data_flag = 1;
                                            }
                                            if (!s_mqtt_cfgs[index]->no_avg) {
                                                cJSON_AddNumberToObject(data, "avg", value);
                                                data_flag = 1;
                                            }
                                            if (!s_mqtt_cfgs[index]->no_cou) {
                                                cJSON_AddNumberToObject(data, "cou", avg->val_avg);
                                                data_flag = 1;
                                            }
                                            avg->val_avg = 0;
                                            avg->val_min = 0;
                                            avg->val_max = 0;
                                            avg->count = 0;
                                            avg->time = rt_tick_get();
                                            *ppnode = node;
                                        }
                                    }
                                }
                                if (add_flag && data_flag) {
                                    cJSON_AddItemToArray(list, data);
                                } else {
                                    cJSON_Delete(data);
                                }
                            } else {
                                break;
                            }
                        } else {
                            *ppnode = node;
                        }
                    } else {
                        *ppnode = node;
                        break;
                    }
                }
            }
            rt_exit_critical();
            cJSON_AddItemToObject(package, "data", list);
            if (s_mqtt_cfgs[index]->topic_pub && cJSON_GetArraySize(list) > 0) {
                MQTTClient_message pubmsg = MQTTClient_message_initializer;
    	        MQTTClient_deliveryToken token;

                pubmsg.payload = (void *)cJSON_PrintUnformatted(package);
                if (pubmsg.payload) {
            	    pubmsg.payloadlen = strlen((char *)pubmsg.payload);
                }
            	pubmsg.qos = s_mqtt_cfgs[index]->qos;

                int repub_retry = MQTT_REPUBLISH_TIMES;
                while (s_mqtt_run[index] && MQTTClient_isConnected(client) && repub_retry--) {
                    if (pubmsg.payload) {
                        rt_kprintf("MQTTClient_publishMessage [%s]: %s\n", s_mqtt_cfgs[index]->topic_pub, (char *)pubmsg.payload);
                    }
                    int rc = MQTTClient_publishMessage(client, s_mqtt_cfgs[index]->topic_pub, &pubmsg, &token);
                    if (MQTTCLIENT_SUCCESS == rc || !MQTTClient_isConnected(client)) {
                        break;
                    }
                    rt_kprintf("MQTTClient_publishMessage failed : %d\n", rc);
                }
                if (pubmsg.payload) {
            	    RT_KERNEL_FREE(pubmsg.payload);
                }
            }
        }
	}
    cJSON_Delete(package);
    return RT_TRUE;
}

typedef struct  {
    var_bool_t      bEnable;        // 有效标志,该变量是否需要上传
    VarAvg_t        xAvgUp;         // 上传均值器
    VarAvg_t        xAvgMin;        // 分钟均值器
    VarAvg_t        xAvg5Min;       // 5分钟均值器
    VarAvg_t        xAvgHour;       // 小时均值器
    
}s_ExtDataUp_t;

typedef struct
{
    var_bool_t use;
    char nid[32];                   //仪表ID
    char fid[32];                   //因子ID
    s_ExtDataUp_t var;
}eVarInfo_t;

typedef struct 
{
    char nid[32];
    int var_cnt;
    eVarInfo_t pvar[128];  
}s_nid_t;

#define NID_NUMS (32)
static s_nid_t s_nid[NID_NUMS]; //最多支持多少个仪表
static int s_nid_num = 0;

static eVarInfo_t vars[128];
static eVarInfo_t vars_del[128];
static int var_nums = 0,var_del_nums = 0;

static void del_struct(eVarInfo_t *pvars,int *plen,int k)
{
    int len = *plen;
    for(int i = k; i < len; i++){
        pvars[i] = pvars[i+1];
    }
    len--;
    *plen = len;
}

static void find_key(eVarInfo_t *pvars,int *plen)
{
    int len = *plen;
    int i = 0;
    for(i = 0 ; i < len;){
        int flag = 0;
        for(int j = i+1;j<len;j++){
            if(strcmp(pvars[j].nid, pvars[i].nid) == 0 ){
                del_struct(pvars,&len,j);
                flag = 1;
            }
        }
        if(flag == 1) i = 0;
        else i++;
    }
    *plen = len;
}

static char * create_json(s_nid_t *p_nid,int len,rt_time_t report_time,int mins)
{
   /* VarAvg_t *avg = RT_NULL;
    int time_diff = 60 * 1000;
    if (mins == 0) {
        avg = &node->xUp.xAvgUp;
        time_diff = s_mqtt_cfgs[index]->real_interval;
    } else if (mins == 1) {
        avg = &node->xUp.xAvgMin;
        time_diff = 60 * 1000;
    } else if (mins == 5) {
        avg = &node->xUp.xAvg5Min;
        time_diff = 5 * 60 * 1000;
    } else if (mins == 60) {
        avg = &node->xUp.xAvgHour;
        time_diff = 60 * 60 * 1000;
    }*/
       cJSON *package = cJSON_CreateObject();

       for(int i = 0 ; i < len; i++){

            VarAvg_t *avg = RT_NULL;
    
            cJSON *array = cJSON_CreateArray();
             if (array == NULL){
                 goto _end;
             }
             cJSON *item = cJSON_CreateObject();
             if (item == NULL){
                 goto _end;
             }
             cJSON *dvalue = cJSON_CreateObject();
             if (dvalue == NULL){
                 goto _end;
             }
             for(int j = 0; j < p_nid[i].var_cnt; j++){
                VarAvg_t *avg = RT_NULL;
                 if (mins == 0){
                    avg = &p_nid[i].pvar[j].var.xAvgUp;
                 }else if (mins == 1) {
                    avg = &p_nid[i].pvar[j].var.xAvgMin;
                } else if (mins == 5) {
                    avg = &p_nid[i].pvar[j].var.xAvg5Min;
                } else if (mins == 60) {
                    avg = &p_nid[i].pvar[j].var.xAvgHour;
                }
                if (avg->count > 0) {
                        var_double_t value = (avg->val_avg / avg->count);
                        cJSON_AddNumberToObject(dvalue,  p_nid[i].pvar[j].fid, value);
                }
             }
             cJSON_AddItemToObject(item, "values", dvalue);
             cJSON_AddNumberToObject(item,"ts",report_time);
              cJSON_AddItemToArray(array, item);
             cJSON_AddItemToObject(package,  p_nid[i].nid, array);
       }

        char* string = cJSON_Print(package);
        rt_kprintf("json: %s\n",string);
_end:
     cJSON_Delete(package);
     return 0;  

}

static mdUINT32 last_report_data = 0;


static rt_bool_t __mqtt_report_data_td(rt_uint8_t index, MQTTClient client, ExtData_t **ppnode, rt_time_t report_time, int mins)  //上传分钟数据
{
      //  rt_kprintf("__mqtt_report_data_td: %d\r\n",mins);

        struct timeval tv;
        gettimeofday(&tv,NULL);
        mdUINT64 millisecond = (mdUINT64)tv.tv_sec*1000+tv.tv_usec/1000;

      //  rt_kprintf("report_time:%ld\n",report_time);  //秒
       // rt_kprintf("second:%ld\n",tv.tv_sec);  //秒
       // rt_kprintf("millisecond:%lu\n",millisecond);  //毫秒


		char tmp[128] = {0};
        struct tm lt;
        das_localtime_r(&report_time, &lt);
        sprintf(tmp, "%04d%02d%02d%02d%02d%02d", lt.tm_year + 1900, lt.tm_mon+1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
       // cJSON_AddStringToObject(package, "time", tmp);
        //cJSON_AddNumberToObject(package, "tag", mins);
       // cJSON *package = cJSON_CreateObject();

        var_nums = 0;
        var_del_nums = 0;

        memset(vars,0,sizeof(vars));
        memset(vars_del,0,sizeof(vars_del));
        
        ExtData_t* node = *ppnode;

     // rt_kprintf("__mqtt_report_data_td: %d\r\n",1);

        
        rt_enter_critical();
        {
            rt_bool_t first = RT_TRUE;
            while (1) {
                node = pVarManage_GetExtDataWithUpProto(node, PROTO_DEV_NET_TYPE(index), PROTO_DEV_NET_INDEX(index), PROTO_MQTT);
                if (node) {
                    var_double_t ext_value = 0;
                    if (node->xUp.bEnable) {

                         vars[var_nums].use = mdTRUE;
                         strcpy(vars[var_nums].nid,node->xUp.szNid);
                         strcpy(vars[var_nums].fid,node->xUp.szFid);
                         vars[var_nums].var.xAvgUp = node->xUp.xAvgUp;
                         vars[var_nums].var.xAvgMin = node->xUp.xAvgMin;
                         vars[var_nums].var.xAvg5Min = node->xUp.xAvg5Min;
                         vars[var_nums].var.xAvgHour = node->xUp.xAvgHour;
                         vars[var_nums].var.bEnable = mdTRUE;
                         var_nums++;
                         *ppnode = node;      
                    } else {
                        *ppnode = node;
                    }
                } else {
                    *ppnode = node;
                    break;
                }
            }
        }  
        rt_exit_critical();

        //rt_kprintf("__mqtt_report_data_td: %d\r\n",1);


        var_del_nums = var_nums;
        memcpy(vars_del,vars,sizeof(vars_del));

              
       /* rt_kprintf("var_nums: %d\r\n",var_nums);
        for(int i = 0; i < var_nums; i++){
            rt_kprintf("var: %d, nid: %s, fid: %s\n",i,vars[i].nid,vars[i].fid);
        }*/
        
        find_key(vars_del, &var_del_nums);
  

       /* rt_kprintf("var_del_nums: %d\r\n",var_del_nums);
        for(int i = 0; i < var_del_nums; i++){
            rt_kprintf("var: %d, nid: %s, fid: %s\n",i,vars_del[i].nid,vars_del[i].fid);
        }*/

        memset(s_nid,0,sizeof(s_nid));
        s_nid_num = var_del_nums; //实际有多少个仪表
       // rt_kprintf("s_nid_num: %d\r\n",s_nid_num);

        char var_index[32] = {0};

        for(int j = 0; j < s_nid_num;j++){
            for(int i = 0; i < var_nums; i++){
                if( strcmp(vars[i].nid,vars_del[j].nid) == 0){
                    strcpy(s_nid[j].nid,vars[i].nid);
                    s_nid[j].pvar[var_index[j]++] = vars[i];
                }
            }
        }

        for(int i = 0 ; i < s_nid_num; i++){
            s_nid[i].var_cnt = var_index[i]; 
        }

     /*s   for(int i = 0 ; i < s_nid_num; i++){
            rt_kprintf("nid:%s ->\n",s_nid[i].nid);
            for(int j = 0; j < s_nid[i].var_cnt;j++){
                rt_kprintf("%d,fid:%s, val_avg: %d,count: %d\n",
                    j,s_nid[i].pvar[j].fid,s_nid[i].pvar[j].var.xAvgUp.val_avg,s_nid[i].pvar[j].var.xAvgUp.count);
            }
            rt_kprintf("===============\r\n");
        }*/

        int time_diff = 60 * 1000;
        if (mins == 0) {
            time_diff = s_mqtt_cfgs[index]->real_interval;
        } else if (mins == 1) {
            time_diff = 60 * 1000;
        } else if (mins == 5) {
            time_diff = 5 * 60 * 1000;
        } else if (mins == 60) {
            time_diff = 60 * 60 * 1000;
        }

        cJSON *package = cJSON_CreateObject();


         if ((s_mqtt_cfgs[index]->sharp_flag && mins > 0) || ((int)(rt_tick_get() - last_report_data) >= rt_tick_from_millisecond(time_diff))) {
               for(int i = 0 ; i < s_nid_num; i++){
                    cJSON *array = cJSON_CreateArray();
                     if (array == NULL){
                         goto _end;
                     }
                     cJSON *item = cJSON_CreateObject();
                     if (item == NULL){
                         goto _end;
                     }
                     cJSON *dvalue = cJSON_CreateObject();
                     if (dvalue == NULL){
                         goto _end;
                     }
                     for(int j = 0; j < s_nid[i].var_cnt; j++){
                        VarAvg_t *avg = RT_NULL;
                         if (mins == 0){
                            avg = &s_nid[i].pvar[j].var.xAvgUp;
                         }else if (mins == 1) {
                            avg = &s_nid[i].pvar[j].var.xAvgMin;
                        } else if (mins == 5) {
                            avg = &s_nid[i].pvar[j].var.xAvg5Min;
                        } else if (mins == 60) {
                            avg = &s_nid[i].pvar[j].var.xAvgHour;
                        }
                       // if (avg->count > 0) {
                                var_double_t value = avg->val_cur;
                                cJSON_AddNumberToObject(dvalue,  s_nid[i].pvar[j].fid, value);
                              //  avg->count = 0;
                       // }
                     }
                     cJSON_AddItemToObject(item, "values", dvalue);
                     cJSON_AddNumberToObject(item,"ts",millisecond);
                      cJSON_AddItemToArray(array, item);
                     cJSON_AddItemToObject(package,  s_nid[i].nid, array);
               }

              if ( (s_mqtt_cfgs[index]->topic_pub > 0) && (s_nid_num > 0) ) {
                MQTTClient_message pubmsg = MQTTClient_message_initializer;
    	        MQTTClient_deliveryToken token;

                pubmsg.payload = (void *)cJSON_PrintUnformatted(package);
                if (pubmsg.payload) {
            	    pubmsg.payloadlen = strlen((char *)pubmsg.payload);
                }
            	pubmsg.qos = s_mqtt_cfgs[index]->qos;

                int repub_retry = MQTT_REPUBLISH_TIMES;
                while (s_mqtt_run[index] && MQTTClient_isConnected(client) && repub_retry--) {
                    if (pubmsg.payload) {
                        rt_kprintf("MQTTClient_publishMessage [%s]: %s\n", s_mqtt_cfgs[index]->topic_pub, (char *)pubmsg.payload);
                        elog_i("mqtt td", "send:%d", strlen((char *)pubmsg.payload));
                    }
                    int rc = MQTTClient_publishMessage(client, s_mqtt_cfgs[index]->topic_pub, &pubmsg, &token);
                    if (MQTTCLIENT_SUCCESS == rc || !MQTTClient_isConnected(client)) {
                        break;
                    }
                    rt_kprintf("MQTTClient_publishMessage failed : %d\n", rc);
                    elog_i("mqtt td", "publishMessage err:%d", rc);
                }
                if (pubmsg.payload) {
            	    RT_KERNEL_FREE(pubmsg.payload);
                }
            }
              
            last_report_data = rt_tick_get();

         }
        
_end:        
	    cJSON_Delete(package);
        return RT_TRUE;
}


//北京中益明光科技有限公司
static rt_bool_t __mqtt_report_data_zy(rt_uint8_t index, MQTTClient client, ExtData_t **ppnode, rt_time_t report_time, int mins)  //上传分钟数据
{
       // rt_kprintf("__mqtt_report_data_zy: %d\r\n",mins);

        struct timeval tv;
        gettimeofday(&tv,NULL);
        mdUINT64 millisecond = (mdUINT64)tv.tv_sec*1000+tv.tv_usec/1000;

      //  rt_kprintf("report_time:%ld\n",report_time);  //秒
       // rt_kprintf("second:%ld\n",tv.tv_sec);  //秒
       // rt_kprintf("millisecond:%lu\n",millisecond);  //毫秒


		char tmp[128] = {0};
        struct tm lt;
        das_localtime_r(&report_time, &lt);
        sprintf(tmp, "%04d%02d%02d%02d%02d%02d", lt.tm_year + 1900, lt.tm_mon+1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
       // cJSON_AddStringToObject(package, "time", tmp);
        //cJSON_AddNumberToObject(package, "tag", mins);
       // cJSON *package = cJSON_CreateObject();

        var_nums = 0;
        var_del_nums = 0;

        memset(vars,0,sizeof(vars));
        memset(vars_del,0,sizeof(vars_del));
        
        ExtData_t* node = *ppnode;

     // rt_kprintf("__mqtt_report_data_td: %d\r\n",1);

        
        rt_enter_critical();
        {
            rt_bool_t first = RT_TRUE;
            while (1) {
                node = pVarManage_GetExtDataWithUpProto(node, PROTO_DEV_NET_TYPE(index), PROTO_DEV_NET_INDEX(index), PROTO_MQTT);
                if (node) {
                    var_double_t ext_value = 0;
                    if (node->xUp.bEnable) {

                         vars[var_nums].use = mdTRUE;
                         strcpy(vars[var_nums].nid,node->xUp.szNid);
                         strcpy(vars[var_nums].fid,node->xUp.szFid);
                         vars[var_nums].var.xAvgUp = node->xUp.xAvgUp;
                         vars[var_nums].var.xAvgMin = node->xUp.xAvgMin;
                         vars[var_nums].var.xAvg5Min = node->xUp.xAvg5Min;
                         vars[var_nums].var.xAvgHour = node->xUp.xAvgHour;
                         vars[var_nums].var.bEnable = mdTRUE;
                         var_nums++;
                         *ppnode = node;      
                    } else {
                        *ppnode = node;
                    }
                } else {
                    *ppnode = node;
                    break;
                }
            }
        }  
        rt_exit_critical();

        //rt_kprintf("__mqtt_report_data_td: %d\r\n",1);


        var_del_nums = var_nums;
        memcpy(vars_del,vars,sizeof(vars_del));

              
       /* rt_kprintf("var_nums: %d\r\n",var_nums);
        for(int i = 0; i < var_nums; i++){
            rt_kprintf("var: %d, nid: %s, fid: %s\n",i,vars[i].nid,vars[i].fid);
        }*/
        
        find_key(vars_del, &var_del_nums);
  

       /* rt_kprintf("var_del_nums: %d\r\n",var_del_nums);
        for(int i = 0; i < var_del_nums; i++){
            rt_kprintf("var: %d, nid: %s, fid: %s\n",i,vars_del[i].nid,vars_del[i].fid);
        }*/

        memset(s_nid,0,sizeof(s_nid));
        s_nid_num = var_del_nums; //实际有多少个仪表
       // rt_kprintf("s_nid_num: %d\r\n",s_nid_num);

        char var_index[32] = {0};

        for(int j = 0; j < s_nid_num;j++){
            for(int i = 0; i < var_nums; i++){
                if( strcmp(vars[i].nid,vars_del[j].nid) == 0){
                    strcpy(s_nid[j].nid,vars[i].nid);
                    s_nid[j].pvar[var_index[j]++] = vars[i];
                }
            }
        }

        for(int i = 0 ; i < s_nid_num; i++){
            s_nid[i].var_cnt = var_index[i]; 
        }

     /*s   for(int i = 0 ; i < s_nid_num; i++){
            rt_kprintf("nid:%s ->\n",s_nid[i].nid);
            for(int j = 0; j < s_nid[i].var_cnt;j++){
                rt_kprintf("%d,fid:%s, val_avg: %d,count: %d\n",
                    j,s_nid[i].pvar[j].fid,s_nid[i].pvar[j].var.xAvgUp.val_avg,s_nid[i].pvar[j].var.xAvgUp.count);
            }
            rt_kprintf("===============\r\n");
        }*/

        int time_diff = 60 * 1000;
        if (mins == 0) {
            time_diff = s_mqtt_cfgs[index]->real_interval;
        } else if (mins == 1) {
            time_diff = 60 * 1000;
        } else if (mins == 5) {
            time_diff = 5 * 60 * 1000;
        } else if (mins == 60) {
            time_diff = 60 * 60 * 1000;
        }

        cJSON *package = cJSON_CreateObject();
        if(package == NULL){
			 goto _end;
		}

         cJSON *item = cJSON_CreateObject();
         if (item == NULL){
             goto _end;
         }


         if ((s_mqtt_cfgs[index]->sharp_flag && mins > 0) || ((int)(rt_tick_get() - last_report_data) >= rt_tick_from_millisecond(time_diff))) {
              // rt_kprintf("000:%d\r\n",s_nid_num);

               for(int i = 0 ; i < s_nid_num; i++){

                     cJSON *dvalue = cJSON_CreateObject();
                     if (dvalue == NULL){
                         rt_kprintf("dvalue failed\r\n");
                         goto _end;
                     }


                    // rt_kprintf("111:%d\r\n",s_nid[i].var_cnt);

                     
                     for(int j = 0; j < s_nid[i].var_cnt; j++){
                        
                        cJSON *FID = cJSON_CreateObject();
                        if (FID == NULL){
                             rt_kprintf("fid failed\r\n");
                            goto _end;
                        }
                     
                        VarAvg_t *avg = RT_NULL;
                         if (mins == 0){
                            avg = &s_nid[i].pvar[j].var.xAvgUp;
                         }else if (mins == 1) {
                            avg = &s_nid[i].pvar[j].var.xAvgMin;
                        } else if (mins == 5) {
                            avg = &s_nid[i].pvar[j].var.xAvg5Min;
                        } else if (mins == 60) {
                            avg = &s_nid[i].pvar[j].var.xAvgHour;
                        }
                        
                        var_double_t value = avg->val_cur;
                        cJSON_AddNumberToObject(FID,  "value", value);
                        cJSON_AddNumberToObject(FID,  "type", 2);
                        cJSON_AddItemToObject(dvalue, s_nid[i].pvar[j].fid, FID);
                     }
                     
                     cJSON_AddItemToObject(item,   s_nid[i].nid,dvalue);
               }

            //   printf("111:%s\r\n",(void *)cJSON_PrintUnformatted(item));

               if(strlen(g_host_cfg.szHostName) > 0){
                    cJSON_AddItemToObject(package,g_host_cfg.szHostName, item);
               }else {
                    if(strlen(g_sys_info.SN) > 0){
                        cJSON_AddItemToObject(package,g_sys_info.SN, item);
                    }else {
                        cJSON_AddItemToObject(package,"dev_id", item);
                    }
               }

              // rt_kprintf("222\r\n");


              if ( (s_mqtt_cfgs[index]->topic_pub > 0) && (s_nid_num > 0) ) {
                MQTTClient_message pubmsg = MQTTClient_message_initializer;
    	        MQTTClient_deliveryToken token;

                pubmsg.payload = (void *)cJSON_PrintUnformatted(package);

                // rt_kprintf("333\r\n");
                
                if (pubmsg.payload) {
            	    pubmsg.payloadlen = strlen((char *)pubmsg.payload);
                }
            	pubmsg.qos = s_mqtt_cfgs[index]->qos;

                int repub_retry = MQTT_REPUBLISH_TIMES;
                while (s_mqtt_run[index] && MQTTClient_isConnected(client) && repub_retry--) {
                    if (pubmsg.payload) {
                        rt_kprintf("MQTTClient_publishMessage [%s]: %s\n", s_mqtt_cfgs[index]->topic_pub, (char *)pubmsg.payload);
                        elog_i("mqtt td", "send:%d", strlen((char *)pubmsg.payload));
                    }
                    int rc = MQTTClient_publishMessage(client, s_mqtt_cfgs[index]->topic_pub, &pubmsg, &token);
                    if (MQTTCLIENT_SUCCESS == rc || !MQTTClient_isConnected(client)) {
                        break;
                    }
                    rt_kprintf("MQTTClient_publishMessage failed : %d\n", rc);
                    elog_i("mqtt td", "publishMessage err:%d", rc);
                }
                if (pubmsg.payload) {
            	    RT_KERNEL_FREE(pubmsg.payload);
                }
            }
              
            last_report_data = rt_tick_get();

         }
        
_end:        
	    cJSON_Delete(package);
        return RT_TRUE;
}




rt_bool_t mqtt_report_real_data(rt_uint8_t index, void *client, ExtData_t **ppnode, rt_time_t report_time)  //上传分钟数据
{
    if( (strlen(s_mqtt_cfgs[index]->sdccp_format) >0 ) && (strcmp(s_mqtt_cfgs[index]->sdccp_format,"td") == 0) ) {
        return __mqtt_report_data_td( index, (MQTTClient)client, ppnode, report_time, 0);
    }else if( (strlen(s_mqtt_cfgs[index]->sdccp_format) >0 ) && (strcmp(s_mqtt_cfgs[index]->sdccp_format,"zy") == 0) ) {
        return __mqtt_report_data_zy( index, (MQTTClient)client, ppnode, report_time, 0);
    }else {
       return __mqtt_report_data(index, (MQTTClient)client, ppnode, report_time, 0);
    }
}

rt_bool_t mqtt_report_minutes_data(rt_uint8_t index, void *client, ExtData_t **ppnode, rt_time_t report_time)  //上传分钟数据
{
    if( (strlen(s_mqtt_cfgs[index]->sdccp_format) >0 ) && (strcmp(s_mqtt_cfgs[index]->sdccp_format,"td") == 0) ) {
        return __mqtt_report_data_td( index, (MQTTClient)client, ppnode, report_time, 1);
    }else if( (strlen(s_mqtt_cfgs[index]->sdccp_format) >0 ) && (strcmp(s_mqtt_cfgs[index]->sdccp_format,"zy") == 0) ) {
        return __mqtt_report_data_zy( index, (MQTTClient)client, ppnode, report_time, 1);
    }else {
       return __mqtt_report_data(index, (MQTTClient)client, ppnode, report_time, 1);
    }
    //return __mqtt_report_data(index, (MQTTClient)client, ppnode, report_time, 1);
}

rt_bool_t mqtt_report_5minutes_data(rt_uint8_t index, void *client, ExtData_t **ppnode, rt_time_t report_time)  //上传分钟数据
{
    if( (strlen(s_mqtt_cfgs[index]->sdccp_format) >0 ) && (strcmp(s_mqtt_cfgs[index]->sdccp_format,"td") == 0) ) {
        return __mqtt_report_data_td( index, (MQTTClient)client, ppnode, report_time, 5);
    }else if( (strlen(s_mqtt_cfgs[index]->sdccp_format) >0 ) && (strcmp(s_mqtt_cfgs[index]->sdccp_format,"zy") == 0) ) {
        return __mqtt_report_data_zy( index, (MQTTClient)client, ppnode, report_time, 5);
    }else {
       return __mqtt_report_data(index, (MQTTClient)client, ppnode, report_time, 5);
    }
   // return __mqtt_report_data(index, (MQTTClient)client, ppnode, report_time, 5);
}

rt_bool_t mqtt_report_hour_data(rt_uint8_t index, void *client, ExtData_t **ppnode, rt_time_t report_time)  //上传小时数据
{
    if( (strlen(s_mqtt_cfgs[index]->sdccp_format) >0 ) && (strcmp(s_mqtt_cfgs[index]->sdccp_format,"td") == 0) ) {
        return __mqtt_report_data_td( index, (MQTTClient)client, ppnode, report_time, 60);
    }else if( (strlen(s_mqtt_cfgs[index]->sdccp_format) >0 ) && (strcmp(s_mqtt_cfgs[index]->sdccp_format,"zy") == 0) ) {
        return __mqtt_report_data_zy( index, (MQTTClient)client, ppnode, report_time, 60);
    }else {
       return __mqtt_report_data(index, (MQTTClient)client, ppnode, report_time, 60);
    }
    //return __mqtt_report_data(index, (MQTTClient)client, ppnode, report_time, 60);
}

static void __mqtt_default_cfg(int index)
{
    memset(s_mqtt_cfgs[index], 0, sizeof(mqtt_cfg_t));
	s_mqtt_cfgs[index]->username = NULL;
	s_mqtt_cfgs[index]->password = NULL;
	s_mqtt_cfgs[index]->client_id = das_strdup(NULL, g_sys_info.SN);
	if(g_host_cfg.szId[0]) {
        if (s_mqtt_cfgs[index]->client_id) RT_KERNEL_FREE(s_mqtt_cfgs[index]->client_id);
	    s_mqtt_cfgs[index]->client_id = das_strdup(NULL, g_host_cfg.szHostName);
	}
    s_mqtt_cfgs[index]->keepalive = 60;
    s_mqtt_cfgs[index]->no_cou = 1;
    s_mqtt_cfgs[index]->no_min = 1;
    s_mqtt_cfgs[index]->no_max = 1;
}

static void __mqtt_cfg_free(int index)
{
    if (s_mqtt_cfgs[index]) {
        RT_KERNEL_FREE(s_mqtt_cfgs[index]->username);
        RT_KERNEL_FREE(s_mqtt_cfgs[index]->password);
        RT_KERNEL_FREE(s_mqtt_cfgs[index]->client_id);
        RT_KERNEL_FREE(s_mqtt_cfgs[index]->topic_sub);
        RT_KERNEL_FREE(s_mqtt_cfgs[index]->topic_pub);
        RT_KERNEL_FREE(s_mqtt_cfgs[index]);
    }
}

void mqtt_global_init(void)
{
    static int _init_flag = 0;
    MQTTClient_init_options mqtt_inits = MQTTClient_init_options_initializer;
    
    if (0 == _init_flag) {
        mqtt_inits.do_openssl_init = 1;
        MQTTClient_global_init(&mqtt_inits);
        _init_flag = 1;
    }
}

rt_bool_t mqtt_open(rt_uint8_t index)
{
    mqtt_close(index);

    __mqtt_cfg_free(index);
    s_mqtt_cfgs[index] = RT_KERNEL_CALLOC(sizeof(mqtt_cfg_t));

    s_cc_reinit_flag[index][0] = RT_FALSE;
    __mqtt_default_cfg(index);
    {
        char buf[64] = "";
        ini_t *ini;
        sprintf(buf, MQTT_INI_CFG_PATH_PREFIX "%d" ".ini", index);
        ini = ini_load(buf);

        if (ini) {
            const char *str = ini_getstring(ini, "common", "username", "");
            if (str && str[0]) s_mqtt_cfgs[index]->username = das_strdup(NULL, str);
            
            str = ini_getstring(ini, "common", "password", "");
            if (str && str[0]) s_mqtt_cfgs[index]->password = das_strdup(NULL, str);
            
            str = ini_getstring(ini, "common", "client_id", "");
            if (s_mqtt_cfgs[index]->client_id) RT_KERNEL_FREE(s_mqtt_cfgs[index]->client_id);
            s_mqtt_cfgs[index]->client_id = das_strdup(NULL, str);

            str = ini_getstring(ini, "common", "sdccp_format", "");
            if (s_mqtt_cfgs[index]->sdccp_format) RT_KERNEL_FREE(s_mqtt_cfgs[index]->sdccp_format);
            s_mqtt_cfgs[index]->sdccp_format = das_strdup(NULL, str);

            
            
            str = ini_getstring(ini, "common", "topic_sub", "");
            if(str && str[0]) s_mqtt_cfgs[index]->topic_sub = das_strdup(NULL, str);
            
            str = ini_getstring(ini, "common", "topic_pub", "");
            if(str && str[0]) s_mqtt_cfgs[index]->topic_pub = das_strdup(NULL, str);

            s_mqtt_cfgs[index]->keepalive = ini_getint(ini, "common", "keepalive", 60);
            s_mqtt_cfgs[index]->real_interval = ini_getint(ini, "common", "real_interval", 30000);
            
            s_mqtt_cfgs[index]->qos = ini_getint(ini, "common", "qos", 0);
            s_mqtt_cfgs[index]->retained = ini_getint(ini, "common", "retained", 0);
            s_mqtt_cfgs[index]->real_flag = ini_getboolean(ini, "common", "real_flag", 0);
            s_mqtt_cfgs[index]->min_flag = ini_getboolean(ini, "common", "min_flag", 0);
            s_mqtt_cfgs[index]->min_5_flag = ini_getboolean(ini, "common", "min_5_flag", 0);
            s_mqtt_cfgs[index]->hour_flag = ini_getboolean(ini, "common", "hour_flag", 0);
            s_mqtt_cfgs[index]->sharp_flag = ini_getboolean(ini, "common", "sharp_flag", 0);
            s_mqtt_cfgs[index]->no_min = ini_getboolean(ini, "common", "no_min", 1);
            s_mqtt_cfgs[index]->no_max = ini_getboolean(ini, "common", "no_max", 1);
            s_mqtt_cfgs[index]->no_avg = ini_getboolean(ini, "common", "no_avg", 1);
            s_mqtt_cfgs[index]->no_cou = ini_getboolean(ini, "common", "no_cou", 1);
            
            s_mqtt_upload_data[index].last_min_time = das_get_time();
            s_mqtt_upload_data[index].last_min_5_time = das_get_time();
            s_mqtt_upload_data[index].last_hour_time = das_get_time();

            s_mqtt_cfgs[index]->plc_num = ini_getint(ini, "common", "plc_num", 0);
            rt_kprintf("plc_num:%d\r\n",s_mqtt_cfgs[index]->plc_num);
            for(int i = 0 ; i <  s_mqtt_cfgs[index]->plc_num; i++){
              char key_plc_name[32] = {0};
              sprintf(key_plc_name,"plc%d_name",i);
              char key_plc_chan[32] = {0};
              sprintf(key_plc_chan,"plc%d_chan",i);
              char key_plc_reg[32] = {0};
              sprintf(key_plc_reg,"plc%d_reg",i);
              str = ini_getstring(ini, "common", key_plc_name, "");
              if (str && str[0]) s_mqtt_cfgs[index]->plc_info[i].plc_name = das_strdup(NULL, str);
              s_mqtt_cfgs[index]->plc_info[i].plc_chan = ini_getint(ini, "common", key_plc_chan, -1);
              s_mqtt_cfgs[index]->plc_info[i].plc_reg = ini_getint(ini, "common", key_plc_reg, -1);
              rt_kprintf("## plc%d->name:%s,chan:%d,reg:%d\r\n",i,s_mqtt_cfgs[index]->plc_info[i].plc_name,s_mqtt_cfgs[index]->plc_info[i].plc_chan,s_mqtt_cfgs[index]->plc_info[i].plc_reg);
            }

            rt_kprintf("mqtt[%d], username: %s, password: %s\n", index, s_mqtt_cfgs[index]->username, s_mqtt_cfgs[index]->password);
            rt_kprintf("mqtt[%d], client_id: %s\n", index, s_mqtt_cfgs[index]->client_id);
            rt_kprintf("mqtt[%d], topic_sub: %s\n", index, s_mqtt_cfgs[index]->topic_sub);
            rt_kprintf("mqtt[%d], topic_pub: %s\n", index, s_mqtt_cfgs[index]->topic_pub);
            rt_kprintf("mqtt[%d], sdccp_format: %s\n", index, s_mqtt_cfgs[index]->sdccp_format);

            

            ini_free(ini);
            
            mqtt_startwork(index);
        } else {
            rt_kprintf(" %s load failed\r\n", buf);
        }
    }
    return RT_TRUE;
}

void mqtt_close(rt_uint8_t index)
{
    mqtt_exitwork(index);
}

static void __mqtt_work_task(void *parameter);

void mqtt_startwork(rt_uint8_t index)
{
    rt_kprintf("s_mqtt_run[%d] = %d\n", index, s_mqtt_run[index]);
    if(RT_NULL == s_mqtt_work_thread[index]) {
        s_mqtt_run[index] = 1;
        s_mqtt_state[index] = MQTT_S_INIT;
        BOARD_CREAT_NAME(szWork, "mqtt_%d", index);
        s_mqtt_work_thread[index] = \
            rt_thread_create(szWork,
                                    __mqtt_work_task,
                                    (void *)(long)index,
                                    2048,
                                    20, 20);
        if (s_mqtt_work_thread[index] != RT_NULL) {
            rt_thddog_register(s_mqtt_work_thread[index], 30);
            rt_thread_startup(s_mqtt_work_thread[index]);
        } else {
            s_mqtt_run[index] = 0;
            s_mqtt_state[index] = MQTT_S_EXIT;
        }
    }
}

void mqtt_exitwork(rt_uint8_t index)
{
    if (s_mqtt_run[index] && s_mqtt_state[index] != MQTT_S_INIT) {
        s_mqtt_run[index] = 0;
        while (s_mqtt_state[index] != MQTT_S_EXIT) {
            sleep(1);
            rt_kprintf("mqtt_state[%d] : %d\n", index, s_mqtt_state[index]);
        }
    }
}

typedef struct {
    char nid[32];
    char fid[32];
    int type;
    int value;
    int mode;
    int rate;
}ZyControlData_t;

static int zy_find_nid(int index, char *nid)
{
    for(int i = 0 ; i < s_mqtt_cfgs[index]->plc_num;i++){
        if(s_mqtt_cfgs[index]->plc_info[i].plc_name && strcmp(nid,s_mqtt_cfgs[index]->plc_info[i].plc_name) == 0){
            return i;
        }
    }
    return -1;
}

static int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
	int i;
    rt_uint8_t index = (int)(long)context;

	rt_kprintf("Message arrived\n");
	rt_kprintf("     index: %d\n", index);
	rt_kprintf("     topic(%d): %s\n", topicLen, topicName);
	rt_kprintf("     dup: %d\n", message->dup);
	rt_kprintf("     msgid: %d\n", message->msgid);
	rt_kprintf("   message: %.*s\n", message->payloadlen, message->payload ? message->payload : "");

    if (topicName && message->payload && message->payloadlen > 0) {

        if(strcmp(s_mqtt_cfgs[index]->sdccp_format,"zy") == 0){
            static char json_str[1024] = {0};
            memcpy(json_str,message->payload,message->payloadlen);
            cJSON *root = cJSON_Parse(json_str);
            if(!root){
                rt_kprintf("json error:%s\r\n",cJSON_GetErrorPtr());
            }else {
                char dev_id[64] = {0};
                if(strlen(g_host_cfg.szHostName) > 0){
                    strcpy(dev_id,g_host_cfg.szHostName);
                }else if(strlen(g_sys_info.SN) > 0){
                    strcpy(dev_id,g_sys_info.SN);
                }
                ZyControlData_t dataArray[10];
                int data_cnt = 0;
                memset(dataArray,0,sizeof(ZyControlData_t)*10);
                
                rt_kprintf("string:%s , dev_id:%s\r\n",root->child->string,dev_id);
              //  cJSON *item = cJSON_GetObjectItem(root, root->child->string);
                if(strcmp(root->child->string,dev_id) == 0){
                   // rt_kprintf("item->next:%p\r\n",item->next);
                   cJSON *item_nid = root->child->child;
                   rt_kprintf("item_nid:%s\r\n",item_nid->string);
                    while(item_nid != NULL){
                       // rt_kprintf("nid:%s\r\n",item_nid->string);
                        strcpy(dataArray[data_cnt].nid,item_nid->string);
                        cJSON *item_fid = item_nid->child;
                       // rt_kprintf("fid:%s\r\n",item_fid->string);
                        strcpy(dataArray[data_cnt].fid,item_fid->string);
                        dataArray[data_cnt].type =  cJSON_GetInt(item_fid,"type", -1);
                        dataArray[data_cnt].value =  cJSON_GetInt(item_fid,"value", -1);
                        dataArray[data_cnt].mode =  cJSON_GetInt(item_fid,"mode", -1);
                        dataArray[data_cnt].rate =  cJSON_GetInt(item_fid,"rate", -1);
                        rt_kprintf("nid:%s, fid:%s,%d,%d,%d,%d\r\n",dataArray[data_cnt].nid,dataArray[data_cnt].fid,
                            dataArray[data_cnt].type,dataArray[data_cnt].value,dataArray[data_cnt].mode,dataArray[data_cnt].rate);
                        item_nid=item_nid->next;
                        data_cnt++;
                    }
                    for(int i = 0 ; i < data_cnt; i++){
                        int chan = zy_find_nid(index, dataArray[i].nid);
                        rt_kprintf("chan:%d\r\n",chan);
                        if(chan >= 0){
                             rt_uint16_t data = htons(dataArray[i].value);
                             rt_kprintf("write register-> port:%d,reg:%d,data:%d\r\n",s_mqtt_cfgs[index]->plc_info[chan].plc_chan,
                             s_mqtt_cfgs[index]->plc_info[chan].plc_reg,dataArray[i].value);
                             modbus_write_registers_with_tcp(s_mqtt_cfgs[index]->plc_info[chan].plc_chan, 1, s_mqtt_cfgs[index]->plc_info[chan].plc_reg, 1, &data);
                             rt_kprintf("modbus_write_registers_with_tcp\r\n");
                        }
                    }
                }else {
                    rt_kprintf("不是控制当前采集器\r\n");
                }
                 cJSON_Delete(root);
            }
        }else {
            rt_kprintf("mqtt sub is only fomat  zy"); //当前只有中益明光科技有限公司支持反控
        }
        
        /*uint16_t data[2] = {1,1};
        data[0] = htons(data[0]);
        data[1] = htons(data[1]);
        modbus_write_registers_with_tcp(0, 1, 0, 2, data);
        rt_kprintf("modbus_write_registers_with_tcp\r\n");*/
    }

	MQTTClient_freeMessage(&message);
	MQTTClient_free(topicName);
	return 1;
}

static MQTTClient __mqtt_reconnect(rt_uint8_t index, MQTTClient client, MQTTClient_connectOptions *conn_opts)
{
    mqtt_cfg_t *mqtt_cfg = s_mqtt_cfgs[index];
    tcpip_cfg_t *tcpip_cfg = &g_tcpip_cfgs[index];
    int reconn_retry;
    int rc = 0;
    char url[256] = {0};
    reconn_retry = MQTT_RECONNECT_TIMES;
    while (reconn_retry-- && s_mqtt_run[index]) {
        rt_kprintf("__mqtt_reconnect times = %d\n", MQTT_RECONNECT_TIMES - reconn_retry);
        if (client) {
            MQTTClient_disconnect(client, 0);
            MQTTClient_destroy(&client);
            client = NULL;
        }
    	conn_opts->connectTimeout = 5;
    // create
        if (!s_mqtt_run[index]) break;
        snprintf(url, sizeof(url), "%s://%s:%u", mqtt_cfg->ssl_flag ? "ssl" : "tcp", tcpip_cfg->peer, tcpip_cfg->port);
        if ((rc = MQTTClient_create(&client, url, mqtt_cfg->client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS) {
            rt_kprintf("__mqtt_reconnect MQTTClient_create failed, rc = %d\n", rc);
            sleep(1);
            continue;
        }
    // set callbacks
        if (!s_mqtt_run[index]) break;
        if ((rc = MQTTClient_setCallbacks(client, NULL, NULL, msgarrvd, NULL)) != MQTTCLIENT_SUCCESS) {
            rt_kprintf("__mqtt_reconnect MQTTClient_setCallbacks failed, rc = %d\n", rc);
            sleep(1);
            continue;
        }
        if (!s_mqtt_run[index]) break;
        if ((rc = MQTTClient_connect(client, conn_opts)) != MQTTCLIENT_SUCCESS) {
            rt_kprintf("__mqtt_reconnect MQTTClient_connect failed, rc = %d\n", rc);
            sleep(1);
            continue;
        }
    // subscribe
        if (!s_mqtt_run[index]) break;
        if (mqtt_cfg->topic_sub) {
            if ((rc = MQTTClient_subscribe(client, mqtt_cfg->topic_sub, mqtt_cfg->qos)) != MQTTCLIENT_SUCCESS) {
                rt_kprintf("__mqtt_reconnect MQTTClient_subscribe(%s) failed, rc = %d\n", mqtt_cfg->topic_sub, rc);
                sleep(1);
        		continue;
        	}
        }
        break;
    }
    
    if (!MQTTClient_isConnected(client)) {
        if (client) {
            MQTTClient_disconnect(client, 0);
            MQTTClient_destroy(&client);
            client = NULL;
        }
        return NULL;
    }

    return client;
}

int mqtt_wait_connect(rt_uint8_t index)
{
    while (s_mqtt_state[index] != MQTT_S_CONN_OK) {
        if (MQTT_S_EXIT == s_mqtt_state[index]) break;
        sleep(1);
        rt_kprintf("mqtt_wait_connect[%d], state : %d\n", index, s_mqtt_state[index]);
    }
    return mqtt_is_connected(index);
}

int mqtt_is_connected(rt_uint8_t index)
{
    return (MQTT_S_CONN_OK == s_mqtt_state[index]);
}

int mqtt_is_exit(rt_uint8_t index)
{
    return (MQTT_S_EXIT == s_mqtt_state[index]);
}

static int log_cnt = 0;

static void __mqtt_work_task(void *parameter)
{
    rt_uint8_t index = (int)(long)parameter;
    while (1) {
        MQTTClient client = NULL;
        mqtt_cfg_t *mqtt_cfg = s_mqtt_cfgs[index];
        tcpip_cfg_t *tcpip_cfg = &g_tcpip_cfgs[index];
        mqtt_upload_t *mqtt_up = &s_mqtt_upload_data[index];
    	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    	MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
        int rc;
        int repub_retry;
        char url[256];

    // create
        snprintf(url, sizeof(url), "%s://%s:%u", mqtt_cfg->ssl_flag ? "ssl" : "tcp", tcpip_cfg->peer, tcpip_cfg->port);
        if (client == NULL) {
            if ((rc = MQTTClient_create(&client, url, mqtt_cfg->client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS) {
                rt_kprintf("MQTTClient_create failed, rc = %d\n", rc);
                goto _exit;
            }
        }
    // set callbacks
        if ((rc = MQTTClient_setCallbacks(client, (void *)(long)index, NULL, msgarrvd, NULL)) != MQTTCLIENT_SUCCESS) {
            rt_kprintf("MQTTClient_setCallbacks failed, rc = %d\n", rc);
            goto _exit;
        }
    // connect
        conn_opts.keepAliveInterval = mqtt_cfg->keepalive;
    	conn_opts.cleansession = 0;
    	conn_opts.username = ((mqtt_cfg->username && mqtt_cfg->username[0]) ? mqtt_cfg->username : NULL);
    	conn_opts.password = ((mqtt_cfg->password && mqtt_cfg->password[0]) ? mqtt_cfg->password : NULL);
    	conn_opts.connectTimeout = 20;
    	conn_opts.retryInterval = 20;
        strcpy(conn_opts.intfc, das_do_get_net_driver_name(NET_IS_ENET(index) ? DAS_NET_TYPE_ETH : DAS_NET_TYPE_GPRS, 0));
        if (mqtt_cfg->ssl_flag) {
            //ssl_opts.enableServerCertAuth = 1;
            //ssl_opts.trustStore = MQTT_CA_PATH;
            ssl_opts.sslVersion = mqtt_cfg->ssl_version;
        	conn_opts.ssl = &ssl_opts;
        }
        s_mqtt_state[index] = MQTT_S_CONN_ING;
        if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
            rt_kprintf("MQTTClient_connect failed, rc = %d\n", rc);
            sleep(5);
            if(log_cnt++ >= 20){
               elog_i("mqtt td", "connect failed:%d,s:%d", rc,das_do_is_gprs_up());
               log_cnt = 0;
            }
            goto _exit;
        }
    // subscribe
        if (mqtt_cfg->topic_sub) {
            if ((rc = MQTTClient_subscribe(client, mqtt_cfg->topic_sub, mqtt_cfg->qos)) != MQTTCLIENT_SUCCESS) {
                rt_kprintf("MQTTClient_subscribe(%s) failed, rc = %d\n", mqtt_cfg->topic_sub, rc);
        		goto _exit;
        	}
        }
        s_mqtt_state[index] = MQTT_S_CONN_OK;
        
    // loop publish
        while (s_mqtt_run[index]) {
            sleep(1);
            rt_enter_critical();
            {
                ExtData_t *node = RT_NULL;
                while (s_mqtt_run[index] && MQTTClient_isConnected(client)) {
                    node = pVarManage_GetExtDataWithUpProto(node, PROTO_DEV_NET_TYPE(index), PROTO_DEV_NET_INDEX(index), PROTO_MQTT);
                    if (node) {
                        var_double_t ext_value = 0;
                        if (node->xUp.bEnable) {
                            if (bVarManage_GetExtValue(node, node->xIo.btOutVarType, &ext_value)) {
                                if(mqtt_cfg->real_flag) {
                                    bVarManage_UpdateAvgValue(&node->xUp.xAvgUp, ext_value);
                                }
                                if(mqtt_cfg->min_flag) {
                                    bVarManage_UpdateAvgValue(&node->xUp.xAvgMin, ext_value);
                                }
                                if(mqtt_cfg->min_5_flag) {
                                    bVarManage_UpdateAvgValue(&node->xUp.xAvg5Min, ext_value);
                                }
                                if(mqtt_cfg->hour_flag) {
                                    bVarManage_UpdateAvgValue(&node->xUp.xAvgHour, ext_value);
                                }
                            }
                        }
                    } else {
                        break;
                    }
                }
            }
            rt_exit_critical();
            
            if (s_mqtt_run[index] && !MQTTClient_isConnected(client)) {
                rt_kprintf("!MQTTClient_isConnected connect lost!\n");
                s_mqtt_state[index] = MQTT_S_CONN_ING;
                client = __mqtt_reconnect(index, client, &conn_opts);
                if (client) {
                    if (!MQTTClient_isConnected(client)) {
                        rt_kprintf("mqtt_quick_reconnect failed\n");
                        goto _exit;
                    } else {
                        s_mqtt_state[index] = MQTT_S_CONN_OK;
                        rt_kprintf("mqtt_quick_reconnect ok\n");
                    }
                } else {
                    rt_kprintf("mqtt_quick_reconnect failed\n");
                    goto _exit;
                }
            }

            // 根据配置上报数据
            {
                ExtData_t *node = RT_NULL;
                if(mqtt_cfg->min_flag || mqtt_cfg->min_5_flag || mqtt_cfg->hour_flag || mqtt_cfg->real_flag) {
                    node = RT_NULL;
                    if (mqtt_cfg->real_flag) {
                        rt_time_t report_real_time = time(NULL);
                        while(1) {
                            rt_thddog_suspend("mqtt_report_real_data");
                            rt_kprintf("mqtt_report_real_data!\n\n");
                            mqtt_report_real_data(index, client, &node, report_real_time);
                            rt_thddog_resume();
                            if(!node) break;
                        }
                    }
                    if (!mqtt_cfg->sharp_flag) {
                        node = RT_NULL;
                        if (mqtt_cfg->min_flag) {
                            rt_time_t report_minutes_time = time(NULL);
                            while(1) {
                                rt_thddog_suspend("mqtt_report_minutes_data");
                                mqtt_report_minutes_data(index, client, &node, report_minutes_time);
                                rt_thddog_resume();
                                if(!node) break;
                            }
                        }
                        node = RT_NULL;
                        if (mqtt_cfg->min_5_flag) {
                            rt_time_t report_5minutes_time = time(NULL);
                            while(1) {
                                rt_thddog_suspend("mqtt_report_minutes_data");
                                mqtt_report_5minutes_data(index, client, &node, report_5minutes_time);
                                rt_thddog_resume();
                                if(!node) break;
                            }
                        }
                        node = RT_NULL;
                        if (mqtt_cfg->hour_flag) {
                            rt_time_t report_hour_time = time(NULL);
                            while(1) {
                                rt_thddog_suspend("mqtt_report_hour_data");
                                mqtt_report_hour_data(index, client, &node, report_hour_time);
                                rt_thddog_resume();
                                if(!node) break;
                            }
                        }
                    } else {
                        rt_time_t now_time = time(NULL);
                        node = RT_NULL;
                        if (mqtt_cfg->min_flag && 
                            das_time_get_day_min(mqtt_up->last_min_time) != das_time_get_day_min(now_time)) {
                            rt_kprintf("mqtt_report_minutes_data 1 mins : %u\n", now_time);
                            while(1) {
                                rt_thddog_suspend("mqtt_report_minutes_data");
                                mqtt_report_minutes_data(index, client, &node, now_time);
                                rt_thddog_resume();
                                if(!node) break;
                            }
                            mqtt_up->last_min_time = now_time;
                        }
                        node = RT_NULL;
                        if (mqtt_cfg->min_5_flag && 
                            (das_time_get_day_min(mqtt_up->last_min_5_time) / 5) != (das_time_get_day_min(now_time) / 5)) {
                            rt_kprintf("mqtt_report_minutes_data 5 mins : %u\n", now_time);
                            while(1) {
                                rt_thddog_suspend("mqtt_report_minutes_data");
                                mqtt_report_5minutes_data(index, client, &node, now_time);
                                rt_thddog_resume();
                                if(!node) break;
                            }
                            mqtt_up->last_min_5_time = now_time;
                        }
                        node = RT_NULL;
                        if (mqtt_cfg->hour_flag && 
                            das_time_get_hour(mqtt_up->last_hour_time) != das_time_get_hour(now_time)) {
                            rt_kprintf("mqtt_report_hour_data : %u\n", now_time);
                            while(1) {
                                rt_thddog_suspend("mqtt_report_hour_data");
                                mqtt_report_hour_data(index, client, &node, now_time);
                                rt_thddog_resume();
                                if(!node) break;
                            }
                            mqtt_up->last_hour_time = now_time;
                        }
                    }
                }
            }
        }

    _exit:
        if (client) {
            MQTTClient_disconnect(client, 0);
            MQTTClient_destroy(&client);
        }
        if (!s_mqtt_run[index]) {
            s_mqtt_state[index] = MQTT_S_EXIT;
            break;
        } else {
            s_mqtt_state[index] = MQTT_S_INIT;
        }
    }
    s_mqtt_work_thread[index] = RT_NULL;
    rt_thddog_unreg_inthd();
    
    pthread_exit(NULL);
}

