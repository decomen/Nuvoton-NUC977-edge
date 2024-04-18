/*
 * Copyright © 2001-2013 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#ifndef OBMODBUS_H
#define OBMODBUS_H

/* Add this for macros that defined unix flavor */
#if (defined(__unix__) || defined(unix)) && !defined(USG)
#include <sys/param.h>
#endif

#ifndef _MSC_VER
#include <stdint.h>
#else
#include "stdint.h"
#endif

#include "obmodbus-version.h"

#if defined(_MSC_VER)
# if defined(DLLBUILD)
/* define DLLBUILD when building the DLL */
#  define OBMODBUS_API __declspec(dllexport)
# else
#  define OBMODBUS_API __declspec(dllimport)
# endif
#else
# define OBMODBUS_API
#endif

#ifdef  __cplusplus
# define OBMODBUS_BEGIN_DECLS  extern "C" {
# define OBMODBUS_END_DECLS    }
#else
# define OBMODBUS_BEGIN_DECLS
# define OBMODBUS_END_DECLS
#endif

OBMODBUS_BEGIN_DECLS

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef OFF
#define OFF 0
#endif

#ifndef ON
#define ON 1
#endif

/* Modbus function codes */

#define OBMODBUS_FC_READ_01                   0x01
#define OBMODBUS_FC_READ_03                   0x03
#define OBMODBUS_FC_READ_04                   0x04
#define OBMODBUS_FC_READ_07                   0x07
#define OBMODBUS_FC_READ_09                   0x09
#define OBMODBUS_FC_READ_12                   0x12
#define OBMODBUS_FC_READ_13                   0x13
#define OBMODBUS_FC_READ_16                   0x16
#define OBMODBUS_FC_READ_17                   0x17
#define OBMODBUS_FC_READ_1A                   0x1A

#define OBMODBUS_FC_WRITE_06                  0x06
#define OBMODBUS_FC_WRITE_08                  0x08
#define OBMODBUS_FC_WRITE_10                  0x10
#define OBMODBUS_FC_WRITE_0A                  0x0A
#define OBMODBUS_FC_WRITE_0B                  0x0B
#define OBMODBUS_FC_WRITE_0C                  0x0C
#define OBMODBUS_FC_WRITE_14                  0x14
#define OBMODBUS_FC_WRITE_15                  0x15
#define OBMODBUS_FC_WRITE_18                  0x18
#define OBMODBUS_FC_WRITE_19                  0x19
#define OBMODBUS_FC_WRITE_2A                  0x2A
#define OBMODBUS_FC_WRITE_2C                  0x2C

#define OBMODBUS_BROADCAST_ADDRESS    0

/* Modbus_Application_Protocol_V1_1b.pdf (chapter 6 section 1 page 12)
 * Quantity of Coils to read (2 bytes): 1 to 2000 (0x7D0)
 * (chapter 6 section 11 page 29)
 * Quantity of Coils to write (2 bytes): 1 to 1968 (0x7B0)
 */
#define OBMODBUS_MAX_READ_BITS              2000
#define OBMODBUS_MAX_WRITE_BITS             1968

/* Modbus_Application_Protocol_V1_1b.pdf (chapter 6 section 3 page 15)
 * Quantity of Registers to read (2 bytes): 1 to 125 (0x7D)
 * (chapter 6 section 12 page 31)
 * Quantity of Registers to write (2 bytes) 1 to 123 (0x7B)
 * (chapter 6 section 17 page 38)
 * Quantity of Registers to write in R/W registers (2 bytes) 1 to 121 (0x79)
 */
#define OBMODBUS_MAX_READ_REGISTERS          250
#define OBMODBUS_MAX_WRITE_REGISTERS         246
#define OBMODBUS_MAX_WR_WRITE_REGISTERS      242
#define OBMODBUS_MAX_WR_READ_REGISTERS       250

/* The size of the OBMODBUS PDU is limited by the size constraint inherited from
 * the first OBMODBUS implementation on Serial Line network (max. RS485 ADU = 256
 * bytes). Therefore, OBMODBUS PDU for serial line communication = 256 - Server
 * address (1 byte) - CRC (2 bytes) = 253 bytes.
 */
