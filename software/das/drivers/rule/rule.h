#ifndef __RULE_H__
#define __RULE_H__

#include <stdint.h>
#include "ez_tokenizer.h"

#define RULE_NAME_SIZE      (EZ_KEYWORD_SIZE - 2)       // 输出规则使用 test_o 表示

enum rule_type {
    RULE_TYPE_ERR   = -1,
    RULE_TYPE_SYS   = 0,
    RULE_TYPE_CTRL  = 1,
    RULE_TYPE_SMS   = 2,
    RULE_TYPE_NONE  = 3,
};

enum rule_sys {
    RULE_SYS_ERR       = -1,
    RULE_SYS_REBOOT    = 0,    //重启
};

struct rule_node {
    int             enable;
    char            name[RULE_NAME_SIZE + 1];
    enum rule_type  type;
    char            *p_in;
    char            *c_in;
    char            *p_out;
    char            *c_out;
    enum rule_sys   sys;
};


void rule_init(void);
void rule_lock(void);
void rule_unlock(void);

struct rule_node *rule_new(
    int enable,
    const char *name, 
    enum rule_type type, 
    const char *p_in, 
    const char *c_in, 
    const char *p_out, 
    const char *c_out, 
    enum rule_sys sys);

int rule_del(const char *name);
struct rule_node *rule_get(const char *name);

#endif
