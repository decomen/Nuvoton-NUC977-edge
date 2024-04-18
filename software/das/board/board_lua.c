
#include <board_lua.h>
#include <rtthread.h>
#include <string.h>

static lua_State *S_LUA = RT_NULL;

#define BOARD_LUA_SIZE      (256+1)

static rt_mq_t s_board_lua_queue = RT_NULL;
static rt_thread_t s_board_lua_thread = RT_NULL;
static void _board_lua_thread(void *parameter);

void board_lua_init(void)
{
    list_mem();
    S_LUA = luaL_newstate();
    if (S_LUA == NULL) {
        rt_kprintf("cannot create lua state: not enough memory\n");
    } else {
    list_mem();
        luaL_openlibs(S_LUA);
    list_mem();
    }

    if ((s_board_lua_queue = rt_mq_create("brd_lua", BOARD_LUA_SIZE, 1, RT_IPC_FLAG_FIFO)) == NULL) {
        rt_kprintf("cannot create lua queue: not enough memory\n");
    }

    s_board_lua_thread = rt_thread_create("brd_lua", _board_lua_thread, RT_NULL, 0x2000, 10, 10);
    if (s_board_lua_thread != RT_NULL) {
        rt_thddog_register(s_board_lua_thread, 30);
        rt_thread_startup(s_board_lua_thread);
    }
}

static void _board_lua_thread(void *parameter)
{
    while (1) {
        char lua_str[BOARD_LUA_SIZE] = {0};
        rt_thddog_suspend("rt_mq_recv");
        if (RT_EOK == rt_mq_recv(s_board_lua_queue, &lua_str[0], BOARD_LUA_SIZE, RT_WAITING_FOREVER)) {
            lua_str[BOARD_LUA_SIZE-1] = '\0';
            if (S_LUA&&lua_str[0]) {
                rt_thddog_suspend("luaL_dostring");
                luaL_dostring(S_LUA,lua_str);
                //lua_gc(S_LUA,LUA_GCCOLLECT,0);
            }
        }
        rt_thddog_feed("");
    }
    s_board_lua_thread = RT_NULL;
    rt_thddog_exit();
}

rt_bool_t board_lua_dostring(const char *luastr)
{
    if (
        S_LUA&&
        luastr&&luastr[0]&&
        s_board_lua_queue&&
        s_board_lua_thread&&s_board_lua_thread->stat!=RT_THREAD_INIT) {
        int len = strlen(luastr);
        if (len < BOARD_LUA_SIZE) {
            return (RT_EOK == rt_mq_send(s_board_lua_queue, (void *)luastr, len+1));
        } else {
            rt_kprintf("board luastr error with size:%d\n", len);
        }
    }
    return RT_FALSE;
}