#define OBMODBUS_MAX_PDU_LENGTH              253

/* Consequently:
 * - RTU OBMODBUS ADU = 253 bytes + Server address (1 byte) + CRC (2 bytes) = 256
 *   bytes.
 * - TCP OBMODBUS ADU = 253 bytes + MBAP (7 bytes) = 260 bytes.
 * so the maximum of both backend in 260 bytes. This size can used to allocate
 * an array of bytes to store responses and it will be compatible with the two
 * backends.
 */
#define OBMODBUS_MAX_ADU_LENGTH              260

/* Random number to avoid errno conflicts */
#define OBMODBUS_ENOBASE 112345678

/* Protocol exceptions */
enum {
    OBMODBUS_EXCEPTION_ILLEGAL_FUNCTION = 0x01,
    OBMODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS,
    OBMODBUS_EXCEPTION_ILLEGAL_DATA_VALUE,
    OBMODBUS_EXCEPTION_SLAVE_OR_SERVER_FAILURE,
    OBMODBUS_EXCEPTION_ACKNOWLEDGE,
    OBMODBUS_EXCEPTION_SLAVE_OR_SERVER_BUSY,
    OBMODBUS_EXCEPTION_NEGATIVE_ACKNOWLEDGE,
    OBMODBUS_EXCEPTION_MEMORY_PARITY,
    OBMODBUS_EXCEPTION_NOT_DEFINED,
    OBMODBUS_EXCEPTION_GATEWAY_PATH,
    OBMODBUS_EXCEPTION_GATEWAY_TARGET,
    OBMODBUS_EXCEPTION_MAX
};

#define EOBMBXILFUN  (OBMODBUS_ENOBASE + OBMODBUS_EXCEPTION_ILLEGAL_FUNCTION)
#define EOBMBXILADD  (OBMODBUS_ENOBASE + OBMODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS)
#define EOBMBXILVAL  (OBMODBUS_ENOBASE + OBMODBUS_EXCEPTION_ILLEGAL_DATA_VALUE)
#define EOBMBXSFAIL  (OBMODBUS_ENOBASE + OBMODBUS_EXCEPTION_SLAVE_OR_SERVER_FAILURE)
#define EOBMBXACK    (OBMODBUS_ENOBASE + OBMODBUS_EXCEPTION_ACKNOWLEDGE)
#define EOBMBXSBUSY  (OBMODBUS_ENOBASE + OBMODBUS_EXCEPTION_SLAVE_OR_SERVER_BUSY)
#define EOBMBXNACK   (OBMODBUS_ENOBASE + OBMODBUS_EXCEPTION_NEGATIVE_ACKNOWLEDGE)
#define EOBMBXMEMPAR (OBMODBUS_ENOBASE + OBMODBUS_EXCEPTION_MEMORY_PARITY)
#define EOBMBXGPATH  (OBMODBUS_ENOBASE + OBMODBUS_EXCEPTION_GATEWAY_PATH)
#define EOBMBXGTAR   (OBMODBUS_ENOBASE + OBMODBUS_EXCEPTION_GATEWAY_TARGET)

/* Native libobmodbus error codes */
#define EOBMBBADCRC  (EOBMBXGTAR + 1)
#define EOBMBBADDATA (EOBMBXGTAR + 2)
#define EOBMBBADEXC  (EOBMBXGTAR + 3)
#define EOBMBUNKEXC  (EOBMBXGTAR + 4)
#define EOBMBMDATA   (EOBMBXGTAR + 5)
#define EOBMBBADSLAVE (EOBMBXGTAR + 6)

extern const unsigned int libobmodbus_version_major;
extern const unsigned int libobmodbus_version_minor;
extern const unsigned int libobmodbus_version_micro;

typedef struct _obmodbus obmodbus_t;

typedef struct _obmodbus_customize_backend {
    ssize_t (*send) (obmodbus_t *ctx, const uint8_t *req, int req_length);
    ssize_t (*recv) (obmodbus_t *ctx, uint8_t *rsp, int rsp_length);
    int (*connect) (obmodbus_t *ctx);
    void (*close) (obmodbus_t *ctx);
    int (*select) (obmodbus_t *ctx, fd_set *rset, struct timeval *tv, int msg_length);
} obmodbus_customize_backend_t;

