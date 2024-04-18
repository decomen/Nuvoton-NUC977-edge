/**
 * example of adding lua external library
 */

#include "lua.h"
#include "lstate.h"
#include "lualib.h"
#include "lauxlib.h"

#include "lrtu.h"
#include "board.h"

#define lua_uart_printf(_fmt,...)   rt_kprintf( "[lua uart]" _fmt, ##__VA_ARGS__ )

typedef struct {
    rt_uint16_t bufsz;
    rt_uint16_t timeout;
    rt_serial_t *serial;
    struct serial_device *node;
} lua_uart_cfg_t;

static lua_uart_cfg_t s_lua_uart_cfgs[BOARD_UART_MAX] = {{0,0}};
static rt_timer_t s_lua_uart_timers[BOARD_UART_MAX] = {RT_NULL};
static lua_rtu_queue_t *s_lua_uart_queue[BOARD_UART_MAX] = {RT_NULL};

static rt_bool_t _lua_uart_busy(int port)
{
    return RT_TRUE;
}

static void _lua_uart_recv_handle(int port, rt_uint8_t value)
{
    if (s_lua_uart_queue[port] && s_lua_uart_queue[port]->n < s_lua_uart_queue[port]->size) {
        int num = s_lua_uart_queue[port]->n;
        s_lua_uart_queue[port]->buf[num] = value;
        s_lua_uart_queue[port]->n = num+1;
    }
    if (s_lua_uart_timers[port] ) {
        rt_timer_start(s_lua_uart_timers[port]);
        //rt_timer_stop(s_lua_uart_timers[port]);
    }
}

static rt_err_t _lua_uart_rx_ind(rt_device_t dev, rt_size_t size)
{
    struct serial_device *node = ((struct serial_device *)((rt_serial_t *)dev)->parent.user_data);
    int port = node->instance;
    if (s_lua_uart_queue[port] && s_lua_uart_queue[port]->n < s_lua_uart_queue[port]->size) {
        rt_uint8_t ucByte = 0;
        while (dev->read(dev, 0, &ucByte, 1) > 0) {
            int num = s_lua_uart_queue[port]->n;
            s_lua_uart_queue[port]->buf[num] = ucByte;
            s_lua_uart_queue[port]->n = num+1;
        }
    }
    if (s_lua_uart_timers[port] ) {
        rt_timer_start(s_lua_uart_timers[port]);
        //rt_timer_stop(s_lua_uart_timers[port]);
    }
    return RT_EOK;
}

static rt_err_t _lua_uart_ide_ind(rt_device_t dev, rt_size_t size)
{
    struct serial_device *node = ((struct serial_device *)((rt_serial_t *)dev)->parent.user_data);
    int port = node->instance;
    if (s_lua_uart_timers[port] ) {
        rt_timer_start(s_lua_uart_timers[port]);
    }
    return RT_EOK;
}

static void _lua_uart_timeout_handle(void *parameter)
{
    int port = (int)(rt_uint32_t)parameter;
    if (s_lua_uart_queue[port]) {
        lua_rtu_recvdata(s_lua_uart_queue[port]);
        s_lua_uart_queue[port]->n = 0;
    }
}

int lua_uart_cfg(lua_State *L)
{
    // 默认 115200,8,n,1
    lua_Integer index = luaL_optinteger(L,1,-1);
    if (index >= 0 && nUartGetInstance(index) >= 0) {
        int n = nUartGetInstance(index);
        s_lua_uart_cfgs[n].bufsz = luaL_optinteger(L,2,256);
        s_lua_uart_cfgs[n].timeout = luaL_optinteger(L,3,10);
        lua_pushboolean(L, 1);
    } else {
        lua_uart_printf("err:cfg index[%d]\n",index);
        lua_pushboolean(L, 0);
    }

    return 1;
}

rt_bool_t lua_uart_check(int index)
{
    int port = nUartGetInstance(index);
    if (port >= 0) {
        if (PROTO_LUA == g_uart_cfgs[port].proto_type
#ifdef RT_USING_CONSOLE
            && (!g_host_cfg.bDebug || BOARD_CONSOLE_USART != port)
#endif
           ) {

            return (!g_xfer_net_dst_uart_occ[port]);
        }
    }

    return RT_FALSE;
}

void lua_uart_rx_enable(int index)
{
    int port = nUartGetInstance(index);
    if (s_lua_uart_cfgs[port].node) {
        struct serial_device *node = s_lua_uart_cfgs[port].node;
        if( node->cfg->uart_type == UART_TYPE_485 ) {
            rt_thread_delay( rt_uart_485_getrx_delay(node->cfg->port_cfg.baud_rate) *  RT_TICK_PER_SECOND / 1000 );
            rt_pin_write( node->pin_en, PIN_HIGH );
        }
    }
}

