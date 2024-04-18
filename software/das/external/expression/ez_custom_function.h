#ifndef __EZ_CUSTOM_FUNCTION_H__
#define __EZ_CUSTOM_FUNCTION_H__

#include "ez_tokenizer.h"

struct ez_custom_node {
    char                name[EZ_KEYWORD_SIZE + 1];
    reg_name_to_token   regs[EZ_EXP_REG_COUNT];
    char *code;
};

struct ez_custom_node *ez_custom_new(const char *key, const char *code);
struct ez_custom_node *ez_custom_get(const char *key);
int ez_custom_del(const char *key);
int ez_custom_set_code(struct ez_custom_node *custom, const char *code);
int ez_custom_set_code_ex(struct ez_custom_node *custom, const char *code, int len);
int ez_custom_add_reg(struct ez_custom_node *custom, const char *name, ez_vtype type);

#endif



