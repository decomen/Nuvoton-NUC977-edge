/*
 * Copyright © 2001-2010 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#ifndef MODBUS_UDP_H
#define MODBUS_UDP_H

#include "modbus.h"

MODBUS_BEGIN_DECLS

MODBUS_API modbus_t* modbus_new_udp(const char *host, int port);
MODBUS_API int modbus_udp_listen(modbus_t *ctx, int nb_connection);

MODBUS_END_DECLS

#endif /* MODBUS_TCP_H */
