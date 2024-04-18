
#ifndef __INI_PARSER_H__
#define __INI_PARSER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _ininode_ {
    unsigned short  hash;
    unsigned short  key;
    unsigned short  val;
} ininode_t;

typedef struct _inisection_ {
    unsigned short  n;
    unsigned short  hash;
    char            *name;          // session  名称
    ininode_t       *nodes;         // key->val 列表
    struct _inisection_ *next;      // next section
} inisection_t;

typedef struct _ini_ {
    char            *path;
    int             n;
    int             keyssize;
    int             keyspos;
    char            *keys;          // 所有 key 缓存
    int             valssize;
    int             valspos;
    char            *vals;          // 所有 val 缓存
    inisection_t    *sections;      // section 列表
} ini_t;

#ifdef __cplusplus
extern "C" {
#endif

const char * ini_getstring( const ini_t *ini, const char *section, const char *key, const char *def );
int ini_getint(const ini_t *ini, const char *section, const char *key, int def);
long ini_getlong( const ini_t *ini, const char *section, const char *key, long def );
long long ini_getlonglong( const ini_t *ini, const char *section, const char *key, long long def );
double ini_getdouble( const ini_t *ini, const char *section, const char *key, double def );
int ini_getboolean( const ini_t *ini, const char *section, const char *key, int def );
inisection_t* ini_find_section(const ini_t *ini, const char *section);
const char* ini_find_entry(const ini_t *ini, const char *section, const char *key);
ini_t * ini_load(const char * ininame);
void ini_free(ini_t * ini);

#ifdef __cplusplus
}
#endif

#endif

