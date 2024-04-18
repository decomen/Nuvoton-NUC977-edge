#ifndef __EZ_FUNCTION2_H__
#define __EZ_FUNCTION2_H__

#include "ez_tokenizer.h"

typedef double (*function_2)(double number1, double number2);

bool is_function2_token(ez_token t);
function_2 get_function2(ez_token t);

#endif

