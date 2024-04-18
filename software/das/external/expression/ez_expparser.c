#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#include "ez_exp.h"
#include "ez_exp_os.h"
#include "ez_function0.h"
#include "ez_function1.h"
#include "ez_function2.h"
#include "ez_string_function.h"
#include "ez_custom_function.h"
#include "ez_tokenizer.h"

#define _ul(n)     (unsigned long)n

typedef struct {
    unsigned char is_break      :1;
    unsigned char is_continue   :1;
} ez_flag;

#define EZ_SKIP(_f)     ((_f)->is_break || (_f)->is_continue)
#define EZ_RETURN(_e)   ((_e)->is_return || (_e)->is_error)
#define EZ_CHECK(_e)    ((_e)->is_check)

struct ez_exp {
    unsigned char   last_sym, sym;
    unsigned char   depth, depth_max;
    unsigned char   is_check    :1;
    unsigned char   is_return   :1;
    unsigned char   is_error    :1;
    int             stack_max_use;
    ez_tokenizer    tokenizer;
    void            *param;
    double          return_value;
    //
    double          number;
    char            *string;
    //
    char            error[64];
    ez_reg          R[EZ_EXP_REG_COUNT];

    void            *p_a, *p_b;
};

static void _stack_begin(struct ez_exp *e)
{
    int p;
    if (e->p_a == NULL) e->p_a = &p;
}

static void _stack_end(struct ez_exp *e)
{
    int p;
    if (e->p_b == NULL || (unsigned long)e->p_b > (unsigned long)&p) e->p_b = &p;
}

static int _stack_used(struct ez_exp *e)
{
    return e->p_a - e->p_b;
}

static void _print_number(double num)
{
    char tmp[33], int_flag = true;
    int i = 0, j = 0;
    snprintf(tmp, 32, "%lf", isnan(num) ? 0 : num);
    for(i = 0; i < 32 && tmp[i] != '.'; i++, j++);
    for(i++; i < 32 && tmp[i] != '\0'; i++) {
        if (tmp[i] != '0') int_flag = false;
    }
    if (int_flag) {
        for(i = 0; i < j; i++) {
            ez_exp_putc(tmp[i]);
        }
    } else {
        for(i = 0; i < 32 && tmp[i] != '\0'; i++) {
            ez_exp_putc(tmp[i]);
        }
    }
}

static void _print_string(const char *s)
{
    int i;
    for(i = 0; s[i] != '\0'; i++) {
        ez_exp_putc(s[i]);
    }
}

/*
static char* __strdup(const char* str)
{
      size_t len;
      char* copy;

      len = strlen(str) + 1;
      if (!(copy = (char*)ez_exp_malloc(len))) return 0;
      memcpy(copy,str,len);
      return copy;
}
*/

static void __error(struct ez_exp *e, const char *error_msg, const char *_func, const int _line)
{
    char error[sizeof(e->error)];
    if (!e->is_return) {
        int ln_num = e->tokenizer.ln_num + 1;
        int col_num = 0;
        char *line = e->tokenizer.p;
        while (line != e->tokenizer.line && *line != '\n') {
            line--;
        }
        col_num = e->tokenizer.p - line;
        snprintf(error, sizeof(error) - 1, "exp->error:%d:%d:%s\n", ln_num, col_num, error_msg);
        error[sizeof(error) - 1] = '\0';
        if (!e->is_error) {
            strcpy(e->error, error);
            e->is_error = true;
        }
        _print_string("["); _print_string(_func); _print_string(":+"); _print_number(_line); _print_string("]");
        _print_string(error);
    }
}

#define _error(e,m) __error(e, m, __FUNCTION__, __LINE__)

static void __get_sym(struct ez_exp *e, const char *_func, const int _line)
{
    e->last_sym = e->sym;
    e->sym = ez_tokenizer_get_next_token(&e->tokenizer, T_EOF);
    if (e->sym == T_ERROR) {
        __error(e, e->tokenizer.error, _func, _line);
    }
    //printf("t: %d\n", e->sym);
}
#define _get_sym(e) __get_sym(e, __FUNCTION__, __LINE__)

