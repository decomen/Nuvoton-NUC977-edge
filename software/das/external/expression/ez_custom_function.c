
#include <stdio.h>
#include <stdlib.h>
#include "ez_exp_os.h"
#include "ez_custom_function.h"
#include "ez_hash.h"

#define CUSTOM_HASH_LIMIT           97

struct ez_custom_hash_node {
    char                key[EZ_KEYWORD_SIZE + 1];
    struct ez_custom_node *custom;
    struct ez_custom_hash_node *next;
};

static struct ez_custom_hash_node *s_custom_table[CUSTOM_HASH_LIMIT];

static int __get_hash(const char *str)
{
    // 31 131 1313 13131 131313 etc..
    unsigned int seed = 131;
    unsigned int hash = 0;

    while(*str) {
        hash = hash * seed + (*str++);
    }

    return (hash % CUSTOM_HASH_LIMIT);
}

struct ez_custom_node *ez_custom_new(const char *key, const char *code)
{
    if (key) {
        struct ez_custom_hash_node *new_node = NULL;
        int pos = __get_hash(key);
        struct ez_custom_hash_node *head = s_custom_table[pos];
        while (head) {
            head = head->next;
        }
        new_node = (struct ez_custom_hash_node *)malloc(sizeof(struct ez_custom_hash_node));
        if (new_node) {
            struct ez_custom_node *custom = (struct ez_custom_node *)ez_exp_malloc(sizeof(struct ez_custom_node));
            if (custom) {
                memset(custom, 0, sizeof(*custom));
                custom->code = ez_exp_strdup_ex(code, ";");
                strncpy(custom->name, key, sizeof(custom->name));
                strncpy(new_node->key, key, sizeof(new_node->key));
                new_node->custom = custom;
                new_node->next = s_custom_table[pos];
                s_custom_table[pos] = new_node;
                ez_hash_set(key, T_FUNC_CUSTOM);
            }
            return custom;
        }
    }
    
    return NULL;
}

struct ez_custom_node *ez_custom_get(const char *key)
{
    int pos = __get_hash(key);
    if (pos >= 0) {
        struct ez_custom_hash_node *node = s_custom_table[pos];
        while (node) {
            if (0 == strcmp(node->key, key)) {
                return node->custom;
            }
            node = node->next;
        }
    }
    return NULL;
}

int ez_custom_del(const char *key)
{
    int result = -1;
    int pos = __get_hash(key);
    if (pos >= 0) {
        struct ez_custom_hash_node *head = s_custom_table[pos];
        if (head) {
            struct ez_custom_hash_node *last = NULL;
            struct ez_custom_hash_node *remove = NULL;
            while (head) {
                if (0 == strcmp(head->key, key)) {
                    remove = head;
                    break;
                }
                last = head;
                head = head->next;
            }
            if (remove) {
                if (last) 
                    last->next = remove->next;
                else
                    s_custom_table[pos] = remove->next;

                if (remove->custom && remove->custom->code) ez_exp_free(remove->custom->code);
                if (remove->custom) ez_exp_free(remove->custom);
                ez_exp_free(remove);
                result = 0;
            }
        }
    }

    return result;
}

int ez_custom_set_code(struct ez_custom_node *custom, const char *code)
{
    int err = -1;
    if (custom) {
        if (custom->code) ez_exp_free(custom->code);
        custom->code = ez_exp_strdup_ex(code, ";");
        err = 0;
    }
    return err;
}

int ez_custom_set_code_ex(struct ez_custom_node *custom, const char *code, int len)
{
    int err = -1;
    if (custom) {
        if (custom->code) ez_exp_free(custom->code);
        {
            custom->code = (char *)ez_exp_malloc(len + 2);
            if (custom->code) {
                memcpy(custom->code, code, len);
                memcpy(custom->code + len, ";", 2);
            }
            //printf("custom->code = %s\n", custom->code);
        }
        err = 0;
    }
    return err;
}

int ez_custom_add_reg(struct ez_custom_node *custom, const char *name, ez_vtype type)
{
    int err = -1;

    if (custom) {
        int i;
        for (i = 0; i < EZ_EXP_REG_COUNT; i++) {
            if (!custom->regs[i]._use) {
                strncpy(custom->regs[i]._name, name, sizeof(custom->regs[i]._name));
                custom->regs[i]._use = 1;
                custom->regs[i]._type = type;
                break;
            }
        }
        err = 0;
    }
    return err;
}

