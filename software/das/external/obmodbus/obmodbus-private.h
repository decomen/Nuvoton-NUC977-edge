/*
 * Copyright © 2010-2012 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#ifndef OBMODBUS_PRIVATE_H
#define OBMODBUS_PRIVATE_H

#ifndef _MSC_VER
# include <stdint.h>
# include <sys/time.h>
#else
# include "stdint.h"
# include <time.h>
typedef int ssize_t;
#endif
#include <sys/types.h>
#include <config.h>

#include "obmodbus.h"

OBMODBUS_BEGIN_DECLS

/* It's not really the minimal length (the real one is report slave ID
 * in RTU (4 bytes)) but it's a convenient size to use in RTU or TCP
 * communications to read many values or write a single one.
 * Maximum between :
 * - HEADER_LENGTH_TCP (7) + function (1) + address (2) + number (2)
 * - HEADER_LENGTH_RTU (1) + function (1) + address (2) + number (2) + CRC (2)
 */
#define _MIN_REQ_LENGTH 12

#define _REPORT_SLAVE_ID 180

#define _OBMODBUS_EXCEPTION_RSP_LENGTH 5

/* Timeouts in microsecond (0.5 s) */
#define _RESPONSE_TIMEOUT   500000
#define _BYTE_TIMEOUT       500000

typedef enum {
    _OBMODBUS_BACKEND_TYPE_RTU=0,
    _OBMODBUS_BACKEND_TYPE_ASCII,
    _OBMODBUS_BACKEND_TYPE_TCP,
    _OBMODBUS_BACKEND_TYPE_RTU_OVER_TCP,
    _OBMODBUS_BACKEND_TYPE_ASCII_OVER_TCP,
    _OBMODBUS_BACKEND_TYPE_UDP,
    _OBMODBUS_BACKEND_TYPE_RTU_OVER_UDP,
    _OBMODBUS_BACKEND_TYPE_ASCII_OVER_UDP,
} obmodbus_backend_type_t;

/*
 *  ---------- Request     Indication ----------
 *  | Client | ---------------------->| Server |
 *  ---------- Confirmation  Response ----------
 */
typedef enum {
    /* Request message on the server side */
    MSG_INDICATION,
    /* Request message on the client side */
    MSG_CONFIRMATION
} msg_type_t;

/* This structure reduces the number of params in functions and so
 * optimizes the speed of execution (~ 37%). */
typedef struct _sft {
    int slave;
    int function;
    int t_id;
} sft_t;

typedef struct _obmodbus_backend {
    unsigned int backend_type;
    unsigned int header_length;
    unsigned int checksum_length;
    unsigned int max_adu_length;
    int (*set_slave) (obmodbus_t *ctx, int slave);
    int (*build_request_basis) (obmodbus_t *ctx, int function, int addr,
                                int nb, uint8_t *req);
    int (*build_response_basis) (sft_t *sft, uint8_t *rsp);
    int (*prepare_response_tid) (const uint8_t *req, int *req_length);
    int (*send_msg_pre) (uint8_t *req, int req_length);
    ssize_t (*send) (obmodbus_t *ctx, const uint8_t *req, int req_length);
    int (*receive) (obmodbus_t *ctx, uint8_t *req);
    ssize_t (*recv) (obmodbus_t *ctx, uint8_t *rsp, int rsp_length);
    int (*check_integrity) (obmodbus_t *ctx, uint8_t *msg,
                            const int msg_length);
    int (*pre_check_confirmation) (obmodbus_t *ctx, const uint8_t *req,
                                   const uint8_t *rsp, int rsp_length);
    int (*connect) (obmodbus_t *ctx);
    void (*close) (obmodbus_t *ctx);
    int (*flush) (obmodbus_t *ctx);
    int (*select) (obmodbus_t *ctx, fd_set *rset, struct timeval *tv, int msg_length);
    void (*free) (obmodbus_t *ctx);
} obmodbus_backend_t;

struct _obmodbus {
    /* Slave address */
    int slave;
    /* Socket or file descriptor */
    int s;
    /* If UDP */
    int udp;
    /* If Ascii */
    int ascii;
    
    int debug;
    int error_recovery;
    struct timeval response_timeout;
    struct timeval byte_timeout;
    const obmodbus_backend_t *backend;
    const obmodbus_customize_backend_t *customize_backend;
    const obmodbus_monitor_t *monitor;
    void *backend_data;
    void *user_data;
};

void _obmodbus_init_common(obmodbus_t *ctx);
void _error_print(obmodbus_t *ctx, const char *context);
int _obmodbus_receive_msg(obmodbus_t *ctx, uint8_t *msg, msg_type_t msg_type);

#ifndef HAVE_STRLCPY
size_t strlcpy(char *dest, const char *src, size_t dest_size);
#endif

OBMODBUS_END_DECLS

#endif  /* OBMODBUS_PRIVATE_H */
