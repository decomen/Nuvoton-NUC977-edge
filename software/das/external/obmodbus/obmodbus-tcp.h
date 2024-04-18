/*
 * Copyright © 2001-2010 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#ifndef OBMODBUS_TCP_H
#define OBMODBUS_TCP_H

#include "obmodbus.h"

OBMODBUS_BEGIN_DECLS

#if defined(_WIN32) && !defined(__CYGWIN__)
/* Win32 with MinGW, supplement to <errno.h> */
#include <winsock2.h>
#if !defined(ECONNRESET)
#define ECONNRESET   WSAECONNRESET
#endif
#if !defined(ECONNREFUSED)
#define ECONNREFUSED WSAECONNREFUSED
#endif
#if !defined(ETIMEDOUT)
#define ETIMEDOUT    WSAETIMEDOUT
#endif
#if !defined(ENOPROTOOPT)
#define ENOPROTOOPT  WSAENOPROTOOPT
#endif
#if !defined(EINPROGRESS)
#define EINPROGRESS  WSAEINPROGRESS
#endif
#endif

#define OBMODBUS_TCP_DEFAULT_PORT   502
#define OBMODBUS_TCP_SLAVE         0xFF

/* Modbus_Application_Protocol_V1_1b.pdf Chapter 4 Section 1 Page 5
 * TCP OBMODBUS ADU = 253 bytes + MBAP (7 bytes) = 260 bytes
 */
#define OBMODBUS_TCP_MAX_ADU_LENGTH  260

OBMODBUS_API obmodbus_t* obmodbus_new_tcp(const char *intfc, const char *host, int port);
OBMODBUS_API int obmodbus_tcp_listen(obmodbus_t *ctx, int nb_connection);
OBMODBUS_API int obmodbus_tcp_accept(obmodbus_t *ctx, int *s);

OBMODBUS_API obmodbus_t* obmodbus_new_tcp_pi(const char *node, const char *service);
OBMODBUS_API int obmodbus_tcp_pi_listen(obmodbus_t *ctx, int nb_connection);
OBMODBUS_API int obmodbus_tcp_pi_accept(obmodbus_t *ctx, int *s);

OBMODBUS_END_DECLS

#endif /* OBMODBUS_TCP_H */
