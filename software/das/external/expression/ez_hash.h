#ifndef __EZ_HASH_H__
#define __EZ_HASH_H__

#include "ez_tokenizer.h"

int ez_hash_init(const keyword_to_token *list);
int ez_hash_set(const char *key, int token);
int ez_hash_get(const char *key);
int ez_hash_del(const char *key);

#endif

