/**
 * example of adding lua external library
 */

#include "lua.h"
#include "lauxlib.h"

#include "board.h"

int rtu_do_set(lua_State *L)
{
    lua_Integer index = luaL_optinteger(L,1,-1);
    lua_Integer val = luaL_optinteger(L,2,-1);
    if( index >= 0 && index < DO_CHAN_NUM && val >= 0 ) {
        g_xDOResultReg.xDOResult.usDO_xx[index] = (val!=0?PIN_SET:PIN_RESET);
    }
    
    return 0;
}

int rtu_do_get(lua_State *L)
{
    lua_Integer index = luaL_optinteger(L,1,-1);
    if( index >= 0 && index < DO_CHAN_NUM ) {
        lua_pushnumber(L, (g_xDOResultReg.xDOResult.usDO_xx[index] == (PIN_RESET ? 0 : 1)));
        return 1;
    }
    return 0;
}

int rtu_di_get(lua_State *L)
{
    lua_Integer index = luaL_optinteger(L,1,-1);
    if( index >= 0 && index < DI_CHAN_NUM ) {
        lua_pushnumber(L, (g_xDIResultReg.xDIResult.usDI_xx[index] == (PIN_RESET ? 0 : 1)));
        return 1;
    }
    return 0;
}

static const luaL_Reg rtu_do_lib[] = {
  {"set", rtu_do_set},
  {"get", rtu_do_get},
  {NULL, NULL}
};

static const luaL_Reg rtu_di_lib[] = {
  {"get", rtu_di_get},
  {NULL, NULL}
};

LUAMOD_API int luaopen_rtu_do (lua_State *L)
{
  luaL_newlib(L, rtu_do_lib);
  return 1;
}

LUAMOD_API int luaopen_rtu_di (lua_State *L)
{
  luaL_newlib(L, rtu_di_lib);
  return 1;
}