typedef enum _obmodbus_state {
    OBMB_STATE_INIT = 0,
    OBMB_STATE_ACCEPT,
    OBMB_STATE_CONNING,
    OBMB_STATE_CONNED,
    OBMB_STATE_DISCONNED,
} obmodbus_state_t;

typedef struct _obmodbus_monitor {
    void (*read) (obmodbus_t *ctx, const uint8_t *data, int length);
    void (*write) (obmodbus_t *ctx, const uint8_t *data, int length);
    void (*state_change) (obmodbus_t *ctx, int s, obmodbus_state_t state);
} obmodbus_monitor_t;

typedef struct {
    int nb_registers;
    int start_registers;
    uint8_t *tab_registers;
} obmodbus_mapping_t;

typedef enum
{
    OBMODBUS_ERROR_RECOVERY_NONE          = 0,
    OBMODBUS_ERROR_RECOVERY_LINK          = (1<<1),
    OBMODBUS_ERROR_RECOVERY_PROTOCOL      = (1<<2)
} obmodbus_error_recovery_mode;

OBMODBUS_API int obmodbus_set_customize_backend(obmodbus_t *ctx, obmodbus_customize_backend_t *backend);
OBMODBUS_API int obmodbus_set_monitor(obmodbus_t *ctx, obmodbus_monitor_t *monitor);
OBMODBUS_API int obmodbus_set_user_data(obmodbus_t *ctx, void *user_data);
OBMODBUS_API void *obmodbus_get_user_data(obmodbus_t *ctx);

OBMODBUS_API int obmodbus_set_slave(obmodbus_t* ctx, int slave);
OBMODBUS_API int obmodbus_set_error_recovery(obmodbus_t *ctx, obmodbus_error_recovery_mode error_recovery);
OBMODBUS_API int obmodbus_set_socket(obmodbus_t *ctx, int s);
OBMODBUS_API int obmodbus_get_socket(obmodbus_t *ctx);

OBMODBUS_API int obmodbus_get_response_timeout(obmodbus_t *ctx, uint32_t *to_sec, uint32_t *to_usec);
OBMODBUS_API int obmodbus_set_response_timeout(obmodbus_t *ctx, uint32_t to_sec, uint32_t to_usec);

OBMODBUS_API int obmodbus_get_byte_timeout(obmodbus_t *ctx, uint32_t *to_sec, uint32_t *to_usec);
OBMODBUS_API int obmodbus_set_byte_timeout(obmodbus_t *ctx, uint32_t to_sec, uint32_t to_usec);

OBMODBUS_API int obmodbus_get_header_length(obmodbus_t *ctx);

OBMODBUS_API int obmodbus_connect(obmodbus_t *ctx);
OBMODBUS_API void obmodbus_close(obmodbus_t *ctx);

OBMODBUS_API void obmodbus_free(obmodbus_t *ctx);

OBMODBUS_API int obmodbus_flush(obmodbus_t *ctx);
OBMODBUS_API int obmodbus_set_debug(obmodbus_t *ctx, int flag);

OBMODBUS_API const char *obmodbus_strerror(int errnum);

OBMODBUS_API int obmodbus_read_registers(obmodbus_t *ctx, int function, int addr, int nb, uint8_t *dest);
OBMODBUS_API int obmodbus_write_and_read_registers(obmodbus_t *ctx, int function, int write_addr, int write_nb,
                                               const uint8_t *src, int read_addr, int read_nb,
                                               uint8_t *dest);

OBMODBUS_API obmodbus_mapping_t* obmodbus_mapping_new_start_address(
    unsigned int start_registers, unsigned int nb_registers);

OBMODBUS_API obmodbus_mapping_t* obmodbus_mapping_new(int nb_registers);
OBMODBUS_API void obmodbus_mapping_free(obmodbus_mapping_t *mb_mapping);

OBMODBUS_API int obmodbus_send_raw_request(obmodbus_t *ctx, uint8_t *raw_req, int raw_req_length);