void lua_uart_tx_enable(int index)
{
    int port = nUartGetInstance(index);
    struct serial_device *node = s_lua_uart_cfgs[port].node;
    if( node->cfg->uart_type == UART_TYPE_485 ) {
        rt_pin_write( node->pin_en, PIN_LOW );
        rt_thread_delay( rt_uart_485_gettx_delay(node->cfg->port_cfg.baud_rate) * RT_TICK_PER_SECOND / 1000 );
    }
}

void lua_uart_senddata(int index, const void *data, rt_size_t size)
{
    int port = nUartGetInstance(index);
    if (s_lua_uart_cfgs[port].node) {
        rt_serial_t *serial = s_lua_uart_cfgs[port].serial;
        lua_uart_tx_enable(index);
        serial->parent.write(&serial->parent, 0, data, size);
        lua_uart_rx_enable(index);
    }
}

void lua_uart_openall(void)
{
    for (int index = 0; index < 4; index++) {
        if (lua_uart_check(index)) {
            int port = nUartGetInstance(index);
            rt_serial_t *serial = (rt_serial_t *)rt_device_find(BOARD_UART_DEV_NAME(port));
            if (RT_NULL == serial) {
                rt_kprintf("lua_uart_openall:rt_device_find('%s') falied..\n", BOARD_UART_DEV_NAME(port));
                continue ;
            }
            struct serial_device *node = ((struct serial_device *)serial->parent.user_data);
            serial->config.baud_rate = g_uart_cfgs[port].port_cfg.baud_rate;
            serial->config.data_bits = g_uart_cfgs[port].port_cfg.data_bits;
            serial->config.stop_bits = g_uart_cfgs[port].port_cfg.stop_bits;
            serial->config.parity    = g_uart_cfgs[port].port_cfg.parity;
            serial->parent.close(&serial->parent);
            serial->parent.rx_indicate = RT_NULL;
            serial->parent.ide_indicate = RT_NULL;
            node->cfg->busy_handle = RT_NULL;
            node->cfg->recv_handle = RT_NULL;
            serial->ops->configure(serial, &serial->config);

            s_lua_uart_cfgs[port].serial = serial;
            s_lua_uart_cfgs[port].node = node;

            if (s_lua_uart_timers[port]) {
                rt_timer_delete(s_lua_uart_timers[port]);
            }
            {
                if (s_lua_uart_cfgs[port].timeout<=1) s_lua_uart_cfgs[port].timeout = 10;
                BOARD_CREAT_NAME(szTime, "lua_u%d", port);
                s_lua_uart_timers[port] = \
                    rt_timer_create(szTime, _lua_uart_timeout_handle,
                                         (void *)(uint32_t)port,
                                         (rt_tick_t)rt_tick_from_millisecond(s_lua_uart_cfgs[port].timeout),
                                         RT_TIMER_FLAG_ONE_SHOT
                                         );
            }
            if (s_lua_uart_queue[port]) {
                rt_free(s_lua_uart_queue[port]);
            }
            {
                if (s_lua_uart_cfgs[port].bufsz<=1) s_lua_uart_cfgs[port].bufsz = 256;
                int size = sizeof(lua_rtu_queue_t)+s_lua_uart_cfgs[port].bufsz;
                s_lua_uart_queue[port] = rt_calloc(1, size);
                if (s_lua_uart_queue[port]) {
                    s_lua_uart_queue[port]->dev_type = index + PROTO_DEV_RS1;
                    s_lua_uart_queue[port]->dev_num = 0;
                    s_lua_uart_queue[port]->size = s_lua_uart_cfgs[port].bufsz;
                    s_lua_uart_queue[port]->n = 0;
                }
            }
            
            if ( RT_EOK == serial->parent.open(&serial->parent, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX)) {
                //serial->parent.rx_indicate = _lua_uart_rx_ind;
                //serial->parent.ide_indicate = _lua_uart_ide_ind;
                node->cfg->busy_handle = _lua_uart_busy;
                node->cfg->recv_handle = _lua_uart_recv_handle;
            }

            lua_uart_rx_enable(index);
        }
    }
}


void lua_uart_closeall(void)
{
    for (int index = 0; index < 4; index++) {
        if (lua_uart_check(index)) {
            int port = nUartGetInstance(index);
            rt_serial_t *serial = (rt_serial_t *)rt_device_find(BOARD_UART_DEV_NAME(port));
            if (RT_NULL == serial) {
                rt_kprintf("lua_uart_closeall:rt_device_find('%s') falied..\n", BOARD_UART_DEV_NAME(port));
                continue ;
            }
            serial->parent.close(&serial->parent);
            serial->parent.rx_indicate = RT_NULL;
            serial->parent.ide_indicate = RT_NULL;
        }
    }
}

static const luaL_Reg rtu_uart_lib[] = {
  {"cfg",       lua_uart_cfg}, 
  {NULL, NULL}
};

LUAMOD_API int luaopen_rtu_uart (lua_State *L)
{
  luaL_newlib(L, rtu_uart_lib);
  return 1;
}

