#ifndef __EZ_EXP_OS_H__
#define __EZ_EXP_OS_H__

#include <stdio.h>

void *ez_exp_malloc(size_t size);
void ez_exp_free(void *p);
char *ez_exp_strdup_ex(const char *s, const char *a);
char *ez_exp_strdup(const char *s);
char *ez_exp_strndup(const char *s, int n);
void ez_exp_printf(const char* fmt, ... );
void ez_exp_putc(const char ch);
void ez_exp_putstring(const char *s);

#endif

