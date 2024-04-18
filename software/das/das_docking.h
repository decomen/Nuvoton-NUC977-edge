#ifndef __DAS_DOCKING_H__
#define __DAS_DOCKING_H__

#include "cpu_usage.h"

typedef struct {
    unsigned char gpio;
    unsigned char dir;   //0 input 1 output
    unsigned char val;
} s_io_t;

typedef struct {
    unsigned char gpio;
    unsigned char val;
} s_Output_t;

#define DAS_DRV_NAME_LEN            16

#define DAS_NET_TYPE_ETH            "ETH"
#define DAS_NET_TYPE_LTE            "LTE"
#define DAS_NET_TYPE_GPRS           "GPRS"
#define DAS_NET_TYPE_NBIOT          "NBIOT"

#define DAS_NET_STATUS_UP           "UP"
#define DAS_NET_STATUS_DOWN         "DOWN"

/** xx:xx:xx:xx:xx:xx */
#define DAS_NET_MAC_LEN             18
/** xxx.xxx.xxx.xxx */
#define DAS_NET_IP_LEN              16
#define DAS_NET_STATUS_LEN          64

#define DAS_DIDO_TYPE_RELAY         0
#define DAS_DIDO_TYPE_TTL           1

#define DAS_DIDO_TTL_LOW            0
#define DAS_DIDO_TTL_HIGHT          1

#define DAS_DIDO_RELAY_OFF          0
#define DAS_DIDO_RELAY_ON           1

struct das_net_list_node {
    //字串 否 ETH, LTE, GPRS, NBIOT 端口类型 （下拉选择框）
    char TYPE[DAS_DRV_NAME_LEN];
    //整型 否 编号, 从 0 开始 编号
    uint32_t INDEX;
    //字串 是 如： eth0,eth1,lte0,lte1 端口名称
    char NAME[DAS_DRV_NAME_LEN];
    //整型 是 DHCP开关  0: 关 1:开 启用DHCP (勾选框)
    uint32_t DHCP;
    //字串 是 MAC 地址 MAC 地址
    char MAC[DAS_NET_MAC_LEN];
    //字串 是 ip地址 IP地址
    char IP[DAS_NET_IP_LEN];
    //字串 是 子网掩码 子网掩码
    char MASK[DAS_NET_IP_LEN];
    //字串 是 网关 网关
    char GATEWAY[DAS_NET_IP_LEN];
    //字串 是 DNS1 DNS1
    char DNS1[DAS_NET_IP_LEN];
    //字串 是 DNS2 DNS2
    char DNS2[DAS_NET_IP_LEN];
    //字串 是 连接状态 UP / DOWM 连接状态 （UP / DOWM）
    char STATUS[DAS_NET_STATUS_LEN];
};

int das_do_get_system_resource(cpu_usage_t *res);
const char *das_do_get_net_driver_name(const char *type, uint32_t index);
const char *das_do_get_uart_driver_name(uint32_t index);
const char das_do_get_uart_parity_char(int parity);
void das_do_del_default_routes(void);
void das_do_del_route(const char *dev);
int das_do_set_net_info(const struct das_net_list_node *node);
int das_do_get_net_info(const char *type, uint32_t index, struct das_net_list_node *net);
int das_do_check_dns(const char *dns);
int das_do_io_ctrl(int ctrl, s_io_t *iodata);
int das_do_get_di_state(int index);
int das_do_set_do_state(int type, int index, int state);
int das_do_get_do_state(int type, int index);
int das_do_get_ai_value(int channel, float *value);
int das_do_open_hw_com(int index);
int das_do_set_hw_com_info(int fd, int baud_rate, int data_bits, int stop_bits, int parity);
int das_do_close_hw_com(int fd);
int das_do_is_net_up(char *type, uint32_t index);
int das_do_is_enet_up(void);
int das_do_is_gprs_up(void);

int das_do_check_enet_link(void);


int net_do_add_eth0_route();
int net_do_add_ppp0_route();

#endif

