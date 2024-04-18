
/*
 * File      : rtdef.h
 */

#ifndef __RT_DEF_H__
#define __RT_DEF_H__

#include "rtconfig.h"
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* RT-Thread basic data type definitions */
typedef signed   char                   rt_int8_t;      /**<  8bit integer type */
typedef signed   short                  rt_int16_t;     /**< 16bit integer type */
typedef int32_t                         rt_int32_t;     /**< 32bit integer type */
typedef int64_t                         rt_int64_t;     /**< 64bit integer type */
typedef unsigned char                   rt_uint8_t;     /**<  8bit unsigned integer type */
typedef unsigned short                  rt_uint16_t;    /**< 16bit unsigned integer type */
typedef uint32_t                        rt_uint32_t;    /**< 32bit unsigned integer type */
typedef uint64_t                        rt_uint64_t;    /**< 64bit unsigned integer type */
typedef int                             rt_bool_t;      /**< boolean type */

/* 32bit CPU */
typedef int                             rt_base_t;      /**< Nbit CPU related date type */
typedef unsigned int                    rt_ubase_t;     /**< Nbit unsigned CPU related data type */

typedef rt_base_t                       rt_err_t;       /**< Type for error number */
typedef time_t                          rt_time_t;      /**< Type for time stamp */
typedef rt_uint64_t                     rt_tick_t;      /**< Type for tick count */
typedef rt_base_t                       rt_flag_t;      /**< Type for flags */
typedef rt_ubase_t                      rt_size_t;      /**< Type for size number */
typedef rt_ubase_t                      rt_dev_t;       /**< Type for device */
typedef rt_base_t                       rt_off_t;       /**< Type for offset */

typedef rt_bool_t                       BOOL;

typedef rt_uint8_t                      UCHAR;
typedef rt_int8_t                       CHAR;

typedef rt_uint16_t                     USHORT;
typedef rt_int16_t                      SHORT;

typedef rt_uint32_t                     ULONG;
typedef rt_int32_t                      LONG;

/* boolean type definitions */
#define RT_TRUE                         1               /**< boolean true  */
#define RT_FALSE                        0               /**< boolean fails */

/*@}*/

/* maximum value of base type */
#define RT_UINT8_MAX                    0xff            /**< Maxium number of UINT8 */
#define RT_UINT16_MAX                   0xffff          /**< Maxium number of UINT16 */
#define RT_UINT32_MAX                   0xffffffff      /**< Maxium number of UINT32 */
#define RT_TICK_MAX                     RT_UINT32_MAX   /**< Maxium number of tick */

/* the version of GNU GCC must be greater than 4.x */
typedef __builtin_va_list               __gnuc_va_list;
typedef __gnuc_va_list                  va_list;
#define va_start(v,l)                   __builtin_va_start(v,l)
#define va_end(v)                       __builtin_va_end(v)
#define va_arg(v,l)                     __builtin_va_arg(v,l)

#define SECTION(x)                      __attribute__((section(x)))
#define RT_UNUSED                       __attribute__((unused))
#define RT_USED                         __attribute__((used))
#define ALIGN(n)                        __attribute__((aligned(n)))
#define WEAK                            __attribute__((weak))
#define rt_inline                       static __inline
#define RTT_API

/* kernel malloc definitions */
#ifndef RT_KERNEL_MALLOC
#define RT_KERNEL_MALLOC(sz)            rt_malloc(sz)
#endif

#ifndef RT_KERNEL_CALLOC
#define RT_KERNEL_CALLOC(sz)            rt_calloc(sz,1)
#endif

#ifndef RT_KERNEL_FREE
#define RT_KERNEL_FREE(ptr)             rt_free(ptr)
#endif

#ifndef RT_KERNEL_REALLOC
#define RT_KERNEL_REALLOC(ptr, size)    rt_realloc(ptr, size)
#endif

/**
 * @addtogroup Error
 */

/*@{*/

/* RT-Thread error code definitions */
#define RT_EOK                          0               /**< There is no error */
#define RT_ERROR                        1               /**< A generic error happens */
#define RT_ETIMEOUT                     2               /**< Timed out */
#define RT_EFULL                        3               /**< The resource is full */
#define RT_EEMPTY                       4               /**< The resource is empty */
#define RT_ENOMEM                       5               /**< No memory */
#define RT_ENOSYS                       6               /**< No system */
#define RT_EBUSY                        7               /**< Busy */
#define RT_EIO                          8               /**< IO error */

/*@}*/

/**
 * @ingroup BasicDef
 *
 * @def RT_ALIGN(size, align)
 * Return the most contiguous size aligned at specified width. RT_ALIGN(13, 4)
 * would return 16.
 */
#define RT_ALIGN(size, align)           (((size) + (align) - 1) & ~((align) - 1))

/**
 * @ingroup BasicDef
 *
 * @def RT_ALIGN_DOWN(size, align)
 * Return the down number of aligned at specified width. RT_ALIGN_DOWN(13, 4)
 * would return 12.
 */
#define RT_ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))

/**
 * @ingroup BasicDef
 *
 * @def RT_NULL
 * Similar as the \c NULL in C library.
 */
#define RT_NULL                         (0)

typedef enum
{
    MB_RTU,                     /*!< RTU transmission mode. */
    MB_ASCII,                   /*!< ASCII transmission mode. */
    MB_TCP,                     /*!< TCP mode. */
    MB_RTU_OVERTCP,             /*!< RTU Over TCP mode. */
} eMBMode;

#define DAS_DEV_ID_LEN              64
#define DAS_SN_LEN                  64
#define DAS_HW_ID_LEN               128

#define DAS_HW_VER_LEN              16
#define DAS_SW_VER_LEN              16

/** yyyy-MM-dd HH:mm:ss */
#define DAS_TIME_LEN                20

typedef struct das_system_info {
    //整形 设备型号
    uint32_t DEV_MODEL;
    //字串 是 设备ID 设备ID
    char DEV_ID[DAS_DEV_ID_LEN];
    //字串 是 设备序列号 设备序列号
    char SN[DAS_SN_LEN];
    //字串 是 硬件版本 硬件版本
    char HW_VER[DAS_HW_VER_LEN];
    //字串 是 软件版本 固件版本
    char SW_VER[DAS_SW_VER_LEN];
    //字串 是 硬件序列号 硬件序列号
    char HW_ID[DAS_HW_ID_LEN];
    //时间格式 否 生产日期（约定格式） 生产日期
    char PROD_DATE[DAS_TIME_LEN];
    //时间格式 否 系统当前时间（约定格式） 系统时间 （YYYY/MM/DD hh:mm:ss）
    char SYS_DATE[DAS_TIME_LEN];
    //整形 否 系统运行时长（单位：分钟） 运行时间 （xx时 xx分）
    uint32_t RUNTIME;
    //字串 是 描述 系统描述
    char DESC[1024];
    //整型 否 激活状态 0：未激活 1：已激活 激活状态
    uint32_t REG_STATUS;
    //整型 否 剩余试用时间（分钟） 剩余试用时间 （xx天xx时xx分）
    uint32_t TEST_REMAIN;
} das_system_info_t;

typedef struct das_system_ver {
    uint32_t HW_VER;
    uint32_t SW_VER;
} das_system_ver_t;

#include "das_os.h"

#ifdef __cplusplus
}
#endif

#endif
