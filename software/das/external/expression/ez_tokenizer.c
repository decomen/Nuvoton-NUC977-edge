#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>

#include "ez_exp_os.h"
#include "ez_custom_function.h"
#include "ez_tokenizer.h"

#include "ez_hash.h"

static ez_token __tokenizer_get_next_token_ex(ez_tokenizer *tkz, ez_token tk, int load_function);

#define ez_isdigit(c) ({ int __c = (c); __c >= '0' && __c <= '9'; })
#define ez_isxdigit(c) ({ int __c = (c); ((__c >= '0' && __c <= '9') || (__c >= 'a' && __c <= 'f') || (__c >= 'A' && __c <= 'F')); })

static const unsigned char char_to_tokens[127] = {
    ['='] = T_SET_EQUAL,
    ['+'] = T_PLUS,
    ['-'] = T_MINUS,
    ['*'] = T_OP_MULTIPLY,
    ['/'] = T_OP_DIVIDE,
    ['%'] = T_OP_MOD,
    ['('] = T_LEFT_BANANA,
    [')'] = T_RIGHT_BANANA,
    ['>'] = T_OP_GREATER,
    ['<'] = T_OP_LESS,
    ['~'] = T_FUNC_BNOT,
    ['&'] = T_OP_AND,
    ['|'] = T_OP_OR,
    ['^'] = T_OP_XOR,
    ['!'] = T_FUNC_NOT,
    ['['] = T_LEFT_BRACKET,
    [']'] = T_RIGHT_BRACKET,
    ['{'] = T_LEFT_BRACES,
    ['}'] = T_RIGHT_BRACES,
    [','] = T_DOT,
    [';'] = T_SEMICOLON,
    ['\0'] = T_EOF
};

static ez_token __is_char_token(int ch)
{
    if (ch >= 0 && ch < 127) {
        return char_to_tokens[ch];
    }
    return T_NONE;
}

static const unsigned char char_with_equal_to_tokens[127] = {
    ['*'] = T_SET_MULTIPLY_NUM,
    ['/'] = T_SET_DIVIDE_NUM,
    ['%'] = T_SET_MOD_NUM,
    ['+'] = T_SET_PLUS_NUM,
    ['-'] = T_SET_MINUS_NUM,
    ['|'] = T_SET_OR_NUM,
    ['&'] = T_SET_AND_NUM,
    ['^'] = T_SET_XOR_NUM,
    ['>'] = T_OP_NOT_LESS,
    ['<'] = T_OP_NOT_GREATER,
    ['!'] = T_OP_NOT_SAME,
};

static ez_token __is_equal_char_token(int ch0, int equal)
{
    if (equal == '=' && ch0 >= 0 && ch0 < 127) {
        return char_with_equal_to_tokens[ch0];
    }
    return T_NONE;
}

static const unsigned char char_with_same_to_tokens[127] = {
    ['+'] = T_SET_DPLUS,
    ['-'] = T_SET_DMINUS,
    ['<'] = T_OP_SHIFT_LEFT,
    ['>'] = T_OP_SHIFT_RIGHT,
    ['&'] = T_OP_ANDAND,
    ['|'] = T_OP_OROR,
    ['='] = T_OP_SAME,
};

static ez_token __is_same_char_token(int ch0, int same)
{
    if (same == ch0 && ch0 >= 0 && ch0 < 127) {
        return char_with_same_to_tokens[ch0];
    }
    return T_NONE;
}

// <<= >>=
static ez_token __is_shift_num_token(int ch0, int same, int equal)
{
    if (equal == '=' && same == ch0 && (ch0 == '<' || ch0 == '>')) {
        return ch0 == '<' ? T_SET_SHIFT_LEFT_NUM : T_SET_SHIFT_RIGHT_NUM;
    }
    return T_NONE;
}