#define _accept(e,t)    (t == e->sym ? (_get_sym(e), 1):0)

static bool __expect(struct ez_exp *e, ez_token t, const char *msg, const char *_func, const int _line)
{
    if (_accept(e, t)) {
        return true;
    }
    __error(e, msg ? msg : "unexpected symbol", _func, _line);
    return false;
}

#define _expect(e,t,m) __expect(e, t, m, __FUNCTION__, __LINE__)

static double _expression(struct ez_exp *e, ez_flag *flag);

#include "_expressions.c"

static double _factor(struct ez_exp *e, ez_flag *flag)
{
    if (e->sym == T_RETURN) {
        bool _banana = 0;
        _accept(e, e->sym);
        //_banana = _accept(e, T_LEFT_BANANA);
        e->number = _expression(e, flag);
        //if (_banana) _expect(e, T_RIGHT_BANANA, "return lost ')'");
        if (EZ_CHECK(e)) {
            if (e->sym != T_SEMICOLON) {
                _error(e, "return lost ';'");
            }
        } else {
            _expect(e, T_SEMICOLON, "return lost ';'");
            if (!EZ_RETURN(e)) {
                e->return_value = e->number;
                e->is_return = true;
                e->sym = ez_tokenizer_return(&e->tokenizer);
            }
        }
    } else if (e->sym == T_BREAK) {
        _accept(e, e->sym);
        _expect(e, T_SEMICOLON, "break lost ';'");
        flag->is_break = true;
    } else if (e->sym == T_CONTINUE) {
        _accept(e, e->sym);
        _expect(e, T_SEMICOLON, "continue lost ';'");
        flag->is_continue = true;
    } else if (e->sym == T_FUNC_FOR) {
        _accept(e, e->sym);
        _expression_for(e, flag);
    } else if (e->sym == T_IF_FUNC_IF) {
        _accept(e, e->sym);
        e->number = _expression_if(e, flag);
    } else if (e->sym == T_SET_DPLUS || e->sym == T_SET_DMINUS) {
        ez_token t = e->sym;
        _accept(e, e->sym);
        if (e->sym >= T_REG_FIRST && e->sym <= T_REG_LAST) {
            int n = (int)e->sym - T_REG_FIRST;
            if (!EZ_SKIP(flag)) {
                if (t == T_SET_DPLUS) {
                    e->number = ++e->R[n].val._number;
                } else if (t == T_SET_DMINUS) {
                    e->number = --e->R[n].val._number;
                }
            }
            _accept(e, e->sym);
        }
    } else if (e->sym >= T_REG_FIRST && e->sym <= T_REG_LAST) {
        int n = (int)e->sym - T_REG_FIRST;
        ez_vtype vt = e->tokenizer.regs[n]._type;
        _accept(e, e->sym);
        if (e->sym >= T_SET_FIRST && e->sym <= T_SET_LAST) {
            if (vt == VT_NUMBER) {
                e->number = _expression_set_number(n, e, flag);
            } else if (vt == VT_STRING) {
                ez_exp_free(e->string);
                e->string = ez_exp_strdup(_expression_set_string(n, e, flag));
            }
        } else {
            if (vt == VT_NUMBER) {
                e->number = e->R[n].val._number;
            } else if (vt == VT_STRING) {
                ez_exp_free(e->string);
                e->string = ez_exp_strdup(e->R[n].val._string);
            }
        }
    } else if (e->sym == T_FUNC3_IF) {
        _accept(e, e->sym);
        _expect(e, T_LEFT_BANANA, "ifelse(a,b,c) lost '('");
        if (!EZ_CHECK(e) && (EZ_SKIP(flag) || EZ_RETURN(e))) {
            _expression_skip_until_right_banana(e);
        } else {
            e->number = _expression_ternary(e, flag);
        }
        _expect(e, T_RIGHT_BANANA, "ifelse(a,b,c) lost ')'");
    } else if (e->sym == T_FUNC_CUSTOM) {
        struct ez_custom_node *custom = ez_custom_get(e->tokenizer.actual_custom_keyword);
        _accept(e, e->sym);
        _expect(e, T_LEFT_BANANA, "function(a,b,...) lost '('");
        if (!EZ_CHECK(e) && (EZ_SKIP(flag) || EZ_RETURN(e))) {
            _expression_skip_until_right_banana(e);
        } else {
            e->number = _expression_custom(custom, e, flag);
        }
        _expect(e, T_RIGHT_BANANA, "function(a,b,...) lost ')'");
    } else if (e->sym == T_FUNC_FUNCTION) {
        //printf("e->sym = %d\n", e->sym);
    } else if (e->sym == T_FUNC_MIN) {
        _accept(e, e->sym);
        _expect(e, T_LEFT_BANANA, "min(a,b,...) lost '('");
        if (!EZ_CHECK(e) && (EZ_SKIP(flag) || EZ_RETURN(e))) {
            _expression_skip_until_right_banana(e);
        } else {
            e->number = _expression_min(e, flag);
        }
        _expect(e, T_RIGHT_BANANA, "min(a,b,...) lost ')'");
    } else if (e->sym == T_FUNC_MAX) {
        _accept(e, e->sym);
        _expect(e, T_LEFT_BANANA, "max(a,b,...) lost '('");
        if (!EZ_CHECK(e) && (EZ_SKIP(flag) || EZ_RETURN(e))) {
            _expression_skip_until_right_banana(e);
        } else {
            e->number = _expression_max(e, flag);
        }
        _expect(e, T_RIGHT_BANANA, "max(a,b,...) lost ')'");
    } else if (e->sym == T_FUNC_PRINT) {
        _accept(e, e->sym);
        _expect(e, T_LEFT_BANANA, "print(a,b,...) lost '('");
        if (!EZ_CHECK(e) && (EZ_SKIP(flag) || EZ_RETURN(e))) {
            _expression_skip_until_right_banana(e);
        } else {
            e->number = _expression_print(e, flag);
        }
        _expect(e, T_RIGHT_BANANA, "print(a,b,...) lost ')'");
    } else if (e->sym == T_FUNC_BNOT) {
        _accept(e, e->sym);
        e->number = ~(_ul(e->tokenizer.actual_number));
        _accept(e, T_NUMBER);
    } else if (e->sym == T_FUNC_NOT) {
        _accept(e, e->sym);
        e->number = !_ul(_expression(e, flag));
        _accept(e, T_NUMBER);
    } else if (is_function0_token(e->sym)) {
        ez_token function_sym = e->sym;
        _accept(e, e->sym);
        _expect(e, T_LEFT_BANANA, "fun() lost '('");
        e->number = get_function0(function_sym)();
        _expect(e, T_RIGHT_BANANA, "fun() lost ')'");
    } else if (is_function1_token(e->sym)) {
        ez_token function_sym = e->sym;
        _accept(e, e->sym);
        _expect(e, T_LEFT_BANANA, "fun(num) lost '('");
        if (!EZ_CHECK(e) && (EZ_SKIP(flag) || EZ_RETURN(e))) {
            _expression_skip_until_right_banana(e);
        } else {
            function_1 func = get_function1(function_sym);
            e->number = func(_expression(e, flag));
        }
        _expect(e, T_RIGHT_BANANA, "fun(num) lost ')'");
    } else if (is_function2_token(e->sym)) {
        ez_token function_sym = e->sym;
        _accept(e, e->sym);
        _expect(e, T_LEFT_BANANA, "fun(num1,num2) lost '('");
        if (!EZ_CHECK(e) && (EZ_SKIP(flag) || EZ_RETURN(e))) {
            _expression_skip_until_right_banana(e);
        } else {
            function_2 func = get_function2(function_sym);
            double number1 = _expression(e, flag);
            _expect(e, T_DOT, "fun(num1,num2) lost ','");
            double number2 = _expression(e, flag);
            e->number = func(number1, number2);
        }
        _expect(e, T_RIGHT_BANANA, "fun(num1,num2) lost ')'");
    } else if (is_functionstring_token(e->sym)) {
        ez_token function_sym = e->sym;
        _accept(e, e->sym);
        _expect(e, T_LEFT_BANANA, "fun(string) lost '('");
        if (!EZ_CHECK(e) && (EZ_SKIP(flag) || EZ_RETURN(e))) {
            _expression_skip_until_right_banana(e);
        } else {
            function_string func = get_functionstring(function_sym);
            if (e->sym >= T_REG_FIRST && e->sym <= T_REG_LAST) {
                int n = (int)e->sym - T_REG_FIRST;
                ez_vtype vt = e->tokenizer.regs[n]._type;
                if (vt == VT_STRING) {
                    if (e->R[n].val._string) {
                        e->number = func((const char *)e->R[n].val._string);
                    } else {
                        e->number = func((const char *)e->param);
                    }
                } else {
                    __error(e, "function(@string) just support string params", __FUNCTION__, __LINE__);
                }
            } else if (e->sym == T_STRING) {
                if (e->tokenizer.actual_string && e->tokenizer.actual_size > 0) {
                    char *tmp = ez_exp_strndup((const char *)e->tokenizer.actual_string, e->tokenizer.actual_size);
                    e->number = func((const char *)tmp);
                    ez_exp_free(tmp);
                } else {
                    e->number = func((const char *)e->param);
                }
            } else {    // no string 
                e->number = func((const char *)e->param);
            }
            if(e->sym != T_RIGHT_BANANA) {
                e->sym = ez_tokenizer_get_next_token(&e->tokenizer, T_RIGHT_BANANA);
            }
        }
        _expect(e, T_RIGHT_BANANA, "fun(string) lost ')'");
    } else if (e->sym == T_FUNC_VAR_ALIAS) {
        extern double _getvar(const char *name);
        char *name = ez_exp_strndup((const char *)e->tokenizer.actual_string, e->tokenizer.actual_size);
        _accept(e, e->sym);
        if (name && name[0]) {
            if (e->sym >= T_SET_FIRST && e->sym <= T_SET_LAST) {
                /*if (vt == VT_NUMBER) {
                    e->number = _expression_set_number(n, e, flag);
                } else if (vt == VT_STRING) {
                    ez_exp_free(e->string);
                    e->string = ez_exp_strdup(_expression_set_string(n, e, flag));
                }*/
            } else {
                e->number = _getvar((const char *)name);
            }
        } else {
            if (e->sym >= T_REG_FIRST && e->sym <= T_REG_LAST) {
                int n = (int)e->sym - T_REG_FIRST;
                ez_vtype vt = e->tokenizer.regs[n]._type;
                if (vt == VT_STRING) {
                    if (e->R[n].val._string) {
                        e->number = _getvar((const char *)e->R[n].val._string);
                    } else {
                        e->number = _getvar((const char *)e->param);
                    }
                } else {
                    __error(e, "just support #string", __FUNCTION__, __LINE__);
                }
                _accept(e, e->sym);
            } else {
                e->number = _getvar((const char *)e->param);
            }
        }
        ez_exp_free(name);
    } else if (e->sym == T_NUMBER) {
        e->number = e->tokenizer.actual_number;
        _accept(e, T_NUMBER);
    } else if (e->sym == T_SEMICOLON) {
        _accept(e, T_SEMICOLON);
    } else if (_accept(e, T_LEFT_BANANA)) {
        if (!EZ_CHECK(e) && (EZ_SKIP(flag) || EZ_RETURN(e))) {
            _expression_skip_until_right_banana(e);
        } else {
            e->number = _expression(e, flag);
        }
        _expect(e, T_RIGHT_BANANA, "() lost ')'");
    } else if (_accept(e, T_LEFT_BRACES)) {
        if (!EZ_CHECK(e) && (EZ_SKIP(flag) || EZ_RETURN(e))) {
            _expression_skip_until_right_braces(e);
        } else {
            e->number = _expression_comma(e, flag);
        }
        _expect(e, T_RIGHT_BRACES, "{a;b;...} lost '}'");
    } else if (e->sym == T_EOF) {
        ;
    } else {
        _error(e ,"Factor: syntax error");
        _get_sym(e);
    }
    return e->number;
}

