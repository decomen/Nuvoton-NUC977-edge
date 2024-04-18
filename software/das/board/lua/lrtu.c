/**
 * example of adding lua external library
 */

#include "lua.h"
#include "lstate.h"
#include "lualib.h"
#include "lauxlib.h"

#include "lrtu.h"
#include "board.h"

static lua_State *s_lua_rtu = RT_NULL;
static rt_thread_t s_lua_rtu_thread = RT_NULL;
static rt_uint16_t s_lua_rtu_bufsz = 0;
static rt_mq_t s_lua_rtu_mq = RT_NULL;

//volatile rt_bool_t s_inpoll = RT_FALSE;

#define lua_rtu_printf(_fmt,...)   rt_kprintf( "[lua rtu]" _fmt, ##__VA_ARGS__ )

extern void lua_uart_openall(void);
extern void lua_uart_closeall(void);
extern void lua_uart_senddata(int index, const void *data, rt_size_t size);

static void _lua_rtu_thread(void *parameter);

void lua_rtu_init(void)
{
    s_lua_rtu_thread = rt_thread_create("lua_prot", _lua_rtu_thread, RT_NULL, 0x1200, 8, 10);
    if (s_lua_rtu_thread != RT_NULL) {
        rt_thddog_register(s_lua_rtu_thread, 60);
        rt_thread_startup(s_lua_rtu_thread);
    }
}

static void _lua_rtu_thread(void *parameter)
{
#define LUA_HEAP_SIZE           (40*1024)
    void *luaheapaddr = rt_malloc(LUA_HEAP_SIZE);
    lua_system_heap_init(luaheapaddr, (void *)((rt_uint32_t)luaheapaddr+LUA_HEAP_SIZE));
    if (luaheapaddr) {
        s_lua_rtu = luaL_newstate();
        if (s_lua_rtu == NULL) {
            lua_rtu_printf("err:create lua state not enough memory\n");
            if (luaheapaddr) rt_free(luaheapaddr);
            return ;
        } else {
            luaL_openlibs(s_lua_rtu);
            rt_thddog_suspend("rtu_main.lua luaL_loadfile");
            int ret = luaL_loadfile(s_lua_rtu, "/lua/rtu_main.lua");
            if (LUA_OK == ret) {
                rt_thddog_suspend("rtu_main.lua lua_pcall");
                if (lua_pcall(s_lua_rtu, 0, LUA_MULTRET, 0)) {
                    const char* sz = lua_tostring(s_lua_rtu, -1);
                    lua_pop(s_lua_rtu, 1);
                    lua_settop(s_lua_rtu, 0);
                    lua_rtu_printf(" pcall error please check:%s\n", sz);
                }
                rt_thddog_resume();
            } else {
                goto _do_exp;
            }
        }
    } else {
        lua_rtu_printf("err:create lua heap not enough memory[%d]\n",LUA_HEAP_SIZE);
        if (luaheapaddr) rt_free(luaheapaddr);
        return ;
    }
    
    lua_system_heap_init(luaheapaddr, (void *)((rt_uint32_t)luaheapaddr+LUA_HEAP_SIZE));
    s_lua_rtu = luaL_newstate();
    if (s_lua_rtu == NULL) {
        lua_rtu_printf("err:create lua state not enough memory\n");
        if (luaheapaddr) rt_free(luaheapaddr);
        return ;
    } else {
        luaL_openlibs(s_lua_rtu);
    }

_do_exp:
    lua_rtu_printf("err:rtu_main.lua exit, will do exp thread!\n");

    int size = sizeof(lua_rtu_queue_t)+1024;
    if (s_lua_rtu_mq) rt_mq_delete(s_lua_rtu_mq);
    if ((s_lua_rtu_mq = rt_mq_create("lua_rtu", size, 1, RT_IPC_FLAG_PRIO)) == RT_NULL) {
        lua_rtu_printf("err:cannot create lua mq: not enough memory\n");
        if (luaheapaddr) rt_free(luaheapaddr);
        return ;
    }

    if (s_lua_rtu_mq) {
        lua_rtu_queue_t *rcv = rt_calloc(1, size);
        if (rcv) {
            while (1) {
                rt_thddog_suspend("rt_mq_recv s_lua_rtu_mq");
                if (RT_EOK == rt_mq_recv( s_lua_rtu_mq, (void *)rcv, size, RT_WAITING_FOREVER)) {
                    switch (rcv->msg_type) {
                    case LUA_RTU_DOEXP:
                    {
                        rt_thddog_suspend("luaL_dostring");
                        luaL_dostring(s_lua_rtu, (const char *)rcv->buf);
                        break;
                    }
                    default:
                        lua_rtu_printf("err:rtu_main.lua not run, please check!\n");
                    }
                }
                rt_thddog_resume();
            }
            rt_free(rcv);
        }
    }
    lua_rtu_printf("err:lua_rtu_thread init err!\n");

    rt_thddog_exit();
    if (luaheapaddr) rt_free(luaheapaddr);
}