static const keyword_to_token keyword_to_tokens[] = {
    { "return",     T_RETURN },
    { "floor",      T_FUNC_FLOOR },
    { "ceil",       T_FUNC_CEIL },
    { "abs",        T_FUNC_ABS },
    { "sin",        T_FUNC_SIN },
    { "cos",        T_FUNC_COS },
    { "uint",       T_FUNC_UINT },
    { "int",        T_FUNC_INT },
    { "tan",        T_FUNC_TAN },
    { "sqr",        T_FUNC_SQR },
    { "sgn",        T_FUNC_SGN },
    { "log",        T_FUNC_LOG },
    { "exp",        T_FUNC_EXP },
    { "atn",        T_FUNC_ATN },
    { "bcd2n",      T_FUNC_BCD2N },
    
    { "minute",     T_FUNC_MINUTE },
    { "min",        T_FUNC_MIN },
    { "max",        T_FUNC_MAX },
    
    { "ifelse",     T_FUNC3_IF },

    { "if",         T_IF_FUNC_IF },
    { "else",       T_IF_FUNC_ELSE },
    { "function",   T_FUNC_FUNCTION },

    { "for",        T_FUNC_FOR },
    { "break",      T_BREAK },
    { "continue",   T_CONTINUE },
        
    { "print",      T_FUNC_PRINT },
    
    { "rand",       T_FUNC_RAND },
    { "fvar",       T_FUNC_VAR },
    { "diget",      T_FUNC_DI_GET },
    { "doset",      T_FUNC_DO_SET },
    { "doget",      T_FUNC_DO_GET },

    { "reboot",     T_FUNC_REBOOT },
    { "time",       T_FUNC_TIME },
    { "timezone",   T_FUNC_TIMEZONE },
    { "year",       T_FUNC_YEAR },
    { "month",      T_FUNC_MONTH },
    { "day",        T_FUNC_DAY },
    { "hour",       T_FUNC_HOUR },
    { "second",     T_FUNC_SECOND },
    
    { {0},          T_EOF }
};

static int keyword_chars[127] = {
    ['a'] = 1, ['b'] = 1, ['c'] = 1, ['d'] = 1, ['e'] = 1, ['f'] = 1, ['g'] = 1,
    ['h'] = 1, ['i'] = 1, ['j'] = 1, ['k'] = 1, ['l'] = 1, ['m'] = 1, ['n'] = 1,
    ['o'] = 1, ['p'] = 1, ['q'] = 1, ['r'] = 1, ['s'] = 1, ['t'] = 1, ['u'] = 1,
    ['v'] = 1, ['w'] = 1, ['x'] = 1, ['y'] = 1, ['z'] = 1,
    ['A'] = 1, ['B'] = 1, ['C'] = 1, ['D'] = 1, ['E'] = 1, ['F'] = 1, ['G'] = 1,
    ['H'] = 1, ['I'] = 1, ['G'] = 1, ['K'] = 1, ['L'] = 1, ['M'] = 1, ['N'] = 1,
    ['O'] = 1, ['P'] = 1, ['Q'] = 1, ['R'] = 1, ['S'] = 1, ['T'] = 1, ['U'] = 1,
    ['V'] = 1, ['W'] = 1, ['X'] = 1, ['Y'] = 1, ['Z'] = 1,
    
    ['0'] = 1, ['1'] = 1, ['2'] = 1, ['3'] = 1, ['4'] = 1, 
    ['5'] = 1, ['6'] = 1, ['7'] = 1, ['8'] = 1, ['9'] = 1, 

    ['_'] = 1
};

#if EZ_EXP_USE_HASH
static int __ez_hash_init_flag = 0;
#endif

void ez_tokenizer_global_init(void)
{
#if EZ_EXP_USE_HASH
    if (!__ez_hash_init_flag) {
        ez_hash_init(keyword_to_tokens);
        __ez_hash_init_flag = 1;
    }
#endif
}

void ez_tokenizer_init(char *input, ez_tokenizer *tkz)
{
    ez_token tk = T_ERROR;
    ez_tokenizer_global_init();
    tkz->line = input;
    tkz->p = tkz->next_p = tkz->line;
    tkz->size = strlen(input);
    memset(tkz->regs, 0, sizeof(tkz->regs));

    // load functions
    while (1) {
        tk = __tokenizer_get_next_token_ex(tkz, 0, 1);
        if (tk == T_EOF || tk == T_ERROR || tk == T_LOAD_FUNCTION_ERROR) break;
    }
    if (tk == T_LOAD_FUNCTION_ERROR) {
        ez_exp_printf("tokenizer load function error!\n");
        return ;
    }

    memset(tkz, 0, sizeof(*tkz));
    tkz->line = input;
    tkz->p = tkz->next_p = tkz->line;
    tkz->size = strlen(input);
    memset(tkz->regs, 0, sizeof(tkz->regs));
}

