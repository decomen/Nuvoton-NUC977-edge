
/*-------------------------------------------------------------------------*/
/**
   @file    ini.c
*/
/*--------------------------------------------------------------------------*/
/*---------------------------- Includes ------------------------------------*/
#include <ctype.h>
#include <stdarg.h>
#include "iniparser.h"
#include <rtdef.h>

#define ASCIILINESZ         (512)
#define INI_INVALID_KEY     NULL

#define _xmalloc        rt_malloc
#define _xcalloc        rt_calloc
#define _xrealloc       rt_realloc
#define _xfree          rt_free
#define _xisspace(c)    ((c)==' '||(c)=='\f'||(c)=='\n'||(c)=='\r'||(c)=='\t'||(c)=='\v')

typedef enum _line_status_ {
    LINE_UNPROCESSED,
    LINE_ERROR,
    LINE_EMPTY,
    LINE_COMMENT,
    LINE_SECTION,
    LINE_VALUE
} line_status;


static void __free(ini_t *ini);
static ini_t* __load(ini_t *ini, const char *ininame);

static inisection_t* __find_section(const ini_t *ini, const char *section);
static const char* __find_val(const ini_t *ini, const char *section, const char *key);

static unsigned short __ini_hash(const char *key)
{
    unsigned int seed = 131;
    unsigned int hash = 0;
    char *str = (char *)key;

    while (*str) {
        hash = hash * seed + (*str++);
    }

    return (hash % 65521);
}

static char* __xstrdup(const char *s)
{
    char *t;
    size_t len;
    if (!s) return NULL;

    len = strlen(s) + 1;
    t = (char *)_xmalloc(len);
    if (t) {
        memcpy(t, s, len);
    }
    return t;
}

static const char* __strlwc(const char *in, char *out, unsigned len)
{
    unsigned i;

    if (in == NULL || out == NULL || len == 0) return NULL;
    i = 0;
    while (in[i] != '\0' && i < len - 1) {
        out[i] = (char)tolower((int)in[i]);
        i++;
    }
    out[i] = '\0';
    return out;
}

static unsigned __strstrip(char *s)
{
    char *last = NULL;
    char *dest = s;

    if (s == NULL) return 0;

    last = s + strlen(s);
    while (_xisspace((int)*s) && *s) s++;
    while (last > s) {
        if (!_xisspace((int)*(last - 1))) break;
        last--;
    }
    *last = (char)0;

    memmove(dest, s, last - s + 1);
    return (last - s);
}

const char* ini_getstring(const ini_t *ini, const char *section, const char *key, const char *def)
{
    const char *sval = __find_val(ini, section, key);
    if (!sval) return def;
    return sval;
}

int ini_getint(const ini_t *ini, const char *section, const char *key, int def)
{
    return (int)(ini_getlong(ini,section,key,def));
}

long ini_getlong(const ini_t *ini, const char *section, const char *key, long def)
{
    const char *sval = __find_val(ini, section, key);
    if (!sval) return def;
    return (strtol(sval, NULL, 0));
}

long long ini_getlonglong(const ini_t *ini, const char *section, const char *key, long long def)
{
    const char *sval = __find_val(ini, section, key);
    if (!sval) return def;
    return (strtoll(sval, NULL, 0));
}

double ini_getdouble(const ini_t *ini, const char *section, const char *key, double def)
{
    const char *sval = __find_val(ini, section, key);
    if (!sval) return def;
    return (atof(sval));
}

int ini_getboolean(const ini_t *ini, const char *section, const char *key, int def)
{
    int          ret;
    const char *c = __find_val(ini, section, key);
    if (!c) return def;

    if (c[0] == 'y' || c[0] == 'Y' || c[0] == '1' || c[0] == 't' || c[0] == 'T') {
        ret = 1;
    } else if (c[0] == 'n' || c[0] == 'N' || c[0] == '0' || c[0] == 'f' || c[0] == 'F') {
        ret = 0;
    } else {
        ret = def;
    }
    return ret;
}

inisection_t* ini_find_section(const ini_t *ini, const char *section)
{
    return __find_section(ini, section);
}

const char* ini_find_entry(const ini_t *ini, const char *section, const char *key)
{
    return __find_val(ini, section, key);
}