int lua_rtu_open(lua_State *L)
{
    lua_Integer buf_sz = luaL_optinteger(L,1,512);
    lua_Integer buf_cnt = luaL_optinteger(L,2,1);
    if (buf_sz <= 0 || buf_sz > 4096) {
        lua_rtu_printf("err:buf_sz(1->4096)\n");
        lua_pushboolean(L, 0);
        return 1;
    }
    if (buf_cnt <= 0 || buf_cnt > 5) {
        lua_rtu_printf("err:buf_cnt(1->5)\n");
        lua_pushboolean(L, 0);
        return 1;
    }

    s_lua_rtu_bufsz = buf_sz;
    int size = sizeof(lua_rtu_queue_t)+buf_sz;
    if ((s_lua_rtu_mq = rt_mq_create("lua_prot", size, buf_cnt, RT_IPC_FLAG_PRIO)) == RT_NULL) {
        lua_rtu_printf("err:cannot create lua mq: not enough memory\n");
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_pushboolean(L, 1);

    lua_uart_openall();
    //lua_net_openall();
    //lua_zgb_openall();

    //s_inpoll = RT_FALSE;

    return 1;
}

int lua_rtu_close(lua_State *L)
{
    lua_uart_closeall();
    //lua_net_closeall();
    //lua_zgb_closeall();
    if (s_lua_rtu_mq) rt_mq_delete(s_lua_rtu_mq);
    s_lua_rtu_mq = RT_NULL;
    return 0;
}

void lua_rtu_doexp(char *exp)
{
    if (s_lua_rtu_mq) {
        if (exp&&exp[0]) {
            int len = strlen(exp);
            lua_rtu_queue_t *queue = rt_calloc(1,sizeof(lua_rtu_queue_t)+len+1);
            if (queue) {
                queue->msg_type = LUA_RTU_DOEXP;
                rt_strncpy((char *)queue->buf, exp, len+1);
                rt_mq_send(s_lua_rtu_mq, (void *)queue, sizeof(lua_rtu_queue_t)+len+1);
                rt_free(queue);
            } else {
                lua_rtu_printf("err: lua_rtu_doexp not enough memory\n");
            }
        } else {
            lua_rtu_printf("err: lua_rtu_doexp empty exp\n");
        }
    } else {
        lua_rtu_printf("err: not open\n");
    }
}

void lua_rtu_recvdata(lua_rtu_queue_t *queue)
{
    if (s_lua_rtu_mq) {
        queue->msg_type = LUA_RTU_DATA;
        rt_mq_send(s_lua_rtu_mq, (void *)queue, sizeof(lua_rtu_queue_t)+queue->n);
    } else {
        lua_rtu_printf("err: not open\n");
    }
}

int lua_rtu_poll(lua_State *L)
{
    lua_Integer timeout = luaL_optinteger(L,1,0);
    if (s_lua_rtu_mq) {
        int size = sizeof(lua_rtu_queue_t) + s_lua_rtu_bufsz;
        lua_rtu_queue_t *rcv = rt_calloc(1, size);
        if (rcv) {
            while (1) {
                rt_tick_t start_tick = rt_tick_get();
                if (RT_EOK == rt_mq_recv(
                    s_lua_rtu_mq, 
                    (void *)rcv, 
                    size, 
                    timeout<0?RT_WAITING_FOREVER:rt_tick_from_millisecond(timeout))) {

                    switch (rcv->msg_type) {
                    case LUA_RTU_DOEXP:
                    {
                        luaL_dostring(L, (const char *)rcv->buf);
                        lua_gc(s_lua_rtu, LUA_GCCOLLECT, 0);
                        break;
                    }
                    case LUA_RTU_DATA:
                    {
                        luaL_Buffer b;
                        luaL_buffinit(L, &b);
                        
                        lua_pushinteger(L, 0);
                        lua_newtable(L);
                        int tb_dex = lua_gettop(L);
                        lua_pushinteger(L, rcv->msg_type);
                        lua_setfield(L, tb_dex, "msgtype");
                        lua_pushinteger(L, rcv->dev_type);
                        lua_setfield(L, tb_dex, "devtype");
                        lua_pushinteger(L, rcv->dev_num);
                        lua_setfield(L, tb_dex, "devnum");
                        for (int i = 0; i < rcv->n; i++) {
                            char *buff = luaL_prepbuffsize(&b, 1);
                            buff[0] = (char)(rcv->buf[i]);
                            luaL_addsize(&b, 1);
                        }
                        luaL_pushresult(&b);
                        lua_setfield(L, tb_dex, "buffer");
                        rt_free(rcv);
                        return 2;
                        break;
                    }
                    }
                    // 表达式在本地执行
                    if (LUA_RTU_DOEXP == rcv->msg_type) {
                        if (timeout<0) continue;
                        rt_tick_t total = rt_tick_from_millisecond(timeout);
                        rt_tick_t now_tick = rt_tick_get();
                        if (now_tick-start_tick>=total) {
                            lua_pushinteger(L, -2);
                            break;
                        } else {
                            timeout = rt_millisecond_from_tick(total-(now_tick-start_tick));
                            continue;
                        }
                    }
                } else {
                    lua_pushinteger(L, -2);
                    break;
                }
            }
            rt_free(rcv);
        } else {
            lua_rtu_printf("err:cannot calloc rcvbuffer: not enough memory\n");
            lua_pushinteger(L, -3);
        }
    } else {
        lua_pushinteger(L, -1);
    }
    return 1;
}


static int getintfield(lua_State *L, const char *key, int def) 
{
    lua_Integer val = def;
    if (lua_getfield(L, -1, key)!=LUA_TNIL) {
        val = luaL_optinteger(L,-1,def);
    }
    lua_pop(L, 1);
    return val;
}

static const char *getstringfield(lua_State *L, const char *key, size_t *len) 
{
    const char *val = NULL;
    *len = 0;
    if (lua_getfield(L, -1, key)!=LUA_TNIL) {
        val = luaL_optlstring(L,-1,NULL,len);
    }
    lua_pop(L, 1);
    return val;
}

int lua_rtu_push(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);
    lua_Integer msgtype = getintfield(L, "msgtype", -1);
    switch (msgtype) {
    case LUA_RTU_DATA:
    {
        lua_Integer dev_type = getintfield(L, "devtype", -1);
        switch (dev_type) {
        case PROTO_DEV_RS1:
        case PROTO_DEV_RS2:
        case PROTO_DEV_RS3:
        case PROTO_DEV_RS4:
        {
            size_t len = 0;
            const char *buffer = getstringfield(L, "buffer", &len);
            if (buffer && len > 0) {
                lua_uart_senddata(dev_type, (const void *)buffer, len);
            }
            break;
        }
        }
        lua_pushboolean(L, 1);
        break;
    }
    default:
        lua_pushboolean(L, 0);
    }

    return 1;
}

static const luaL_Reg rtu_lib[] = {
  {"open",      lua_rtu_open}, 
  {"close",     lua_rtu_close}, 
  {"poll",      lua_rtu_poll}, 
  {"push",      lua_rtu_push}, 
  {NULL, NULL}
};

LUAMOD_API int luaopen_rtu (lua_State *L)
{
  luaL_newlib(L, rtu_lib);
  return 1;
}


