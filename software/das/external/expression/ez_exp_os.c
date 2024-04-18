#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "ez_exp_os.h"

void *ez_exp_malloc(size_t size)
{
    return malloc(size);
}

char *ez_exp_strdup_ex(const char *s, const char *a)
{
    if(s) {
        int len1 = strlen(s) + 1;
        int len2 = a ? strlen(a) : 0;
        char *tmp = (char *)ez_exp_malloc(len1 + len2);
        if (!tmp) return NULL;
        memcpy(tmp, s, len1);
        if (len2 > 0) memcpy(tmp + len1 - 1, a, len2 + 1);
        return tmp;
    }
    return NULL;
}

char *ez_exp_strndup(const char *s, int n)
{
    if(s && n > 0) {
        char *tmp = (char *)malloc(n + 1);
        if (!tmp) return NULL;
        memcpy(tmp, s, n);
        tmp[n] = '\0';
        return tmp;
    }
    return NULL;
}

char *ez_exp_strdup(const char *s)
{
    if(s) {
        int len = strlen(s) + 1;
        char *tmp = (char *)malloc(len);
        if (!tmp) return NULL;
        memcpy(tmp, s, len);
        return tmp;
    }
    return NULL;
}

void ez_exp_free(void *p)
{
    if (p) free(p);
}

void ez_exp_printf(const char* fmt, ... )
{
    va_list arp;
    va_start(arp, fmt);
    vprintf(fmt, arp );
    va_end(arp);
}

void ez_exp_putc(const char ch)
{
    putchar(ch);
}

void ez_exp_putstring(const char *s)
{
    if (s) ez_exp_printf("%s", s);
}