static line_status _parse_line(char *line, char *section, char *key, char *value)
{
    line_status sta;
    size_t      len;

    len = __strstrip(line);

    sta = LINE_UNPROCESSED;
    if (len < 1) {
        sta = LINE_EMPTY;
    } else if (line[0] == '#' || line[0] == ';') {
        sta = LINE_COMMENT;
    } else if (line[0] == '[' && line[len - 1] == ']') {
        sscanf(line, "[%[^]]", section);
        __strstrip(section);
        __strlwc(section, section, len);
        sta = LINE_SECTION;
    } else if (sscanf(line, "%[^=] = \"%[^\"]\"", key, value) == 2
               ||  sscanf(line, "%[^=] = '%[^\']'",   key, value) == 2) {
        __strstrip(key);
        __strlwc(key, key, len);
        sta = LINE_VALUE;
    } else if (sscanf(line, "%[^=] = %[^;#]", key, value) == 2) {
        __strstrip(key);
        __strlwc(key, key, len);
        __strstrip(value);
        if (!strcmp(value, "\"\"") || (!strcmp(value, "''"))) {
            value[0] = 0;
        }
        sta = LINE_VALUE;
    } else if (sscanf(line, "%[^=] = %[;#]", key, value) == 2
               ||  sscanf(line, "%[^=] %[=]", key, value) == 2) {
        __strstrip(key);
        __strlwc(key, key, len);
        value[0] = 0;
        sta = LINE_VALUE;
    } else {
        sta = LINE_ERROR;
    }
    return sta;
}

static inisection_t* __find_section(const ini_t *ini, const char *section)
{
    char *lc_section = __xstrdup(section);
    inisection_t *ps = ini->sections;

    if (lc_section) {
        unsigned short hash;
        __strstrip(lc_section);
        __strlwc(lc_section, lc_section, strlen(lc_section) + 1);
        hash = __ini_hash(lc_section);
        while (ps) {
            if (ps->hash == hash) {
                if (ps->name && !strcmp(lc_section, ps->name)) {
                    _xfree(lc_section);
                    return ps;
                }
            }
            ps = ps->next;
        }
    }
    _xfree(lc_section);
    return NULL;
}

static const char* __find_val(const ini_t *ini, const char *section, const char *key)
{
    inisection_t *ps = __find_section(ini, section);
    if (ps) {
        int i;
        unsigned short hash;
        char *lc_key = __xstrdup(key);
        __strstrip(lc_key);
        __strlwc(lc_key, lc_key, strlen(lc_key) + 1);
        hash = __ini_hash(lc_key);
        for (i = 0; i < ps->n; i++) {
            if (ps->nodes[i].hash == hash) {
                if ((ps->nodes[i].key > 0) && !strcmp(lc_key, &ini->keys[ps->nodes[i].key])) {
                    _xfree(lc_key);
                    return (&ini->vals[ps->nodes[i].val]);
                }
            }
        }
        _xfree(lc_key);
    }
    return NULL;
}

static int __stat_ini(ini_t *ini, FILE *in,  char *line, char *section, char *key, char *val)
{
    int  last = 0;
    int  len;
    int  errs = 0;
    inisection_t *pinisection = NULL;

    rewind(in);
    while (fgets(line + last, ASCIILINESZ - last, in) != NULL) {
        len = (int)strlen(line) - 1;
        if (len <= 0) continue;
        /* Safety check against buffer overflows */
        if (line[len] != '\n' && !feof(in)) {
            return (-1);
        }

        while ((len >= 0) && ((line[len] == '\n') || (_xisspace(line[len])))) {
            line[len] = 0;
            len--;
        }
        if (len < 0) {
            len = 0;
        }
        if (line[len] == '\\') {
            last = len;
            continue;
        } else {
            last = 0;
        }
        switch (_parse_line(line, section, key, val)) {
        case LINE_EMPTY:
        case LINE_COMMENT:
            break;

        case LINE_SECTION:
        {
            pinisection = __find_section(ini, section);
            if (!pinisection) {
                inisection_t *ptmps = ini->sections;
                pinisection = _xmalloc(sizeof(inisection_t));
                pinisection->n = 0;
                pinisection->hash = __ini_hash(section);
                pinisection->name = __xstrdup(section);
                pinisection->nodes = NULL;
                pinisection->next = NULL;
                while (ptmps && ptmps->next) ptmps = ptmps->next;
                if (!ptmps) {
                    ini->sections = pinisection;
                } else {
                    ptmps->next = pinisection;
                }
                ini->n++;
            }
            break;
        }

        case LINE_VALUE:
            if (pinisection) {
                pinisection->n++;
            }
            ini->keyssize += (strlen(key) + 1);
            ini->valssize += (strlen(val) + 1);
            break;

        case LINE_ERROR:
            errs++;
            break;

        default:
            break;
        }
        memset(line, 0, ASCIILINESZ);
        last = 0;
    }
    if (errs) {
        return (-1);
    }
    return 0;
}