static double _term(struct ez_exp *e, ez_flag *flag)
{
    double f1 = _factor(e, flag);
    while (e->sym >= T_OP_FIRST && e->sym <= T_OP_LAST) {
        ez_token t = e->sym;
        _get_sym(e);
        double f2 = _factor(e, flag);
        if (!EZ_SKIP(flag) && !EZ_RETURN(e)) {
            switch (t) {
            case T_OP_SHIFT_LEFT:       f1 = (_ul(f1) << _ul(f2)); break;
            case T_OP_SHIFT_RIGHT:      f1 = (_ul(f1) >> _ul(f2)); break;
            case T_OP_MOD:              f1 = (_ul(f1) % _ul(f2)); break;
            case T_OP_AND:              f1 = (_ul(f1) & _ul(f2)); break;
            case T_OP_XOR:              f1 = (_ul(f1) ^ _ul(f2)); break;
            case T_OP_OR:               f1 = (_ul(f1) | _ul(f2)); break;
            
            case T_OP_NOT_LESS:         f1 = (f1 >= f2); break;
            case T_OP_NOT_GREATER:      f1 = (f1 <= f2); break;
            case T_OP_SAME:             f1 = (f1 == f2); break;
            case T_OP_NOT_SAME:         f1 = (f1 != f2); break;
            case T_OP_ANDAND:           f1 = (f1 && f2); break;
            case T_OP_OROR:             f1 = (f1 || f2); break;
            case T_OP_MULTIPLY:         f1 = (f1 * f2); break;
            case T_OP_DIVIDE:           f1 = (f1 / f2); break;
            case T_OP_GREATER:          f1 = (f1 > f2); break;
            case T_OP_LESS:             f1 = (f1 < f2); break;
            default:    _error(e, "term: oops");
            }
        }
        if (EZ_RETURN(e)) goto _exit;
    }

_exit:
    return f1;
}

