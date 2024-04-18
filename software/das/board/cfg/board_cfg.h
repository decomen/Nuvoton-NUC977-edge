
#ifndef _BOARD_CFG_H__
#define _BOARD_CFG_H__

#define CFG_VER                 1

#define CFG_INFO_NAME           "info"
#define REG_CFG_NAME            "reg"
#define UART_CFG_NAME           "uart"
#define NET_CFG_NAME            "net"
#define GPRS_NET_CFG_NAME       "gprs_net"
#define GPRS_WORK_CFG_NAME      "gprs_work"
#define TCPIP_CFG_NAME          "tcpip"
#define DI_CFG_NAME             "di"
#define DO_CFG_NAME             "do"
#define ANALOG_CFG_NAME         "ai"
#define AUTH_CFG_NAME           "auth"
#define STORAGE_CFG_NAME        "storage"
#define HOST_CFG_NAME           "host"
#define ZIGBEE_CFG_NAME         "zigbee"
#define XFER_UART_CFG_NAME      "xfer"
#define LORA_CFG_NAME           "lora"
#define MONITOR_CFG_NAME        "monitor"

typedef struct {
    rt_uint8_t usVer;

    //...
} cfg_info_t;

extern cfg_info_t g_cfg_info;

void board_cfg_init( void );
void board_cfg_uinit(void);
void board_cfg_del_all(void);
rt_bool_t board_cfg_del_one(const char *type);
rt_bool_t board_cfg_read(const char *type, void *pcfg, int len );
rt_bool_t board_cfg_write(const char *type, void const *pcfg, int len );

rt_bool_t board_cfg_varext_loadlist( ExtDataList_t *pList );
rt_bool_t board_cfg_varext_del(const char *name);
rt_bool_t board_cfg_varext_del_all(void);
rt_bool_t board_cfg_varext_update(const char *name, ExtData_t *data);
rt_bool_t board_cfg_varext_add(ExtData_t *data);

rt_bool_t board_cfg_rule_loadlist(void);
rt_bool_t board_cfg_rule_del(const char *name);
rt_bool_t board_cfg_rule_del_all(void);
rt_bool_t board_cfg_rule_update(const char *name, struct rule_node *rule);
rt_bool_t board_cfg_rule_add(struct rule_node *rule);

rt_bool_t cfg_recover_with_json(const char *path);

#endif