ez_token ez_tokenizer_return(ez_tokenizer *tkz)
{
    tkz->p = tkz->next_p = (tkz->line + tkz->size - 1);
    return T_EOF;
}

/*
static int __parse_reg_check(char *ch)
{
    
}
*/

static ez_token __parse_reg(char prefix, ez_tokenizer *tkz)
{
    char *_s0 = &tkz->p[1];
    char *_s1 = _s0;
    while (*tkz->p && !(isspace(*tkz->p) || __is_char_token(*tkz->p))) {
        tkz->p++;
    }
    _s1 = tkz->p;
    if (prefix == '$') {
        if ('\0' == *tkz->p) {
            tkz->error = "$xxx miss name like '$i'";
            return T_ERROR;
        } else if (_s1 - _s0 > EZ_EXP_REG_NAME_LIMIT) {
            tkz->error = "$xxx name max size:" __DEF_TO_STR(EZ_EXP_REG_NAME_LIMIT);
            return T_ERROR;
        }
    } else if (prefix == '@') {
        if ('\0' == *tkz->p) {
            tkz->error = "@xxx miss name like '@str'";
            return T_ERROR;
        } else if (_s1 - _s0 > EZ_EXP_REG_NAME_LIMIT) {
            tkz->error = "@xxx name max size:" __DEF_TO_STR(EZ_EXP_REG_NAME_LIMIT);
            return T_ERROR;
        }
    }
    {
        int i = 0;
        // search
        for (i = 0; i < EZ_EXP_REG_COUNT; i++) {
            if (tkz->regs[i]._use) {
                if (strncmp(_s0, tkz->regs[i]._name, strlen(tkz->regs[i]._name)) == 0) {
                    return T_REG_FIRST + i;
                }
            } else {
                break;
            }
        }
        if (i == EZ_EXP_REG_COUNT) {
            tkz->error = "max reg num :" __DEF_TO_STR(EZ_EXP_REG_COUNT);
            return T_ERROR;
        } else {
            // insert
            memcpy(tkz->regs[i]._name, _s0, _s1 - _s0);
            tkz->regs[i]._use = 1;
            if (prefix == '$') {
                tkz->regs[i]._type = VT_NUMBER;
            } else if (prefix == '@') {
                tkz->regs[i]._type = VT_STRING;
            }
            return T_REG_FIRST + i;
        }
    }

    return T_ERROR;
}

static void __skip_space(ez_tokenizer *tkz)
{
    while (*tkz->p && isspace(*tkz->p)) {
        if (*tkz->p == '\n') {
            tkz->ln_num++;
        }
        tkz->p++;
    }
}

static int __skip_line_comments(ez_tokenizer *tkz)
{
    while (tkz->p[0] == '/' && tkz->p[1] == '/') {
        tkz->p += 2;
        while (*tkz->p && *tkz->p != '\n') {
            tkz->p++;
        }
        if ('\n' == *tkz->p) {
            tkz->ln_num++;
            tkz->p++;
        }
        return 1;
    }
    return 0;
}

static int __skip_multiline_comments(ez_tokenizer *tkz)
{
    while (tkz->p[0] == '/' && (tkz->p[1] == '*' || tkz->p[1] == '-')) {
        char _c = tkz->p[1];
        tkz->p += 2;
        while (
            tkz->p[0] && tkz->p[1] && 
            !(_c == tkz->p[0] && '/' == tkz->p[1])) {
            if (*tkz->p == '\n') {
                tkz->ln_num++;
            }
            tkz->p++;
        }
        tkz->p += 2;
        return 1;
    }
    return 0;
}

static void __skip_function(ez_tokenizer *tkz)
{
    int left = 0, right = 0;
    while (*tkz->p) {
        __skip_space(tkz);
        while (__skip_line_comments(tkz) || __skip_multiline_comments(tkz)) {
            __skip_space(tkz);
            if ('\0' == *tkz->p) return;
        }
        while (*tkz->p && *tkz->p != '\n') {
            if (*tkz->p == '{') left++;
            if (*tkz->p == '}') right++;
            tkz->p++;
        }
        if (left > 0 && left == right) break;
    }
}

