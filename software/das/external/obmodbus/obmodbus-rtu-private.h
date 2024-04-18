/*
 * Copyright © 2001-2011 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#ifndef OBMODBUS_RTU_PRIVATE_H
#define OBMODBUS_RTU_PRIVATE_H

#ifndef _MSC_VER
#include <stdint.h>
#else
#include "stdint.h"
#endif

#if defined(_WIN32)
#include <windows.h>
#else
#include <termios.h>
#endif

#define _OBMODBUS_RTU_HEADER_LENGTH      1
#define _OBMODBUS_RTU_PRESET_REQ_LENGTH  6
#define _OBMODBUS_RTU_PRESET_RSP_LENGTH  2

#define _OBMODBUS_RTU_CHECKSUM_LENGTH    2

#if defined(_WIN32)
#if !defined(ENOTSUP)
#define ENOTSUP WSAEOPNOTSUPP
#endif

/* WIN32: struct containing serial handle and a receive buffer */
#define PY_BUF_SIZE 512
struct win32_ser {
    /* File handle */
    HANDLE fd;
    /* Receive buffer */
    uint8_t buf[PY_BUF_SIZE];
    /* Received chars */
    DWORD n_bytes;
};
#endif /* _WIN32 */

typedef struct _obmodbus_rtu {
    /* Device: "/dev/ttyS0", "/dev/ttyUSB0" or "/dev/tty.USA19*" on Mac OS X. */
    char *device;
    /* Bauds: 9600, 19200, 57600, 115200, etc */
    int baud;
    /* Data bit */
    uint8_t data_bit;
    /* Stop bit */
    uint8_t stop_bit;
    /* Parity: 'N', 'O', 'E' */
    char parity;
#if defined(_WIN32)
    struct win32_ser w_ser;
    DCB old_dcb;
#else
    /* Save old termios settings */
    struct termios old_tios;
#endif
#if HAVE_DECL_TIOCSRS485
    int serial_mode;
#endif
#if HAVE_DECL_TIOCM_RTS
    int rts;
    int rts_delay;
    int onebyte_time;
    void (*set_rts) (obmodbus_t *ctx, int on);
#endif
    /* To handle many slaves on the same link */
    int confirmation_to_ignore;
} obmodbus_rtu_t;

int _obmodbus_rtu_set_slave(obmodbus_t *ctx, int slave);
int _obmodbus_rtu_build_request_basis(obmodbus_t *ctx, int function, int addr, int nb, uint8_t *req);
int _obmodbus_rtu_build_response_basis(sft_t *sft, uint8_t *rsp);
uint16_t _obmodbus_rtu_crc16(uint8_t *buffer, uint16_t buffer_length);
int _obmodbus_rtu_prepare_response_tid(const uint8_t *req, int *req_length);
int _obmodbus_rtu_send_msg_pre(uint8_t *req, int req_length);
#if HAVE_DECL_TIOCM_RTS
void _obmodbus_rtu_ioctl_rts(obmodbus_t *ctx, int on);
#endif
ssize_t _obmodbus_rtu_send(obmodbus_t *ctx, const uint8_t *req, int req_length);
int _obmodbus_rtu_receive(obmodbus_t *ctx, uint8_t *req);
ssize_t _obmodbus_rtu_recv(obmodbus_t *ctx, uint8_t *rsp, int rsp_length);
int _obmodbus_rtu_pre_check_confirmation(obmodbus_t *ctx, const uint8_t *req, const uint8_t *rsp, int rsp_length);
int _obmodbus_rtu_check_integrity(obmodbus_t *ctx, uint8_t *msg, const int msg_length);
int _obmodbus_rtu_connect(obmodbus_t *ctx);
void _obmodbus_rtu_close(obmodbus_t *ctx);
int _obmodbus_rtu_flush(obmodbus_t *ctx);
int _obmodbus_rtu_select(obmodbus_t *ctx, fd_set *rset, struct timeval *tv, int length_to_read);
void _obmodbus_rtu_free(obmodbus_t *ctx);

#endif /* OBMODBUS_RTU_PRIVATE_H */
