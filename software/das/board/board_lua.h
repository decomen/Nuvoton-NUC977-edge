
#ifndef __BOARD_LUA_H__
#define __BOARD_LUA_H__

#include <rtdef.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

void board_lua_init(void);
rt_bool_t board_lua_dostring(const char *str);

#endif

