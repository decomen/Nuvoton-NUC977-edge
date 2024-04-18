/*
 * Copyright © 2001-2010 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#ifndef OBMODBUS_RTU_OVER_UDP_H
#define OBMODBUS_RTU_OVER_UDP_H

#include "obmodbus.h"

OBMODBUS_BEGIN_DECLS

OBMODBUS_API obmodbus_t* obmodbus_new_rtu_over_udp(const char *intfc, const char *host, int port);

OBMODBUS_END_DECLS

#endif /* OBMODBUS_TCP_H */