static void __load_function(ez_tokenizer *tkz)
{
    int left = 0, right = 0;
    const char *left_p = NULL, *right_p = NULL;
    __skip_space(tkz);
    if (*tkz->p) {
        char key[EZ_KEYWORD_SIZE + 1] = {0};
        int n = 0;
        const char *p = tkz->p;
        while (*p && isspace(*p)) p++;
        while (*p && !isspace(*p) && *p != '(' && n < sizeof(key)) key[n++] = *p++;
        while (*p && *p != '(') p++;
        if (*p == '(') p++;
        if (key[0]) {
            //printf("key = %s\n", key);
            struct ez_custom_node *custom = (struct ez_custom_node *)ez_custom_new(key, NULL);
            while (*p && *p != '{') {
                char name[EZ_EXP_REG_NAME_LIMIT + 1] = {0};
                n = 0;
                while (*p && *p != '{' && isspace(*p)) p++;
                while (*p && *p != '{' && !isspace(*p) && *p != ',' && *p != ')' && n < sizeof(name)) name[n++] = *p++;
                while (*p && *p != '{' && *p != ',') p++;
                if (*p == ',') p++;
                //printf("name = %s\n", name);
                if (name[0] == '$') 
                    ez_custom_add_reg(custom, &name[1], VT_NUMBER);
                else if (name[0] == '@') 
                    ez_custom_add_reg(custom, &name[1], VT_STRING);
                else {
                    ez_exp_printf("unknown type of function '%s'->'%s'\n", custom->name, name);
                    tkz->error = "unknown type of function";
                    return ;
                }
            }
            while (*tkz->p) {
                __skip_space(tkz);
                while (__skip_line_comments(tkz) || __skip_multiline_comments(tkz)) {
                    __skip_space(tkz);
                    if ('\0' == *tkz->p) return;
                }
                while (*tkz->p && *tkz->p != '\n') {
                    if (*tkz->p == '{') {
                        if (left == 0) left_p = tkz->p;
                        left++;
                    }
                    if (*tkz->p == '}') right++;
                    tkz->p++;
                }
                if (*tkz->p == '\n') tkz->ln_num++;
                if (left > 0 && left == right) {
                    right_p = tkz->p;
                    break;
                }
            }
            if (right_p > left_p) {
                ez_custom_set_code_ex(custom, left_p, right_p - left_p);
            }
        }
    }
}

