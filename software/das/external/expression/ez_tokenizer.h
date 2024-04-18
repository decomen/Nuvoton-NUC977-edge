#ifndef __EZ_TOKENIZER_H__
#define __EZ_TOKENIZER_H__

#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#include "ez_exp.h"
#include "ez_exp_config.h"

#define EZ_KEYWORD_SIZE     (32)

typedef enum {
    T_NONE          = 0,   
    T_RETURN        = 1,   
    T_PLUS          = 2,   
    T_MINUS         = 3,   
    T_LEFT_BANANA   = 4,   /* ( */
    T_RIGHT_BANANA  = 5,   /* ) */
    T_LEFT_BRACKET  = 6,   /* [ */
    T_RIGHT_BRACKET = 7,   /* ] */
    T_LEFT_BRACES   = 8,   /* { */
    T_RIGHT_BRACES  = 9,   /* } */
    T_DOT           = 10,  /* , */
    T_SEMICOLON     = 11,  /* ; */
    T_STRING        = 12,  /* " or ' */
    T_NUMBER        = 13,
    T_BREAK         = 14,
    T_CONTINUE      = 15,

    T_FUNC_FLOOR    = 20,
    T_FUNC_CEIL     = 21,
    T_FUNC_ABS      = 22,
    T_FUNC_SIN      = 23,
    T_FUNC_COS      = 24,
    T_FUNC_INT      = 25,
    T_FUNC_UINT     = 26,
    T_FUNC_TAN      = 27,
    T_FUNC_SQR      = 28,
    T_FUNC_SGN      = 29,
    T_FUNC_LOG      = 30,
    T_FUNC_EXP      = 31,
    T_FUNC_ATN      = 32,
    T_FUNC_BCD2N    = 33,
    T_FUNC_BNOT     = 34,
    T_FUNC_NOT      = 35,
    T_FUNC_PRINT    = 36,
    T_FUNC_RAND     = 37,
    
    T_FUNC_VAR      = 38,
    T_FUNC_DI_GET   = 39,
    T_FUNC_DO_SET   = 40,
    T_FUNC_DO_GET   = 41,
    
    T_FUNC_REBOOT   = 42,
    T_FUNC_TIME     = 43,
    T_FUNC_TIMEZONE = 44,
    T_FUNC_YEAR     = 45,
    T_FUNC_MONTH    = 46,
    T_FUNC_DAY      = 47,
    T_FUNC_HOUR     = 48,
    T_FUNC_MINUTE   = 49,
    T_FUNC_SECOND   = 50,
    
    T_FUNC_VAR_ALIAS  = 51,  // #

    /** infinite parameter expression */
    T_FUNC_MIN      = 80,
    T_FUNC_MAX      = 81,
    
    /** three parameter expressions */
    T_FUNC3_IF      = 82,
    T_FUNC_FOR      = 83,

    /** if else expressions */
    T_IF_FUNC_IF    = 84,
    T_IF_FUNC_ELSE  = 85,
    
    T_FUNC_CUSTOM       = 86,
    T_FUNC_FUNCTION     = 87,

    T_FUNC_MAX_TOKEN    = 100,

    /** set value */
    T_SET_FIRST                 = 100,
    T_SET_EQUAL                 = 100,
    T_SET_DPLUS                 = 101,
    T_SET_DMINUS                = 102,
    T_SET_SHIFT_LEFT_NUM        = 103,
    T_SET_SHIFT_RIGHT_NUM       = 104,
    T_SET_MULTIPLY_NUM          = 105,
    T_SET_DIVIDE_NUM            = 106,
    T_SET_MOD_NUM               = 107,
    T_SET_PLUS_NUM              = 108,
    T_SET_MINUS_NUM             = 109,
    T_SET_OR_NUM                = 110,
    T_SET_AND_NUM               = 111,
    T_SET_XOR_NUM               = 112,
    T_SET_LAST                  = 112,

    /** operation */
    T_OP_FIRST                  = 120,
    T_OP_MULTIPLY               = 120,
    T_OP_DIVIDE                 = 121,
    T_OP_MOD                    = 122,
    T_OP_SHIFT_LEFT             = 123,
    T_OP_SHIFT_RIGHT            = 124,
    T_OP_NOT_LESS               = 125,
    T_OP_NOT_GREATER            = 126,
    T_OP_GREATER                = 127,
    T_OP_LESS                   = 128,
    T_OP_NOT_SAME               = 129,
    T_OP_SAME                   = 130,
    T_OP_ANDAND                 = 131,
    T_OP_OROR                   = 132,
    T_OP_OR                     = 133,
    T_OP_AND                    = 134,
    T_OP_XOR                    = 135,
    T_OP_LAST                   = 135,

    /* $ */
    T_REG_USE                   = 150,
    T_REG_FIRST,
    T_REG_LAST = T_REG_FIRST + EZ_EXP_REG_COUNT - 1,
	
    T_LOAD_FUNCTION_ERROR 	= 0xFD,
    T_ERROR 	            = 0xFE,
    T_EOF   	            = 0xFF
} ez_token;

typedef enum {
    VT_UNUSE    = 0,
    VT_NUMBER   = 1,
    VT_STRING   = 2,
} ez_vtype;

typedef struct {
    ez_vtype    type;
    union {
        double  _number;
        char    *_string;
    } val;
} ez_reg;

typedef struct {
    char        _keyword[EZ_KEYWORD_SIZE + 1];
    ez_token    _token;
} keyword_to_token;

typedef struct {
    unsigned char   _use;
    char            _name[EZ_EXP_REG_NAME_LIMIT + 1];
    ez_vtype        _type;
    unsigned char   _token;
} reg_name_to_token;

typedef struct {
    char                *line, *p, *next_p;
    unsigned short      size;
    unsigned short      ln_num;
    ez_token            actual_func_token;
    ez_token            actual_token;
    double              actual_number;
    char                actual_char;
    char                *actual_string;
    int                 actual_size;
    char                actual_custom_keyword[EZ_KEYWORD_SIZE + 1];
    reg_name_to_token   regs[EZ_EXP_REG_COUNT];
    const char          *error;
} ez_tokenizer;

void ez_tokenizer_global_init(void);
void ez_tokenizer_init(char *input, ez_tokenizer *tokenizer);
ez_token ez_tokenizer_return(ez_tokenizer *tokenizer);
ez_token ez_tokenizer_get_next_token(ez_tokenizer *tokenizer, ez_token tk);

#endif // __TOKENIZER_H__