static double _expression(struct ez_exp *e, ez_flag *flag)
{
    unsigned char t = T_PLUS;
    double f1 = 0;
    
    e->depth += 1;
    if (e->depth_max < e->depth) e->depth_max = e->depth;
    if (e->sym == T_PLUS || e->sym == T_MINUS) {
        t = e->sym;
        _get_sym(e);
    }
    f1 = _term(e, flag);
    if (t == T_MINUS) {
        f1 = -1 * f1;
    }
    while (e->sym == T_PLUS || e->sym == T_MINUS) {
        t = e->sym;
        _get_sym(e);
        double f2 = _term(e, flag);
        if (!EZ_SKIP(flag)) {
            switch (t) {
            case T_PLUS:    f1 = f1 + f2; break;
            case T_MINUS:   f1 = f1 - f2; break;
            default:        _error(e, "expression: oops");
            }
        }
        if (e->is_return) goto _exit;
    }

_exit:
    e->depth -= 1;
    _stack_end(e);
    return f1;
}

void ez_exp_init(void)
{
    ez_tokenizer_global_init();
}

void *ez_exp_create(void)
{
    struct ez_exp *e = (struct ez_exp *)ez_exp_malloc(sizeof(struct ez_exp));
    return e;
}

void ez_exp_release(void *ez_exp)
{
    if (ez_exp) {
        struct ez_exp *e = (struct ez_exp *)ez_exp;
        int i;
        for (i = 0; i < EZ_EXP_REG_COUNT; i++) {
            if (e->R[i].type == VT_STRING) {
                ez_exp_free(e->R[i].val._string);
            }
        }
        ez_exp_free(e->string);
    }
}

