
#include "ez_string_function.h"

#include "varmanage.h"
double 
_getvar(const char *name) {
    double value = 0;
    if (name && name[0]) bVarManage_GetExtValueWithName(name, &value);
    return value;
}

static const function_string token_to_functions_string[T_FUNC_MAX_TOKEN] = {
    [T_FUNC_VAR]    = _getvar,
};

bool is_functionstring_token(ez_token t)
{
    return (t < T_FUNC_MAX_TOKEN && token_to_functions_string[t] != NULL);
}

function_string get_functionstring(ez_token t)
{
    return (t < T_FUNC_MAX_TOKEN ? token_to_functions_string[t] : NULL);
}


