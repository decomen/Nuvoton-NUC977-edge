
#ifndef __HJT212_H__
#define __HJT212_H__

#include "varmanage.h"

#define MQTT_INI_CFG_PATH_PREFIX         BOARD_CFG_PATH"rtu_mqtt_"

#define MQTT_BUF_SIZE         (4096)
#define MQTT_INBUF_SIZE       (2048)
#define MQTT_PARSE_STACK      (2048)      //解析任务内存占用

typedef struct {
    char *plc_name; //plc的仪表ID
    mdBYTE plc_chan;  //plc挂载的通道
    uint16_t plc_reg; //plc控制的寄存器地址
}s_plc_info_t;

typedef struct {
	char            *username;
	char            *password;
	char            *client_id;
    char            *topic_sub;
    char            *topic_pub;
    char            *sdccp_format;
    
    rt_uint32_t     keepalive;      // 心跳包间隔
	rt_uint32_t     real_interval;  // 实时数据上传间隔

	rt_uint32_t     ssl_flag    :1; // 是否启用ssl
	rt_uint32_t     ssl_version :2; // 0:default, 1:TLS1.0, 2:TLS1.1, 2:TLS1.2
    rt_uint32_t     qos         :2; // 0, 1, 2
	rt_uint32_t     retained    :1; // 
	rt_uint32_t     real_flag   :1; // 是否上传实时数据
	rt_uint32_t     min_flag    :1; // 是否上传分钟数据
	rt_uint32_t     min_5_flag  :1; // 是否上传5分钟数据
	rt_uint32_t     hour_flag   :1; // 是否上传小时数据
	rt_uint32_t     sharp_flag  :1; // 是否整点整分上报(默认0)
	rt_uint32_t     no_min      :1; // 是否上传min
	rt_uint32_t     no_max      :1; // 是否上传max
	rt_uint32_t     no_avg      :1; // 是否上传avg
	rt_uint32_t     no_cou      :1; // 是否上传cou

    mdBYTE   plc_num;
    s_plc_info_t plc_info[10];
} mqtt_cfg_t;

typedef struct {
    rt_time_t last_min_time;
    rt_time_t last_min_5_time;
    rt_time_t last_hour_time;
} mqtt_upload_t;

typedef  enum mqtt_state {
	MQTT_S_INIT = 0,
    MQTT_S_CONN_ING,
    MQTT_S_CONN_OK,
    MQTT_S_DISCON,
    MQTT_S_EXIT,
} e_mqtt_state;

void mqtt_global_init(void);

rt_bool_t mqtt_open(rt_uint8_t index);
void mqtt_close(rt_uint8_t index);

void mqtt_startwork(rt_uint8_t index);
void mqtt_exitwork(rt_uint8_t index);

int mqtt_wait_connect(rt_uint8_t index);
int mqtt_is_connected(rt_uint8_t index);
int mqtt_is_exit(rt_uint8_t index);

rt_bool_t mqtt_report_real_data(rt_uint8_t index, void *client, ExtData_t **ppnode, rt_time_t report_time);      //上传实时数据
rt_bool_t mqtt_report_minutes_data(rt_uint8_t index, void *client, ExtData_t **ppnode, rt_time_t report_time); //上传分钟数据
rt_bool_t mqtt_report_hour_data(rt_uint8_t index, void *client, ExtData_t **ppnode, rt_time_t report_time);     //上传小时数据

void mqtt_try_create_default_config_file(const char *path);

#endif