static int __fill_ini(ini_t *ini, FILE *in,  char *line, char *section, char *key, char *val)
{
    int  last = 0;
    int  len;
    int  errs = 0;
    int  ndindex = 0;
    inisection_t *pinisection = NULL;

    rewind(in);
    while (fgets(line + last, ASCIILINESZ - last, in) != NULL) {
        len = (int)strlen(line) - 1;
        if (len <= 0) continue;
        /* Safety check against buffer overflows */
        if (line[len] != '\n' && !feof(in)) {
            return (-1);
        }

        while ((len >= 0) && ((line[len] == '\n') || (_xisspace(line[len])))) {
            line[len] = 0;
            len--;
        }
        if (len < 0) {
            len = 0;
        }
        if (line[len] == '\\') {
            last = len;
            continue;
        } else {
            last = 0;
        }
        switch (_parse_line(line, section, key, val)) {
        case LINE_EMPTY:
        case LINE_COMMENT:
            break;

        case LINE_SECTION:
            pinisection = __find_section(ini, section);
            if (pinisection && pinisection->nodes == NULL) {
                pinisection->nodes = _xcalloc(sizeof(ininode_t), pinisection->n);
                ndindex = 0;
            }
            break;

        case LINE_VALUE:
            if (pinisection && pinisection->nodes) {
                int keylen = strlen(key) + 1;
                int vallen = strlen(val) + 1;

                pinisection->nodes[ndindex].hash = __ini_hash(key);
                memcpy(&ini->keys[ini->keyspos], key, keylen);
                pinisection->nodes[ndindex].key = ini->keyspos;
                memcpy(&ini->vals[ini->valspos], val, vallen);
                pinisection->nodes[ndindex].val = ini->valspos;
                ndindex++;
                ini->keyspos += keylen;
                ini->valspos += vallen;
            }
            break;

        case LINE_ERROR:
            errs++;
            break;

        default:
            break;
        }
        memset(line, 0, ASCIILINESZ);
        last = 0;
    }
    if (errs) {
        return (-1);
    }
    return 0;
}

static ini_t* __load(ini_t *ini, const char *ininame)
{
    FILE *in       = NULL;
    char *line      = _xcalloc(1, ASCIILINESZ + 1);
    char *section   = _xcalloc(1, ASCIILINESZ + 1);
    char *key       = _xcalloc(1, ASCIILINESZ + 1);
    char *val       = _xcalloc(1, ASCIILINESZ + 1);

    if ((in = fopen(ininame, "r")) == NULL) {
        goto _ERR;
    }

    memset( ini, 0, sizeof(ini_t) );
    if (__stat_ini(ini, in, line, section, key, val) != 0) {
        goto _ERR;
    }
    if (ini->keyssize > 0) {
        ini->keys = _xcalloc(1, ini->keyssize + 1);
    }
    if (ini->valssize > 0) {
        ini->vals = _xcalloc(1, ini->valssize + 1);
    }
    ini->keyspos = 1;
    ini->valspos = 1;
    if (__fill_ini(ini, in, line, section, key, val) != 0) {
        goto _ERR;
    }

    goto _END;
_ERR:
    if (ini) ini_free(ini);
    ini = NULL;
_END:
    _xfree(line); _xfree(section); _xfree(key); _xfree(val);
    if (in) fclose(in);
    return ini;
}

ini_t* ini_load(const char *ininame)
{
    ini_t *ini = _xcalloc(sizeof(ini_t), 1);
    if (ini) ini = __load(ini, ininame);
    if (ini) ini->path = __xstrdup(ininame);
    return ini;
}

static void __free(ini_t *ini)
{
    if (ini == NULL) {
        return;
    } else {
        inisection_t *ps = ini->sections;

        while (ps) {
            inisection_t *next = ps->next;
            if (ps->name) _xfree(ps->name);
            if (ps->nodes) _xfree(ps->nodes);
            _xfree(ps);
            ps = next;
        }

        _xfree(ini->keys);
        _xfree(ini->vals);
    }
    return;
}

void ini_free(ini_t *ini)
{
    if (ini == NULL) {
        return;
    } else {
        __free(ini);
        _xfree(ini->path);
        _xfree(ini);
    }
    return;
}

