
#include "ez_function1.h"
#include "board.h"
#include "stdint.h"

#define _ul(n)     (unsigned long)n

static double _abs(double n)  { return ((n>=0)?(n):(-n)); }
static double _int(double n)   { int64_t i = (int64_t)n; return 1.0 * i; }
static double _uint(double n)   { uint64_t i = (uint64_t)n; return 1.0 * i; }
static double _sqr(double n)  { return sqrt((double)n); }
static double _sgn(double n)  { return (n < 0 ? -1.0 : (n > 0 ? 1.0 : 0.0)); }

static double _bcd_2_dec(double number)
{
    unsigned long bcd = (unsigned long)number;
    unsigned long ret_dec = 0;
    int i;
    for (i = 0; i < 2 * sizeof(long); i++) {
        ret_dec *= 10;
        ret_dec += ((bcd >> 28) & 0x0F);
        bcd <<= 4;
    }
    return (double)ret_dec;
}

static double _diget(double number)
{
    return -1;
}

static double _doget(double number)
{
    return -1;
}

static const function_1 token_to_functions_1[T_FUNC_MAX_TOKEN] = {
    [T_FUNC_DI_GET]     =_diget,
    [T_FUNC_DO_GET]     =_doget,
    [T_FUNC_FLOOR]      = floor,
    [T_FUNC_CEIL]       = ceil,
    [T_FUNC_ABS]        = _abs,
    [T_FUNC_SIN]        = sin,
    [T_FUNC_COS]        = cos,
    [T_FUNC_INT]        = _int,
    [T_FUNC_UINT]       = _uint,
    [T_FUNC_TAN]        = tan,
    [T_FUNC_SQR]        = _sqr,
    [T_FUNC_SGN]        = _sgn,
    [T_FUNC_LOG]        = log,
    [T_FUNC_EXP]        = exp,
    [T_FUNC_ATN]        = atan,
    [T_FUNC_BCD2N]      =_bcd_2_dec,
};

bool is_function1_token(ez_token t)
{
    return (t < T_FUNC_MAX_TOKEN && token_to_functions_1[t] != NULL);
}

function_1 get_function1(ez_token t)
{
    return (t < T_FUNC_MAX_TOKEN ? token_to_functions_1[t] : NULL);
}

