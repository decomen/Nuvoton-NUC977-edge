#ifndef __EZ_EXP_H__
#define __EZ_EXP_H__

#define EZ_EXP_UNUSED(x)   (void)x

void ez_exp_init(void);
void *ez_exp_create(void);
int ez_exp_check(void *ez_exp, char *expstring, const char **errmsg);
double ez_exp_run(void *ez_exp, char *expstring, void *param, const char **errmsg);

// free ez_exp
void ez_exp_destroy(void *ez_exp);

// not free ez_exp
void ez_exp_release(void *ez_exp);

void *ez_exp_custom_new(const char *func, const char *code);
int ez_exp_custom_del(const char *name);

double evaluate(char *exp, char *name, int *is_error);

#endif // __EXP_PARSER_H__