void ez_exp_destroy(void *ez_exp)
{
    if (ez_exp) {
        ez_exp_release(ez_exp);
        ez_exp_free(ez_exp);
    }
}

int ez_exp_check(void *ez_exp, char *expstring, const char **errmsg)
{
    if (ez_exp) {
        double result = 0; EZ_EXP_UNUSED(result);
        ez_flag flag = {false, false};
        struct ez_exp *e = (struct ez_exp *)ez_exp;
        memset(e, 0, sizeof(*e));
        _stack_begin(e);
        e->is_check = true;
        ez_tokenizer_init(expstring, &e->tokenizer);
        _get_sym(e);
        result =  _expression_comma(e, &flag);
        _expect(e, T_EOF, NULL);
        if (e->is_error) {
            if(errmsg) *errmsg = e->error;
        } else {
            if(errmsg) *errmsg = NULL;
        }
        //printf("%d,%d\n", e->depth_max, _stack_used(e));
        return e->is_error ? false : true;
    } else {
        *errmsg = "error param!\n";
        return false;
    }
}

double ez_exp_run(void *ez_exp, char *expstring, void *param, const char **errmsg)
{
    if (ez_exp) {
        double result = 0;
        ez_flag flag = {false, false};
        struct ez_exp *e = (struct ez_exp *)ez_exp;
        memset(e, 0, sizeof(*e));
        _stack_begin(e);
        e->param = param;
        ez_tokenizer_init(expstring, &e->tokenizer);
        if (e->tokenizer.error != NULL) {
            ;
        } else {
            _get_sym(e);
            result =  _expression_comma(e, &flag);
            if (e->is_return) {
                result = e->return_value;
            } else {
                _expect(e, T_EOF, NULL);
            }
            if (e->is_error) {
                if(errmsg) *errmsg = e->error;
            } else {
                if(errmsg) *errmsg = NULL;
            }
            e->stack_max_use = _stack_used(e);
        }
        return result;
    } else {
        *errmsg = "error param!\n";
        return 0;
    }
}

