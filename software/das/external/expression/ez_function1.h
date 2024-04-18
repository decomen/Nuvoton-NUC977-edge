#ifndef __EZ_FUNCTION1_H__
#define __EZ_FUNCTION1_H__

#include "ez_tokenizer.h"

typedef double (*function_1)(double number);

bool is_function1_token(ez_token t);
function_1 get_function1(ez_token t);

#endif

