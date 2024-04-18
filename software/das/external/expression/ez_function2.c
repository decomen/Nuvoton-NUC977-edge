
#include "ez_function2.h"
#include "board.h"

#define _ul(n)     (unsigned long)n

static double _doset(double number1, double number2)
{
    return -1;
}

static const function_2 token_to_functions_2[T_FUNC_MAX_TOKEN] = {
    [T_FUNC_DO_SET]     = _doset,
};

bool is_function2_token(ez_token t)
{
    return (t < T_FUNC_MAX_TOKEN && token_to_functions_2[t] != NULL);
}

function_2 get_function2(ez_token t)
{
    return (t < T_FUNC_MAX_TOKEN ? token_to_functions_2[t] : NULL);
}

