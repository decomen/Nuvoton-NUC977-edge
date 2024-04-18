#include <stdio.h>
#include <stdlib.h>
#include "ez_hash.h"

#define HASH_LIMIT          727

struct ez_hash_node {
    char                key[EZ_KEYWORD_SIZE + 1];
    int                 token;
    struct ez_hash_node *next;
};

static struct ez_hash_node *s_token_table[HASH_LIMIT];

static int __get_hash(const char *str)
{
    // 31 131 1313 13131 131313 etc..
    unsigned int seed = 131;
    unsigned int hash = 0;

    while(*str) {
        hash = hash * seed + (*str++);
    }

    return (hash % HASH_LIMIT);
}

int ez_hash_init(const keyword_to_token *list)
{
    int n;
    for (n = 0; n < HASH_LIMIT; n++) {
        s_token_table[n] = NULL;
    }

    n = 0;
    while (list[n]._token != T_EOF) {
        ez_hash_set(list[n]._keyword, list[n]._token);
        n++;
    }
}

int ez_hash_set(const char *key, int token)
{
    int err = -1;
    if (key) {
        struct ez_hash_node *new_node = NULL;
        int pos = __get_hash(key);
        struct ez_hash_node *head = s_token_table[pos];
        while (head) {
            head = head->next;
            //printf("head->next\n");
        }
        new_node = (struct ez_hash_node *)malloc(sizeof(struct ez_hash_node));
        if (new_node) {
            strncpy(new_node->key, key, sizeof(new_node->key));
            new_node->token = token;
            new_node->next = s_token_table[pos];
            s_token_table[pos] = new_node;
            err = 0;
        }
    }
    
    return err;
}

int ez_hash_get(const char *key)
{
    int pos = __get_hash(key);
    if (pos >= 0) {
        struct ez_hash_node *node = s_token_table[pos];
        while (node) {
            if (0 == strcmp(node->key, key)) {
                return node->token;
            }
            node = node->next;
        }
    }
    return -1;
}

int ez_hash_del(const char *key)
{
    int err = -1;
    int pos = __get_hash(key);
    if (pos >= 0) {
        struct ez_hash_node *head = s_token_table[pos];
        if (head) {
            struct ez_hash_node *last = NULL;
            struct ez_hash_node *remove = NULL;
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
                    s_token_table[pos] = remove->next;
                
                free(remove);
                err = 0;
            }
        }
    }

    return err;
}

