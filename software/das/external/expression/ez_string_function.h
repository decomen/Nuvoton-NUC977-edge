#ifndef __EZ_STRING_FUNCTION_H__
#define __EZ_STRING_FUNCTION_H__

#include "ez_tokenizer.h"

typedef double (*function_string)(const char *str);

bool is_functionstring_token(ez_token t);
function_string get_functionstring(ez_token t);

#endif