OBMODBUS_API int obmodbus_receive(obmodbus_t *ctx, uint8_t *req);

OBMODBUS_API int obmodbus_receive_confirmation(obmodbus_t *ctx, uint8_t *rsp);

OBMODBUS_API int obmodbus_reply(obmodbus_t *ctx, const uint8_t *req,
                            int req_length, obmodbus_mapping_t *mb_mapping);
OBMODBUS_API int obmodbus_reply_exception(obmodbus_t *ctx, const uint8_t *req,
                                      unsigned int exception_code);

/**
 * UTILS FUNCTIONS
 **/

#define OBMODBUS_GET_HIGH_BYTE(data) (((data) >> 8) & 0xFF)
#define OBMODBUS_GET_LOW_BYTE(data) ((data) & 0xFF)
#define OBMODBUS_GET_INT64_FROM_INT16(tab_int16, index) \
    (((int64_t)tab_int16[(index)    ] << 48) + \
     ((int64_t)tab_int16[(index) + 1] << 32) + \
     ((int64_t)tab_int16[(index) + 2] << 16) + \
      (int64_t)tab_int16[(index) + 3])
#define OBMODBUS_GET_INT32_FROM_INT16(tab_int16, index) ((tab_int16[(index)] << 16) + tab_int16[(index) + 1])
#define OBMODBUS_GET_INT16_FROM_INT8(tab_int8, index) ((tab_int8[(index)] << 8) + tab_int8[(index) + 1])
#define OBMODBUS_SET_INT16_TO_INT8(tab_int8, index, value) \
    do { \
        tab_int8[(index)] = (value) >> 8;  \
        tab_int8[(index) + 1] = (value) & 0xFF; \
    } while (0)
#define OBMODBUS_SET_INT32_TO_INT16(tab_int16, index, value) \
    do { \
        tab_int16[(index)    ] = (value) >> 16; \
        tab_int16[(index) + 1] = (value); \
    } while (0)
#define OBMODBUS_SET_INT64_TO_INT16(tab_int16, index, value) \
    do { \
        tab_int16[(index)    ] = (value) >> 48; \
        tab_int16[(index) + 1] = (value) >> 32; \
        tab_int16[(index) + 2] = (value) >> 16; \
        tab_int16[(index) + 3] = (value); \
    } while (0)

OBMODBUS_API void obmodbus_set_bits_from_byte(uint8_t *dest, int idx, const uint8_t value);
OBMODBUS_API void obmodbus_set_bits_from_bytes(uint8_t *dest, int idx, unsigned int nb_bits,
                                       const uint8_t *tab_byte);
OBMODBUS_API uint8_t obmodbus_get_byte_from_bits(const uint8_t *src, int idx, unsigned int nb_bits);
OBMODBUS_API float obmodbus_get_float(const uint16_t *src);
OBMODBUS_API float obmodbus_get_float_abcd(const uint16_t *src);
OBMODBUS_API float obmodbus_get_float_dcba(const uint16_t *src);
OBMODBUS_API float obmodbus_get_float_badc(const uint16_t *src);
OBMODBUS_API float obmodbus_get_float_cdab(const uint16_t *src);

OBMODBUS_API void obmodbus_set_float(float f, uint16_t *dest);
OBMODBUS_API void obmodbus_set_float_abcd(float f, uint16_t *dest);
OBMODBUS_API void obmodbus_set_float_dcba(float f, uint16_t *dest);
OBMODBUS_API void obmodbus_set_float_badc(float f, uint16_t *dest);
OBMODBUS_API void obmodbus_set_float_cdab(float f, uint16_t *dest);

#include "obmodbus-rtu.h"
#include "obmodbus-ascii.h"
#include "obmodbus-tcp.h"
#include "obmodbus-udp.h"
#include "obmodbus-rtu-over-tcp.h"
#include "obmodbus-ascii-over-tcp.h"
#include "obmodbus-rtu-over-udp.h"
#include "obmodbus-ascii-over-udp.h"

OBMODBUS_END_DECLS

#endif  /* OBMODBUS_H */
