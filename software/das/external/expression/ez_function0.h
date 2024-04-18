#ifndef __EZ_FUNCTION0_H__
#define __EZ_FUNCTION0_H__

#include "ez_tokenizer.h"

typedef double (*function_0)(void);

bool is_function0_token(ez_token t);
function_0 get_function0(ez_token t);

#endif


