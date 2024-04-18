/*
 * Copyright © 2001-2010 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#ifndef MODBUS_ASCII_OVER_TCP_H
#define MODBUS_ASCII_OVER_TCP_H

#include "modbus.h"

MODBUS_BEGIN_DECLS

MODBUS_API modbus_t* modbus_new_ascii_over_tcp(const char *intfc, const char *host, int port);

MODBUS_END_DECLS

#endif /* MODBUS_TCP_H */