void *ez_exp_custom_new(const char *func, const char *code)
{
    struct ez_custom_node *custom = NULL;
    char key[EZ_KEYWORD_SIZE + 1] = {0};
    int n = 0;
    const char *p = func;
    while (*p && isspace(*p)) p++;
    while (*p && !isspace(*p) && *p != '(' && n < sizeof(key)) key[n++] = *p++;
    while (*p && *p != '(') p++;
    if (*p == '(') p++;
    if (key[0]) {
        //printf("key = %s\n", key);
        struct ez_custom_node *custom = ez_custom_new(key, code);
        while (*p && isspace(*p)) p++;
        if (*p != ')') {
            while (*p) {
                char name[EZ_EXP_REG_NAME_LIMIT + 1] = {0};
                n = 0;
                while (*p && isspace(*p)) p++;
                while (*p && !isspace(*p) && *p != ',' && *p != ')' && n < sizeof(name)) name[n++] = *p++;
                while (*p && *p != ',') p++;
                if (*p == ',') p++;
                //printf("name = %s\n", name);
                if (name[0] == '$') 
                    ez_custom_add_reg(custom, &name[1], VT_NUMBER);
                else if (name[0] == '@') 
                    ez_custom_add_reg(custom, &name[1], VT_STRING);
                else
                    ez_exp_printf("unknown type of %s\n", name);
            }
        }
    }
}

int ez_exp_custom_del(const char *name)
{
    return ez_custom_del(name);
}

double evaluate(char *exp, char *name, int *is_error)
{
    double val = 0;
    struct ez_exp e;
    val = ez_exp_run(&e, exp, (void *)name, NULL);
    if (is_error) *is_error = e.is_error;
    ez_exp_release(&e);
    return val;
}