static ez_token __tokenizer_get_next_token_ex(ez_tokenizer *tkz, ez_token tk, int load_function)
{
    int i;
    ez_token _t = T_NONE;
    tkz->actual_size = 0;
    tkz->actual_string = NULL;
    
    if (!*tkz->p) {
        return T_EOF;
    }

    // Skip white space
    __skip_space(tkz);
    if ('\0' == *tkz->p) return T_EOF;

    // Skip line comments
    while (__skip_line_comments(tkz) || __skip_multiline_comments(tkz)) {
        __skip_space(tkz);
        if ('\0' == *tkz->p) return T_EOF;
    }
    if ('\0' == *tkz->p) return T_EOF;

    // Check for register
    if (*tkz->p == '$' || *tkz->p == '@') {
        tkz->actual_func_token = __parse_reg(*tkz->p, tkz);
        return tkz->actual_func_token;
    }

    /** '<<=','>>=' */
    if (tkz->p[0] && tkz->p[1] && tkz->p[2]) {
        _t = __is_shift_num_token(tkz->p[0], tkz->p[1], tkz->p[2]);
        if (_t != T_NONE) {
            tkz->next_p = tkz->p + 3;
            tkz->p = tkz->next_p;
            return _t;
        }
    }

    /** '++','--','<<','>>','&&','||','==' */
    if (tkz->p[0] && tkz->p[1]) {
        _t = __is_same_char_token(tkz->p[0], tkz->p[1]);
        if (_t != T_NONE) {
            tkz->next_p = tkz->p + 2;
            tkz->p = tkz->next_p;
            return _t;
        }
    }

    /** '+=','-=','*=','/=','%=','&=','|=','!=','^=','<=','>=' */
    _t = __is_equal_char_token(tkz->p[0], tkz->p[1]);
    if (_t != T_NONE) {
        tkz->next_p = tkz->p + 2;
        tkz->p = tkz->next_p;
        return _t;
    }
    
    // read single char token
    _t = __is_char_token(*tkz->p);
    if (_t != T_NONE) {
        char _char = *tkz->p;
        tkz->actual_char = _char;
        tkz->actual_number = NAN;
        tkz->actual_token = _t;

        tkz->p++;
        tkz->next_p = tkz->p;

        return _t;
    }

    // Check for dot
    if (*tkz->p == ',') {
        tkz->next_p++;
        tkz->p = tkz->next_p;
        return T_DOT;
    }
    
    // Check for semicolon
    if (*tkz->p == ';') {
        tkz->next_p++;
        tkz->p = tkz->next_p;
        return T_SEMICOLON;
    }
    
    // Check for string
    if (*tkz->p == '\"' || *tkz->p == '\'') {
        char _c = *tkz->p;
        tkz->next_p = tkz->p + 1;
        while (*tkz->next_p && *tkz->next_p != _c) {
            tkz->next_p++;
        }
        if (*tkz->next_p) {
            tkz->next_p++;
            tkz->actual_string = tkz->p + 1;
            tkz->actual_size = tkz->next_p - tkz->p - 2;
        }
        tkz->p = tkz->next_p;
        //printf("read string : %.*s\n", tkz->actual_size, tkz->actual_string);
        return T_STRING;
    }

    // Check for #
    if (*tkz->p == '#') {
        tkz->next_p = tkz->p + 1;
        while (*tkz->next_p && !(isspace(*tkz->next_p) || __is_char_token(*tkz->next_p)) && *tkz->next_p != '@') {
            tkz->next_p++;
        }
        if (*tkz->next_p) {
            tkz->actual_string = tkz->p + 1;
            tkz->actual_size = tkz->next_p - tkz->p - 1;
        }
        tkz->p = tkz->next_p;
        return T_FUNC_VAR_ALIAS;
    }

    // Check for number
    if (isdigit(*tkz->p)) {
        char number[32];
        double d;
        int l = 0;
        if (tkz->p[0] == '0' && (tkz->p[01] == 'x' || tkz->p[1] == 'X')) {
            tkz->next_p = tkz->p + 2;
            while (*tkz->next_p && ez_isxdigit(*tkz->next_p)) {
                l++;
                tkz->next_p++;
            }
        } else {
            tkz->next_p = tkz->p;
            while (*tkz->next_p && (ez_isdigit(*tkz->next_p) || *tkz->next_p == '.')) {
                l++;
                tkz->next_p++;
            }
        }
        strncpy(number, tkz->p, sizeof(number));
        tkz->p = tkz->next_p;
        sscanf(number, "%lf", &d);
        tkz->actual_number = d;
        return T_NUMBER;
    }

    // Check for function
#if EZ_EXP_USE_HASH
    {
        int token = 0;
        char keyword[EZ_KEYWORD_SIZE + 1] = {0};
        for (i = 0; i < sizeof(keyword); i++) {
            unsigned char ch = tkz->p[i];
            if (ch <= 127 && !keyword_chars[ch]) break;
            keyword[i] = ch;
        }
        if (keyword[0]) {
            token = ez_hash_get(keyword);
            //printf("token = %s, %d\n", keyword, token);
            if (token >= 0) {
                tkz->next_p = tkz->p + i;
                tkz->p = tkz->next_p;
                tkz->actual_func_token = token;
                if (token == T_FUNC_CUSTOM) {
                    strcpy(tkz->actual_custom_keyword, keyword);
                } else if (token == T_FUNC_FUNCTION) {
                    if (load_function) {
                        __load_function(tkz);
                        if (tkz->error != NULL) {
                            return T_LOAD_FUNCTION_ERROR;
                        }
                    } else {
                        __skip_function(tkz);
                        return __tokenizer_get_next_token_ex(tkz, tk, load_function);
                    }
                    tkz->next_p = tkz->p;
                }
                return token;
            } else {
                tkz->next_p = tkz->p + i;
                tkz->p = tkz->next_p;
                tkz->actual_func_token = T_FUNC_FUNCTION;
                return T_FUNC_FUNCTION;
            }
        }
    }
#else
    for (i = 0;; i++) {
        keyword_to_token ktt = keyword_to_tokens[i];
        if (ktt._token == T_EOF) {
            break;
        }
        if (strncmp(tkz->p, ktt._keyword, strlen(ktt._keyword)) == 0) {
            tkz->next_p = tkz->p + strlen(ktt._keyword);
            tkz->p = tkz->next_p;
            tkz->actual_func_token = ktt._token;
            return ktt._token;
        }
    }
#endif
    
    return T_ERROR;
}

ez_token ez_tokenizer_get_next_token(ez_tokenizer *tkz, ez_token tk)
{
    return __tokenizer_get_next_token_ex(tkz, tk, 0);
}

