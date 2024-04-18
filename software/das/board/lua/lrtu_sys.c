/**
 * example of adding lua external library
 */

#include "lua.h"
#include "lauxlib.h"

#include "board.h"

// runtime ms
int rtu_sys_runtime(lua_State *L)
{
    lua_pushnumber(L, rt_millisecond_from_tick(rt_tick_get()));

    return 1;
}

// time s
int rtu_sys_time(lua_State *L)
{
    lua_pushnumber(L, time(0));

    return 1;
}

int rtu_sys_getvar(lua_State *L)
{
    size_t len = 0;
    const char *name = lua_tolstring(L,1,&len);
    if(name&&len>0) {
        var_double_t value = 0;
        if( bVarManage_GetExtValueWithName( name, &value ) ) {
            lua_pushnumber(L, value);
            return 1;
        }
    }

    return 0;
}

int rtu_sys_setvar(lua_State *L)
{
    /*size_t len = 0;
    const char *name = lua_tolstring(L,1,&len);
    if(name&&len>0) {
        ExtData_t data;
        var_int_t n = nVarManage_GetExtDataWithName( name, &data );
        if( n >= 0 ) {

        }
    }

    return 1;*/

    return 0;
}

int rtu_sys_getproto(lua_State *L)
{
    lua_Integer dev_type = luaL_optinteger(L,1,-1);
    lua_Integer dev_num = luaL_optinteger(L,2,-1);
    if (dev_type>=0&&dev_num>=0) {
        if (dev_type<=PROTO_DEV_RS_MAX) {
            if (lua_uart_check(dev_type)) {
                int port = nUartGetInstance(dev_type);
                lua_pushstring(L, (const char *)g_uart_cfgs[port].lua_proto);
            } else {
                lua_pushstring(L, "");
            }
        }/* else if() {
            
        }*/
    }

    return 1;
}

int rtu_sys_sleep(lua_State *L)
{
    lua_Integer ms = luaL_optinteger(L,1, 1);
    rt_thread_delay(rt_tick_from_millisecond(ms));
    return 0;
}

static const luaL_Reg rtu_sys_lib[] = {
  {"runtime",   rtu_sys_runtime},
  {"time",      rtu_sys_time},
  {"getvar",    rtu_sys_getvar},
  {"setvar",    rtu_sys_setvar},
  {"sleep",     rtu_sys_sleep},
  {"getproto",  rtu_sys_getproto},
  {NULL, NULL}
};

LUAMOD_API int luaopen_rtu_sys (lua_State *L)
{
  luaL_newlib(L, rtu_sys_lib);
  return 1;
}


